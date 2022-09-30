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
    delta = apvts.getRawParameterValue("delta");
    boost = apvts.getRawParameterValue("boost");

    for (int i = 0; i < 3; ++i) {
        crossovers[i] = apvts.getRawParameterValue("crossover" + std::to_string(i));
        apvts.addParameterListener("crossover" + std::to_string(i), this);
    }

    for (auto& b : m_Proc.bandBuffer)
        b.setSize(getTotalNumOutputChannels(), 16384);

    apvts.state.appendChild(numBandsTree, nullptr);

    apvts.state.addListener(this);

    checkActivation();

    startTimer(200);
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

    stopTimer();
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
    return currentPreset;
}

void MaximizerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void MaximizerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    maxNumSamples = samplesPerBlock;
    lastDownSampleRate = sampleRate;
    
    updateOversample();

    if (osIndex > 0)
        lastSampleRate = sampleRate * 4.0;
    else
        lastSampleRate = sampleRate;

    dsp::ProcessSpec spec{lastSampleRate, uint32(oversample[osIndex].getOversamplingFactor() * samplesPerBlock), (uint32)getTotalNumOutputChannels()};
                
    bypassBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
    bypassDelay.prepare(spec);

    for (auto& ovs : oversample)
        ovs.initProcessing(size_t(samplesPerBlock));
    for (auto& ovs : oversampleMono)
        ovs.initProcessing(size_t(samplesPerBlock));

    mPi.prepare();

    m_Proc.prepare(spec);
    m_Proc.initCrossovers();
    m_Proc.setOversamplingFactor(oversample[osIndex].getOversamplingFactor());
    
    filterLength = m_Proc.linBand[0].size;

    mixer.prepare(spec);
    mixer.setMixingRule(dsp::DryWetMixingRule::linear);

    inputMeter.prepare(spec);
    outputMeter.prepare(spec);

    widener.prepare(spec);
}

