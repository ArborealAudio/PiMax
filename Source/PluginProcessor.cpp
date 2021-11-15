/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MaximizerAudioProcessor::MaximizerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts(*this, nullptr, "Parameters", createParams()), mixer(44100),
                        mPi(apvts), m_Proc(apvts)
#endif
{
    lastUIWidth = 720;
    lastUIHeight = 480;
    hq = apvts.getRawParameterValue("hq");
    apvts.addParameterListener("hq", this);
    renderHQ = apvts.getRawParameterValue("renderHQ");
    apvts.addParameterListener("renderHQ", this);
    gain_dB = apvts.getRawParameterValue("gain");
    curve = apvts.getRawParameterValue("curve");
    distIndex = apvts.getRawParameterValue("distType");
    clip = apvts.getRawParameterValue("clipType");
    output_dB = apvts.getRawParameterValue("output");
    bandSplit = apvts.getRawParameterValue("bandSplit");
    apvts.addParameterListener("bandSplit", this);
    linearPhase = apvts.getRawParameterValue("linearPhase");
    apvts.addParameterListener("linearPhase", this);
    width = apvts.getRawParameterValue("width");
    monoWidth = apvts.getRawParameterValue("monoWidth");
    mix = apvts.getRawParameterValue("mix");
    autoGain = apvts.getRawParameterValue("autoGain");
    bypass = apvts.getRawParameterValue("bypass");

    for (int i = 0; i < 3; ++i) {
        crossovers[i] = apvts.getRawParameterValue("crossover" + std::to_string(i));
        apvts.addParameterListener("crossover" + std::to_string(i), this);
    }

    apvts.state.appendChild(numBandsTree, nullptr);

    apvts.state.addListener(this);

    inputMeter.setMaxHoldMS(1000);
    outputMeter.setMaxHoldMS(1000);

    checkActivation();
}

MaximizerAudioProcessor::~MaximizerAudioProcessor()
{
    apvts.removeParameterListener("hq", this);
    apvts.removeParameterListener("renderHQ", this);
    apvts.removeParameterListener("bandSplit", this);
    apvts.removeParameterListener("linearPhase", this);
    for (int i = 0; i < 3; ++i)
        apvts.removeParameterListener("crossover" + std::to_string(i), this);

    apvts.state.removeListener(this);
}

//==============================================================================
const juce::String MaximizerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MaximizerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MaximizerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MaximizerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MaximizerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MaximizerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int MaximizerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MaximizerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String MaximizerAudioProcessor::getProgramName (int index)
{
    return {};
}

void MaximizerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void MaximizerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    numSamples = samplesPerBlock;
    lastDownSampleRate = sampleRate;
    
    updateOversample();

    if (osIndex == 2 || osIndex == 1)
        lastSampleRate = sampleRate * 4.0;
    else
        lastSampleRate = sampleRate;

    dsp::ProcessSpec spec{lastSampleRate, uint32(oversample[osIndex].getOversamplingFactor() * samplesPerBlock),
        (uint32)getTotalNumInputChannels()};

#if USE_SIMD_SAT
    interleaved = dsp::AudioBlock<Vec2>(interleavedBlockData, 1, spec.maximumBlockSize);
    zero = dsp::AudioBlock<float>(zeroData, Vec2::size(), spec.maximumBlockSize);

    zero.clear();
    spec.numChannels = 1;
#endif

    for (auto& ovs : oversample)
        ovs.initProcessing(size_t(samplesPerBlock));

    mPi.prepare();

    m_Proc.prepare(spec);
    m_Proc.initCrossovers();
    m_Proc.setOversamplingFactor(oversample[osIndex].getOversamplingFactor());

    for (auto& b : m_Proc.bandBuffer) {
#if USE_SIMD_SAT
        b.setSize(1, samplesPerBlock * oversample[osIndex].getOversamplingFactor());
#else
        suspendProcessing(true);
        b.setSize(2, samplesPerBlock * oversample[osIndex].getOversamplingFactor());
        suspendProcessing(false);
#endif
    }
    filterLength = m_Proc.linBand[0].size;

    mixer.prepare(spec);
    mixer.setMixingRule(dsp::DryWetMixingRule::linear);

    inputMeter.resize(2, sampleRate * 0.1 / samplesPerBlock);
    outputMeter.resize(2, sampleRate * 0.1 / samplesPerBlock);

    widener.prepare(spec);

}

void MaximizerAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MaximizerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