void MaximizerAudioProcessor::releaseResources()
{
    mPi.prepare();
    mixer.reset();
    m_Proc.reset();
    lastInputGain = 1.f;
    lastOutGain = 1.f;
    m_lastGain = 1.f;
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MaximizerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
    //  && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
    //     return false;

    // PluginHostType host;

    // // clumsily force stereo in Bitwig. CHANGE THIS!!
    // if (host.isBitwigStudio())
    // {
    //     if (layouts.getMainOutputChannelSet() == AudioChannelSet::stereo() &&
    //         layouts.getMainInputChannelSet() == AudioChannelSet::stereo())
    //         return true;
    // }
    // else
    // {
    //     if (layouts.getMainOutputChannelSet() == layouts.getMainInputChannelSet())
    //         return true;
    //     else if (layouts.getMainOutputChannelSet() == AudioChannelSet::stereo()) {
    //         // Mono-to-stereo
    //         if (layouts.getMainInputChannelSet() == AudioChannelSet::mono())
    //             return true;
    //     }
    // }
    
    if (layouts.getMainInputChannels() <= 2 && layouts.getMainOutputChannels() <= 2 && layouts.getMainInputChannels() <= layouts.getMainOutputChannels())
        return true;

    return false;
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

        // if (parameterID != "linearPhase")
        needs_resize.store(true);

        mPi.prepare();
    }

    if (parameterID.contains("crossover")) {
        crossover_changedID = parameterID.getTrailingIntValue();
        if (!*linearPhase) {
            m_Proc.updateCrossoverNonLin(crossover_changedID);
            m_Proc.updateCrossoverLin(crossover_changedID);
        }
        else {
            m_Proc.updateCrossoverLin(crossover_changedID);
            m_Proc.updateCrossoverNonLin(crossover_changedID);
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

    if (onPresetChange != nullptr && hasEditor())
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

    jassert(maxNumSamples >= buffer.getNumSamples());
    if (numSamples != buffer.getNumSamples())
        numSamples = buffer.getNumSamples();

    /*if mono->stereo, copy left channel into right*/
    if (totalNumInputChannels < totalNumOutputChannels)
        buffer.copyFrom(1, 0, buffer.getReadPointer(0), numSamples);
    
    bypassBuffer.makeCopyOf(buffer, true);
    bufferCopied = true;

    dsp::AudioBlock<float> block(buffer);
    dsp::ProcessContextReplacing<float> context(block);
    auto& outBlock = context.getOutputBlock();

    mixer.pushDrySamples(dsp::AudioBlock<float> (buffer));

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

    bool boostRamped = false;
    if (*boost)
    {
        if (*boost != lastBoost)
            buffer.applyGainRamp(0, numSamples, 1.f, 3.98f);
        else
            buffer.applyGain(3.98f);
    }
    else
        if (*boost != lastBoost)
            buffer.applyGainRamp(0, numSamples, 3.98f, 1.f);
    lastBoost = (bool)*boost;

    inputMeter.measureBlock(buffer);

    dsp::AudioBlock<float> osBlock;
    // if (totalNumOutputChannels > 1)
    osBlock = oversample[osIndex].processSamplesUp(dsp::AudioBlock<float>(buffer));
    // else
    //     osBlock = oversampleMono[osIndex].processSamplesUp(dsp::AudioBlock<float>(buffer));

    if (*bandSplit)
    {
        /* if oversample state has changed, update band specs on message thread */
        if (needs_resize.load()) {
            if (async.callAsync([&]{updateBandSpecs();})) {
                needs_resize.store(false);
                needs_update.store(true);
            }
        }

        /* if the band specs have been updated, copy input buffer into band buffers */
        if (!needs_update.load()) {
            for (auto& b : m_Proc.bandBuffer) {
                // if (b.getNumSamples() == osBlock.getNumSamples()) /* make sure band buffer is the same size as the oversampled input block */
                // {
                    b.copyFrom(0, 0, const_cast<float*>(osBlock.getChannelPointer(0)), osBlock.getNumSamples());
                    if (totalNumOutputChannels > 1)
                        b.copyFrom(1, 0, const_cast<float*>(osBlock.getChannelPointer(1)), osBlock.getNumSamples());
                // }
            }
        }
    }

    if (!*bandSplit)
        mPi.process(osBlock);
    else if (*bandSplit && !needs_update.load())
        m_Proc.processBands(osBlock);

    if (*distIndex)
    {
        const double r = 1.0 - (1.0 / (float(oversample[osIndex].getOversamplingFactor()) * 1000.0));

        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            auto in = osBlock.getChannelPointer(channel);

            // if (lastAsym != *distIndex) /* fade in if we just turned on Asym */
            // {
            //     auto inc = 1.0 / osBlock.getNumSamples();
            //     auto gain = 0.0;
            //     for (int i = 0; i < osBlock.getNumSamples(); ++i)
            //     {
            //         in[i] *= gain;
            //         gain += inc;
            //     }
            // }

            for (int i = 0; i < osBlock.getNumSamples(); ++i)
            {
                /*DC blocker*/
                double yn = in[i];
                in[i] = yn - xm1[channel] + r * ym1[channel];
                xm1[channel] = yn;
                ym1[channel] = in[i];
            }
        }
    }

    lastAsym = (bool)*distIndex;

    // if (totalNumOutputChannels > 1)
    oversample[osIndex].processSamplesDown(outBlock);
    // else
    //     oversampleMono[osIndex].processSamplesDown(outBlock);
    
    if (*width != 1.0 && totalNumOutputChannels > 1) {
        if (*monoWidth && *bandSplit && (*m_Proc.bandWidth[0] > 1.f || *m_Proc.bandWidth[1] > 1.f || *m_Proc.bandWidth[2] > 1.f ||
            *m_Proc.bandWidth[3] > 1.f))
            widener.widenBuffer(buffer, *width, false);
        else if (totalNumInputChannels < 2)
            widener.widenBuffer(buffer, *width, true);
        else
            widener.widenBuffer(buffer, *width, *monoWidth);
    }

    (*linearPhase && *bandSplit) ?
        setLatencySamples(osIndex > 1 ? 1596 : 6143) :
        setLatencySamples(oversample[osIndex].getLatencyInSamples());
    
    bypassDelay.setDelay(getLatencySamples());

    mixer.setWetLatency(getLatencySamples());

    if (*autoGain)
    {
        if (gainRamped)
            buffer.applyGainRamp(0, numSamples, 1.0 / (m_lastGain * halfPi), 1.0 / (gain_raw * halfPi));
        else
            buffer.applyGain(1.0 / (gain_raw * halfPi));

        if (*boost)
        {
            if (boostRamped)
                buffer.applyGainRamp(0, numSamples, 1.f, 1.f / 3.98f);
            else
                buffer.applyGain(1.f / 3.98f);
        }
        else if (!*boost && boostRamped)
            buffer.applyGainRamp(0, numSamples, 1.f / 3.98f, 1.f);
    }

    if (output_raw != lastOutGain) {
        buffer.applyGainRamp(0, numSamples, lastOutGain, output_raw);
        lastOutGain = output_raw;
    }
    else
        buffer.applyGain(output_raw);

    outputMeter.measureBlock(buffer);

    mixer.setWetMixProportion(*mix);
    mixer.mixWetSamples(dsp::AudioBlock<float> (buffer));
    
    if (totalNumInputChannels < totalNumOutputChannels) {
        if (*width <= 1.0 && (*m_Proc.bandWidth[0] <= 1.0 && *m_Proc.bandWidth[1] <= 1.0 &&
            *m_Proc.bandWidth[2] <= 1.0 && *m_Proc.bandWidth[3] <= 1.0))
            buffer.copyFrom(1, 0, buffer.getReadPointer(0), numSamples);
        //copy left->right if no wideners are active & mono->stereo
    }

    if (*delta)
        processDelta(buffer, gain_raw * halfPi, output_raw);
    if (*bypass || lastBypass)
        processBlockBypassed(buffer, midiMessages);
}