inline void MaximizerAudioProcessor::updateOversample() noexcept
{
    if (*renderHQ && isNonRealtime())
        osIndex = 2;
    else if (*hq && *linearPhase)
        osIndex = 2;
    else if (*hq && !*linearPhase)
        osIndex = 1;
    else 
        osIndex = 0;
}

void MaximizerAudioProcessor::parameterChanged(const String& parameterID, float newValue)
{
    if (trialEnded && !checkUnlock())
        return;

    if (parameterID == "hq" || parameterID == "renderHQ" || parameterID == "linearPhase") {
        if (*renderHQ && isNonRealtime()) {
            osIndex = 2;
            lastSampleRate = lastDownSampleRate * 4.0;
        }
        else if (*hq && *linearPhase) {
            osIndex = 2;
            lastSampleRate = lastDownSampleRate * 4.0;
        }
        else if (*hq) {
            osIndex = 1;
            lastSampleRate = lastDownSampleRate * 4.0;
        }
        else {
            osIndex = 0;
            lastSampleRate = lastDownSampleRate;
        }

        suspendProcessing(true);
        for (auto& b : m_Proc.bandBuffer)
            b.setSize(2, numSamples * oversample[osIndex].getOversamplingFactor(), false, false, true);
        suspendProcessing(false);

        dsp::ProcessSpec newSpec{ lastSampleRate, uint32(numSamples * oversample[osIndex].getOversamplingFactor()),
            (uint32)getTotalNumInputChannels() };
        m_Proc.setOversamplingFactor(oversample[osIndex].getOversamplingFactor());
        if (*bandSplit)
            suspendProcessing(true);
        m_Proc.updateSpecs(newSpec);
        suspendProcessing(false);
        mPi.prepare();
    }

    else if (*bandSplit) {
        if (parameterID.contains("crossover")) {
            if (!*linearPhase) {
                m_Proc.updateCrossoverNonLin(parameterID.getTrailingIntValue());
                m_Proc.updateCrossoverLin(parameterID.getTrailingIntValue());
            }
            else {
                m_Proc.updateCrossoverLin(parameterID.getTrailingIntValue());
                m_Proc.updateCrossoverNonLin(parameterID.getTrailingIntValue());
            }
        }
    }
}

void MaximizerAudioProcessor::valueTreeRedirected(ValueTree& treeWhichHasBeenChanged)
{
    auto child = treeWhichHasBeenChanged.getChildWithProperty("id", "numBands");
    if (child.isValid()) {
        auto num = child.getProperty("value");
        if (num.operator int() != numBands)
            updateNumBands((int)num);
    }
}

void MaximizerAudioProcessor::updateNumBands(int newNumBands) noexcept
{
    numBands = newNumBands;

    suspendProcessing(true);
    m_Proc.updateAllCrossovers(numBands);
    suspendProcessing(false);

    /*write numBands change to tree state*/
    auto val = apvts.state.getChildWithProperty("id", "numBands");
    val.setProperty("value", numBands, nullptr);

    if (onPresetChange != nullptr)
        onPresetChange();
}

void MaximizerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (trialEnded && !checkUnlock())
        return;

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    dsp::AudioBlock<float> block(buffer);
    dsp::ProcessContextReplacing<float> context(block);
    const auto& inputBlock = context.getInputBlock();
    auto& outBlock = context.getOutputBlock();

    mixer.pushDrySamples(inputBlock);

    float gain_raw = pow(10.f, (*gain_dB / 20.f));
    float output_raw = pow(10.f, (*output_dB / 20.f));

    bool gainRamped = false;
    if (gain_raw != lastInputGain) {
        gainRamped = true;
        m_lastGain = lastInputGain;
        buffer.applyGainRamp(0, buffer.getNumSamples(), lastInputGain, gain_raw);
        lastInputGain = gain_raw;
    }
    else
        buffer.applyGain(gain_raw);

    //const auto curveCoeff = exp(log(0.01f) / (100 * lastSampleRate * 0.001));
    //SmoothedValue<float> smoothCurve(lastK);
    //smoothCurve.reset(numSamples);
    //smoothCurve.setTargetValue(*curve);

    inputMeter.measureBlock(buffer);

    auto osBlock = oversample[osIndex].processSamplesUp(inputBlock);

#if USE_SIMD_SAT
    /*if (*bandSplit) {
        for (auto& b : m_Proc.bandBuffer) {
            b.setSize(0, numSamples * oversample[osIndex].getOversamplingFactor(), false, false, true);
            auto buf = b.getWritePointer(0);
            auto block = osBlock.getChannelPointer(0);
            for (size_t i = 0; i < osBlock.getNumSamples(); ++i)
            {
                buf[i] = (Vec2)block[i];
            }
        }
    }*/
#else
    if (*bandSplit) {
        for (auto& b : m_Proc.bandBuffer) {
            b.copyFrom(0, 0, const_cast<float*>(osBlock.getChannelPointer(0)), osBlock.getNumSamples());
            b.copyFrom(1, 0, const_cast<float*>(osBlock.getChannelPointer(1)), osBlock.getNumSamples());
        }
    }
#endif

    const double r = 1.0 - (1.0 / (float(oversample[osIndex].getOversamplingFactor()) * 1000.0));

    dsp::ProcessContextReplacing<float> osContext(osBlock);
    const auto& input = osContext.getInputBlock();
    auto& output = osContext.getOutputBlock();

#if USE_SIMD_SAT
    auto* inOut = channelPointers.getData();

    for (size_t ch = 0; ch < Vec2::size(); ++ch)
        inOut[ch] = (ch < input.getNumChannels() ? input.getChannelPointer(ch) :
            zero.getChannelPointer(ch));

    interleaveSamples(inOut, reinterpret_cast<float*>(interleaved.getChannelPointer(0)),
        numSamples, static_cast<int>(Vec2::size()));
#endif

#if USE_SIMD_SAT
    if (!bandSplit)
        mPi.process(dsp::ProcessContextReplacing<Vec2>(interleaved));
    
#else

    if (!*bandSplit) {
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            for (int i = 0; i < input.getNumSamples(); ++i)
            {
                double yn = 0.0, out = 0.0;
                double in = input.getSample(channel, i);
                out = mPi.processSample(channel, in);
                
                if (*distIndex == 1) {
                    /*DC blocker*/
                    yn = out;
                    out = yn - xm1[channel] + r * ym1[channel];
                    xm1[channel] = yn;
                    ym1[channel] = out;
                }
                output.setSample(channel, i, out);
            }
        }
    }
#endif

#if USE_CONVOLUTION
#if USE_SIMD_SAT
    if (*bandSplit) {
        if (*linearPhase)
            m_Proc.addBandsConvolution(dsp::ProcessContextReplacing<vec>(interleaved));
        else
            m_Proc.addBands(dsp::ProcessContextReplacing<vec>(interleaved));
    }
#else
    if (*bandSplit) {
        if (*linearPhase)
            m_Proc.addBandsConvolution(osContext);
        else
            m_Proc.addBands(osContext);
    }
#endif
#else
    if (*bandSplit)
        m_Proc.addBands(osContext);
#endif

#if USE_SIMD_SAT
    deinterleaveSamples(reinterpret_cast<float*>(interleaved.getChannelPointer(0)), const_cast<float**>(inOut),
        numSamples, static_cast<int>(Vec2::size()));
#endif

    oversample[osIndex].processSamplesDown(outBlock);

    if (*width != 1.0) {
        if (*monoWidth && (*m_Proc.bandWidth[0] > 1.f || *m_Proc.bandWidth[1] > 1.f || *m_Proc.bandWidth[2] > 1.f ||
            *m_Proc.bandWidth[3] > 1.f))
            widener.widenBlock(outBlock, *width, false);
        else
            widener.widenBlock(outBlock, *width, *monoWidth);
    }

    int hqLinBandLatency = buffer.getNumSamples() > 256 ? 1090 - (buffer.getNumSamples() - 256) : 1090;

    (*linearPhase && *bandSplit) ? 
        setLatencySamples(1535 + (osIndex > 1 ? - hqLinBandLatency : 0)) :
        setLatencySamples(oversample[osIndex].getLatencyInSamples());

    mixer.setWetLatency((*linearPhase && *bandSplit) ?
        1535 + (osIndex > 1 ? - hqLinBandLatency : 0) :
        oversample[osIndex].getLatencyInSamples());

    if (*autoGain && *output_dB == 0.0)
        output_raw = 0.999;

    if (gainRamped && *autoGain && !*bypass)
        buffer.applyGainRamp(0, buffer.getNumSamples(), 1.0 / m_lastGain, 1.0 / gain_raw);
    else if (*autoGain && !*bypass)
        buffer.applyGain(1.0 / gain_raw);

    if (output_raw != lastOutGain) {
        buffer.applyGainRamp(0, buffer.getNumSamples(), lastOutGain, output_raw);
        lastOutGain = output_raw;
    }
    else
        buffer.applyGain(output_raw);

    outputMeter.measureBlock(buffer);

    if (*bypass)
        mixer.setWetMixProportion(0.0);
    else
        mixer.setWetMixProportion(*mix);

    mixer.mixWetSamples(outBlock);
}