void MaximizerAudioProcessor::processBlockBypassed(AudioBuffer<float>& buffer, MidiBuffer&)
{
    /*return if not bypass & unchanged or switched on mid-buffer*/
    if (!bufferCopied)
        return;
    
    /*push to our delay buffer & pop delayed sample*/
    if (getTotalNumInputChannels() < getTotalNumOutputChannels()) {
        auto in = bypassBuffer.getWritePointer(0);
        auto in_1 = bypassBuffer.getWritePointer(1);
        for (int i = 0; i < bypassBuffer.getNumSamples(); ++i) {
            bypassDelay.pushSample(0, in[i]);
            in[i] = bypassDelay.popSample(0);
            in_1[i] = in[i];
        }
    }
    else {
        for (int ch = 0; ch < bypassBuffer.getNumChannels(); ++ch) {
            auto in = bypassBuffer.getWritePointer(ch);
            for (int i = 0; i < bypassBuffer.getNumSamples(); ++i) {
                bypassDelay.pushSample(ch, in[i]);
                in[i] = bypassDelay.popSample(ch);
            }
        }
    }

    /*copy as usual if no change*/
    if (*bypass && lastBypass)
        buffer.makeCopyOf(bypassBuffer, true);
    /*fade bypass in*/
    else if (*bypass && !lastBypass) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            buffer.applyGainRamp(ch, 0, buffer.getNumSamples(), 1.0, 0.0);
            buffer.addFromWithRamp(ch, 0, bypassBuffer.getReadPointer(ch),
                buffer.getNumSamples(), 0.0, 1.0);
        }
    }
    /*fade bypass out*/
    else if (!*bypass && lastBypass) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            buffer.applyGainRamp(ch, 0, buffer.getNumSamples(), 0.0, 1.0);
            buffer.addFromWithRamp(ch, 0, bypassBuffer.getReadPointer(ch),
                buffer.getNumSamples(), 1.0, 0.0);
        }
    }
    
    lastBypass = *bypass;
    bufferCopied = false;
}

void MaximizerAudioProcessor::processDelta(AudioBuffer<float>& buffer, float inGain, float outGain)
{
    if (getTotalNumInputChannels() < getTotalNumOutputChannels())
    {
        auto* data = buffer.getWritePointer(0);
        auto* dryData = bypassBuffer.getWritePointer(0);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            bypassDelay.pushSample(0, dryData[i]);
            dryData[i] = bypassDelay.popSample(0);
            if (!*autoGain) {
                data[i] = ((data[i] / inGain) / outGain) - dryData[i];
                buffer.setSample(1, i, data[i]);
            }
            else {
                data[i] = (data[i] / outGain) - dryData[i];
                buffer.setSample(1, i, data[i]);
            }
        }
    }
    else {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* data = buffer.getWritePointer(ch);
            auto* dryData = bypassBuffer.getWritePointer(ch);

            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                bypassDelay.pushSample(ch, dryData[i]);
                dryData[i] = bypassDelay.popSample(ch);
                if (!*autoGain)
                    data[i] = ((data[i] / inGain) / outGain) - dryData[i];
                else
                    data[i] = (data[i] / outGain) - dryData[i];
            }
        }
    }
}

void MaximizerAudioProcessor::updateBandSpecs()
{
    dsp::ProcessSpec newSpec{ lastSampleRate, uint32(maxNumSamples * oversample[osIndex].getOversamplingFactor()),
        (uint32)getTotalNumOutputChannels() };
    m_Proc.setOversamplingFactor(oversample[osIndex].getOversamplingFactor());
    m_Proc.reset();
    m_Proc.updateSpecs(newSpec);

    needs_update = false;
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
    xml->setAttribute("numBands", numBands);
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
        numBands = xmlState->getIntAttribute("numBands", numBands);
        if (numBands < 1)
            numBands == 1;
        currentPreset = xmlState->getStringAttribute("preset");
        //presetModified = xmlState->getBoolAttribute("presetModified");
        //if (xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(ValueTree::fromXml(*xmlState));
        auto numBandsTree = apvts.state.getChildWithProperty("id", "numBands");
        if (numBandsTree.isValid())
            updateNumBands(numBandsTree.getProperty("value"));
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

    params.emplace_back(std::make_unique<AudioParameterFloat>(ParameterID("gain", 1), "Input Gain", -12.0, 12.0, 0.0));
    params.emplace_back(std::make_unique < AudioParameterFloat>(ParameterID("output", 1), "Output Gain", -12.0, 0.0, -1.0));
    params.emplace_back(std::make_unique<AudioParameterFloat>(ParameterID("curve", 1), "Curve", curveRange, 1.f, String(), AudioProcessorParameter::genericParameter,
            [curveRange](float value, int)
            {
                float curve = jmap(curveRange.convertTo0to1(value), -100.f, 100.f);
                if (fabs(curve) >= 9.999)
                    return String(roundToInt(curve));
                else
                    return String(curve, 1);
            }, nullptr));
    params.emplace_back(std::make_unique<AudioParameterChoice>(ParameterID("distType", 1), "Saturation Type", StringArray("Symmetric", "Asymmetric"), 0));
    params.emplace_back(std::make_unique<AudioParameterChoice>(ParameterID("clipType", 1), "Saturation Limit", StringArray("Finite", "Clip", "Infinite", "Deep", "Warm"), 0));
    params.emplace_back(std::make_unique<AudioParameterBool>(ParameterID("bandSplit", 1), "Band Split On/Off", false));

    /*create 3 crossover freq params*/
    for (int i = 0; i < 3; ++i)
    {
        params.emplace_back(std::make_unique<AudioParameterFloat>(ParameterID("crossover" + std::to_string(i), 1),
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
        params.emplace_back(std::make_unique<AudioParameterFloat>(ParameterID("bandInGain" + std::to_string(i), 1),
            "Band " + std::to_string(i + 1) + " Input Gain", -12.0, 12.0, 0.0));
        params.emplace_back(std::make_unique<AudioParameterFloat>(ParameterID("bandOutGain" + std::to_string(i), 1),
            "Band " + std::to_string(i + 1) + " Output Gain", -12.0, 12.0, 0.0));
        params.emplace_back(std::make_unique<AudioParameterFloat>(ParameterID("bandWidth" + std::to_string(i), 1),
            "Band " + std::to_string(i + 1) + " Width", widthRange, 1.0, String(), AudioProcessorParameter::genericParameter,
            [widthRange](float value, int)
            {
                float percentage = jmap(widthRange.convertTo0to1(value), 0.f, 200.f);

                return String(percentage, 0);

            }, [widthRange](const String& text) { return text.getFloatValue() / 100.f; }));
        params.emplace_back(std::make_unique<AudioParameterBool>(ParameterID("soloBand" + std::to_string(i), 1),
            "Solo Band " + std::to_string(i + 1), false));
        params.emplace_back(std::make_unique<AudioParameterBool>(ParameterID("muteBand" + std::to_string(i), 1),
            "Mute Band " + std::to_string(i + 1), false));
        params.emplace_back(std::make_unique<AudioParameterBool>(ParameterID("bypassBand" + std::to_string(i), 1),
            "Bypass Band " + std::to_string(i + 1), false));
    }
    
    params.emplace_back(std::make_unique<AudioParameterBool>(ParameterID("hq", 1), "HQ", false));
    params.emplace_back(std::make_unique<AudioParameterBool>(ParameterID("renderHQ", 1), "Render HQ", false));
    params.emplace_back(std::make_unique<AudioParameterBool>(ParameterID("linearPhase", 1), "Linear Phase", false));
    params.emplace_back(std::make_unique<AudioParameterFloat>(ParameterID("width", 1), "Width", widthRange, 1.f, String(), AudioProcessorParameter::genericParameter,
            [widthRange](float value, int)
            {
                float percentage = jmap(widthRange.convertTo0to1(value), 0.f, 200.f);
                
                return String(percentage, 0);

            }, [widthRange](const String& text) { return text.getFloatValue() / 100.f; }));
    params.emplace_back(std::make_unique<AudioParameterBool>(ParameterID("monoWidth", 1), "Widen Mono", false));
    params.emplace_back(std::make_unique<AudioParameterFloat>(ParameterID("mix", 1), "Dry/Wet Mix", mixRange, 1.f, String(), AudioProcessorParameter::genericParameter,
            [](float value, int)
            {
                float percentage = jmap(value, 0.f, 100.f);

                return String(percentage, 0);

            }, [widthRange](const String& text) { return text.getFloatValue() / 100.f; }));
    params.emplace_back(std::make_unique<AudioParameterBool>(ParameterID("autoGain", 1), "Auto Gain", false));
    params.emplace_back(std::make_unique<AudioParameterBool>(ParameterID("boost", 1), "Input Boost", false));
    params.emplace_back(std::make_unique<AudioParameterBool>(ParameterID("bypass", 1), "Bypass", false));
    params.emplace_back(std::make_unique<AudioParameterBool>(ParameterID("delta", 1), "Delta", false));

    return { params.begin(), params.end() };
}

void MaximizerAudioProcessor::checkActivation()
{
    PluginHostType host;
    File dir = File(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory)
        .getFullPathName() + "/Arboreal Audio/PiMax/License/license.aal");
    File dirGB;
    uint64 gbuuid;
    
    auto uuid = String(OnlineUnlockStatus::MachineIDUtilities::getLocalMachineIDs().strings[0].hashCode64());

#if JUCE_WINDOWS
    String timeFile = "HKEY_CURRENT_USER\\SOFTWARE\\Arboreal Audio\\PiMax\\TrialKey";
#elif JUCE_MAC
    dirGB = File("~/Music/Audio Music Apps/Arboreal Audio/PiMax/License/license.aal");
    File timeFile = File(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory)
                             .getFullPathName() + "/Arboreal Audio/PiMax/License/trialkey.aal");
    File timeFileGB = File("~/Music/Audio Music Apps/Arboreal Audio/PiMax/License/trialkey.aal");
    gbuuid = File("~/Music/Audio Music Apps").getFileIdentifier();
#endif
    if (!host.isGarageBand()) {
        if (dir.exists() && !checkUnlock()) {
            auto xml = parseXML(dir);
            isUnlocked = (uuid == xml->getStringAttribute("uuid"));
            if (xml->hasAttribute("key"))
                isUnlocked = xml->getStringAttribute("key") != " ";
        }
    }
    else {
        if (dirGB.exists() && !checkUnlock()) {
            auto xml = parseXML(dirGB);
            isUnlocked = String(gbuuid) == xml->getStringAttribute("GBuuid");
        }
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
    if (!host.isGarageBand()) {
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
    }
    if (!timeFileGB.exists()) {
        timeFileGB.create();
        auto trialStart = Time::getCurrentTime();
        XmlElement xml{ "TrialKey" };
        xml.setAttribute("key", String(trialStart.toMilliseconds()));
        xml.writeTo(timeFileGB);
        timeFileGB.setReadOnly(true);
    }
    else {
        auto xml = parseXML(timeFileGB);

        auto trialEnd = Time(xml->getStringAttribute("key").getLargeIntValue());
        trialEnd += RelativeTime::days(7);
        trialEnded = (trialEnd <= Time::getCurrentTime());
        if (!trialEnded)
            trialRemaining_ms = trialEnd.toMilliseconds() - Time::getCurrentTime().toMilliseconds();
    }
#endif
}