//==============================================================================
bool MaximizerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MaximizerAudioProcessor::createEditor()
{
    return new MaximizerAudioProcessorEditor (*this);
}

AudioProcessorEditor* MaximizerAudioProcessor::getEditor() const noexcept
{
    return dynamic_cast<AudioProcessorEditor*>(getActiveEditor());
}

//==============================================================================
void MaximizerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    auto xml = state.createXml();
    xml->setAttribute("uiWidth", lastUIWidth);
    xml->setAttribute("uiHeight", lastUIHeight);
    //xml->setAttribute("numBands", numBands);
    xml->setAttribute("preset", currentPreset);
    //presetModified = manager->compareStates(currentPreset);
    //xml->setAttribute("presetModified", presetModified);
    copyXmlToBinary(*xml, destData);
}

void MaximizerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (trialEnded && !checkUnlock())
        return;

    auto xmlState = getXmlFromBinary(data, sizeInBytes);
    if (xmlState.get() != nullptr) {
        lastUIWidth = xmlState->getIntAttribute("uiWidth", lastUIWidth);
        lastUIHeight = xmlState->getIntAttribute("uiHeight", lastUIHeight);
        //numBands = xmlState->getIntAttribute("numBands", numBands);
        currentPreset = xmlState->getStringAttribute("preset");
        //presetModified = xmlState->getBoolAttribute("presetModified");
        //if (xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(ValueTree::fromXml(*xmlState));
        updateNumBands(apvts.state.getChildWithProperty("id", "numBands").getProperty("value"));
    }
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MaximizerAudioProcessor();
}

AudioProcessorValueTreeState::ParameterLayout MaximizerAudioProcessor::createParams()
{
    NormalisableRange<float> curveRange(0.8f, 1.2f, 0.001f, 1.f);
    curveRange.setSkewForCentre(1.0);
    NormalisableRange<float> bandRange(40.f, 18000.f, 0.01f);
    NormalisableRange<float> widthRange(0.f, 2.f, 0.01f);
    widthRange.setSkewForCentre(1.0);
    NormalisableRange<float> mixRange(0.f, 1.f, 0.01f);

    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.emplace_back(std::make_unique<AudioParameterFloat>
        ("gain", "Input Gain", -12.0, 12.0, 0.0));
    params.emplace_back(std::make_unique < AudioParameterFloat>
        ("output", "Output Gain", -12.0, 0.0, -1.0));
    params.emplace_back(std::make_unique<AudioParameterFloat>
        ("curve", "Curve", curveRange, 1.f, String(), AudioProcessorParameter::genericParameter,
            [curveRange](float value, int)
            {
                float curve = static_cast<int> (jmap(curveRange.convertTo0to1(value), -100.f, 100.f));
                return String(curve, 0);
            }, nullptr));
    params.emplace_back(std::make_unique<AudioParameterChoice>
        ("distType", "Saturation Type", StringArray("Symmetric", "Asymmetric"), 0));
    params.emplace_back(std::make_unique<AudioParameterChoice>
        ("clipType", "Saturation Limit", StringArray("Finite", "Clip", "Infinite"), 0));
    params.emplace_back(std::make_unique<AudioParameterBool>
        ("bandSplit", "Band Split On/Off", false));

    /*create 3 crossover freq params*/
    for (int i = 0; i < 3; ++i)
    {
        params.emplace_back(std::make_unique<AudioParameterFloat>("crossover" + std::to_string(i),
            "Crossover " + std::to_string(i + 1) + " Freq", bandRange, i == 0 ? 150.0 :
            i == 1 ? 2500.0 : 40.0,
            String(), AudioProcessorParameter::genericParameter,
            [](float value, int) {float cutoff = (int)value; return String(cutoff, 0); },
            [](const String& text) {
                return text.contains("k") || text.contains("K") ?
                     text.getFloatValue() * 1000.f :
                     text.getFloatValue();
            }
        ));
    }
    /*create params for solo/mono/bypass for each band*/
    for (int i = 0; i < 4; ++i)
    {
        params.emplace_back(std::make_unique<AudioParameterFloat>("bandInGain" + std::to_string(i),
            "Band " + std::to_string(i + 1) + " Input Gain", -12.0, 12.0, 0.0));
        params.emplace_back(std::make_unique<AudioParameterFloat>("bandOutGain" + std::to_string(i),
            "Band " + std::to_string(i + 1) + " Output Gain", -12.0, 12.0, 0.0));
        params.emplace_back(std::make_unique<AudioParameterFloat>("bandWidth" + std::to_string(i),
            "Band " + std::to_string(i + 1) + " Width", widthRange, 1.0, String(), AudioProcessorParameter::genericParameter,
            [widthRange](float value, int)
            {
                float percentage = jmap(widthRange.convertTo0to1(value), 0.f, 200.f);

                return String(percentage, 0);

            }, [widthRange](const String& text) { return text.getFloatValue() / 100.f; }));
        params.emplace_back(std::make_unique<AudioParameterBool>("soloBand" + std::to_string(i),
            "Solo Band " + std::to_string(i + 1), false));
        params.emplace_back(std::make_unique<AudioParameterBool>("muteBand" + std::to_string(i),
            "Mute Band " + std::to_string(i + 1), false));
        params.emplace_back(std::make_unique<AudioParameterBool>("bypassBand" + std::to_string(i),
            "Bypass Band " + std::to_string(i + 1), false));
    }
    
    params.emplace_back(std::make_unique<AudioParameterBool>("hq", "HQ", false));
    params.emplace_back(std::make_unique<AudioParameterBool>("renderHQ", "Render HQ", false));
    params.emplace_back(std::make_unique<AudioParameterBool>("linearPhase", "Linear Phase", false));
    params.emplace_back(std::make_unique<AudioParameterFloat>
        ("width", "Width", widthRange, 1.f, String(), AudioProcessorParameter::genericParameter,
            [widthRange](float value, int)
            {
                float percentage = jmap(widthRange.convertTo0to1(value), 0.f, 200.f);
                
                return String(percentage, 0);

            }, [widthRange](const String& text) { return text.getFloatValue() / 100.f; }));
    params.emplace_back(std::make_unique<AudioParameterBool>("monoWidth", "Widen Mono", false));
    params.emplace_back(std::make_unique<AudioParameterFloat>
        ("mix", "Dry/Wet Mix", mixRange, 1.f, String(), AudioProcessorParameter::genericParameter,
            [](float value, int)
            {
                float percentage = jmap(value, 0.f, 100.f);

                return String(percentage, 0);

            }, [widthRange](const String& text) { return text.getFloatValue() / 100.f; }));
    params.emplace_back(std::make_unique<AudioParameterBool>
        ("autoGain", "Auto Gain", false));
    params.emplace_back(std::make_unique<AudioParameterBool>
        ("bypass", "Bypass", false));

    return { params.begin(), params.end() };
}

void MaximizerAudioProcessor::checkActivation()
{
    auto dir = File(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory)
        .getFullPathName() + "/Arboreal Audio/PiMax/License/license.aal");

#if JUCE_WINDOWS
    String timeFile = "HKEY_CURRENT_USER\\SOFTWARE\\Arboreal Audio\\PiMax\\TrialKey";
#elif JUCE_MAC
    File timeFile = File(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory)
        .getFullPathName() + "/Arboreal Audio/PiMax/License/trialkey.aal");
#endif

    if (dir.exists() && !checkUnlock()) {
        auto xml = parseXML(dir);
        auto uuid = String(OnlineUnlockStatus::MachineIDUtilities::getLocalMachineIDs().strings[0].hashCode64());
        isUnlocked = uuid == xml->getStringAttribute("uuid");
    }
#if JUCE_WINDOWS
    if (!WindowsRegistry::valueExists(timeFile)) {
        auto trialStart = Time::getCurrentTime();
        WindowsRegistry::setValue(timeFile, String(trialStart.toMilliseconds()));
    }
    else {
        auto trialEnd = Time(WindowsRegistry::getValue(timeFile).getLargeIntValue());
        trialEnd += RelativeTime::days(7);
        trialEnded = (trialEnd <= Time::getCurrentTime());
        if (!trialEnded)
            trialRemaining_ms = trialEnd.toMilliseconds() - Time::getCurrentTime().toMilliseconds();
    }
#elif JUCE_MAC
    if (!timeFile.exists()) {
        timeFile.create();
        auto trialStart = Time::getCurrentTime();
        XmlElement xml{ "TrialKey" };
        xml.setAttribute("key", String(trialStart.toMilliseconds()));
        xml.writeTo(timeFile);
        timeFile.setReadOnly(true);
    }
    else {
        auto xml = parseXML(timeFile);

        auto trialEnd = Time(xml->getStringAttribute("key").getLargeIntValue());
        trialEnd += RelativeTime::days(7);
        trialEnded = (trialEnd <= Time::getCurrentTime());
        if (!trialEnded)
            trialRemaining_ms = trialEnd.toMilliseconds() - Time::getCurrentTime().toMilliseconds();
    }
#endif
}
