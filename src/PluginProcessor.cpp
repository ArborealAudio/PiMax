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
    : AudioProcessor(
          BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
              ),
      apvts(*this, nullptr, "Parameters", createParams()), mixer(44100),
      mPi(apvts), mbProc(apvts)
#endif

{
    hq = apvts.getRawParameterValue("hq");
    apvts.addParameterListener("hq", this);
    renderHQ = apvts.getRawParameterValue("renderHQ");
    apvts.addParameterListener("renderHQ", this);
    gain_dB = dynamic_cast<strix::FloatParameter *>(apvts.getParameter("gain"));
    curve = dynamic_cast<strix::FloatParameter *>(apvts.getParameter("curve"));
    distType = apvts.getRawParameterValue("distType");
    lastAsym = (bool)distType->load();
    clip = apvts.getRawParameterValue("clipType");
    output_dB =
        dynamic_cast<strix::FloatParameter *>(apvts.getParameter("output"));
    bandSplit = apvts.getRawParameterValue("bandSplit");
    apvts.addParameterListener("bandSplit", this);
    linearPhase = apvts.getRawParameterValue("linearPhase");
    apvts.addParameterListener("linearPhase", this);
    width = dynamic_cast<strix::FloatParameter *>(apvts.getParameter("width"));
    monoWidth = apvts.getRawParameterValue("monoWidth");
    mix = dynamic_cast<strix::FloatParameter *>(apvts.getParameter("mix"));
    autoGain = apvts.getRawParameterValue("autoGain");
    bypass = apvts.getRawParameterValue("bypass");
    delta = apvts.getRawParameterValue("delta");
    boost = apvts.getRawParameterValue("boost");

    for (int i = 0; i < 3; ++i)
        apvts.addParameterListener("crossover" + String(i), this);

    apvts.state.appendChild(numBandsTree, nullptr);

    apvts.state.addListener(this);

    checkActivation();
}

MaximizerAudioProcessor::~MaximizerAudioProcessor()
{
    apvts.removeParameterListener("hq", this);
    apvts.removeParameterListener("renderHQ", this);
    apvts.removeParameterListener("bandSplit", this);
    apvts.removeParameterListener("linearPhase", this);
    for (int i = 0; i < 3; ++i)
        apvts.removeParameterListener("crossover" + String(i), this);

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

double MaximizerAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int MaximizerAudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are
              // 0 programs, so this should be at least 1, even if you're not
              // really implementing programs.
}

int MaximizerAudioProcessor::getCurrentProgram() { return 0; }

void MaximizerAudioProcessor::setCurrentProgram(int index) {}

const juce::String MaximizerAudioProcessor::getProgramName(int index)
{
    return currentPreset;
}

void MaximizerAudioProcessor::changeProgramName(int index,
                                                const juce::String &newName)
{
}

//==============================================================================
void MaximizerAudioProcessor::prepareToPlay(double sampleRate,
                                            int samplesPerBlock)
{
    maxNumSamples = samplesPerBlock;
    lastDownSampleRate = sampleRate;

    updateOversample();

    dsp::ProcessSpec spec{lastSampleRate,
                          uint32(oversample[osIndex].getOversamplingFactor() *
                                 (uint32)samplesPerBlock),
                          (uint32)getTotalNumOutputChannels()};

    bypassBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock, false,
                         true, true);
    bypassDelay.prepare(spec);

    for (auto &ovs : oversample)
        ovs.initProcessing(size_t(samplesPerBlock));
    for (auto &ovs : oversampleMono)
        ovs.initProcessing(size_t(samplesPerBlock));

    for (auto &s : smoothOffset)
        s.reset(lastSampleRate, 0.5);

    distState = distType->load() ? SymToAsym : Sym;

    mPi.prepare(spec);

    mbProc.prepare(spec);
    mbProc.setOversamplingFactor(
        (int)oversample[osIndex].getOversamplingFactor());

    filterLength = mbProc.linBand[0].mSize;

    mixer.prepare(spec);
    mixer.setMixingRule(dsp::DryWetMixingRule::linear);

    inputMeter.prepare(spec, 0.01f);
    outputMeter.prepare(spec, 0.01f);

    widener.prepare(spec);
}

void MaximizerAudioProcessor::releaseResources()
{
    mPi.reset();
    mixer.reset();
    mbProc.reset();
    lastInputGain = 1.f;
    lastOutGain = 1.f;
    m_lastGain = 1.f;
    lastAsym = false;
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MaximizerAudioProcessor::isBusesLayoutSupported(
    const BusesLayout &layouts) const
{
    if (layouts.getMainInputChannels() <= 2 &&
        layouts.getMainOutputChannels() <= 2 &&
        layouts.getMainInputChannels() <= layouts.getMainOutputChannels())
        return true;

    return false;
}
#endif

inline void MaximizerAudioProcessor::updateOversample() noexcept
{
    if ((bool)*renderHQ && isNonRealtime()) {
        osIndex = 2;
        lastSampleRate = lastDownSampleRate * 4.0;
    } else if ((bool)*hq && (bool)*linearPhase) {
        osIndex = 2;
        lastSampleRate = lastDownSampleRate * 4.0;
    } else if ((bool)*hq) {
        osIndex = 1;
        lastSampleRate = lastDownSampleRate * 4.0;
    } else {
        osIndex = 0;
        lastSampleRate = lastDownSampleRate;
    }
}

void MaximizerAudioProcessor::parameterChanged(const String &parameterID, float)
{
    if (parameterID == "hq" || parameterID == "renderHQ" ||
        parameterID == "linearPhase") {
        updateOversample();

        // CHANGE: We probably don't need to worry about resizing at any point
        // in the plugin's runtime other than init. We can just update the specs
        // here.
        // if (parameterID != "renderHQ")
        //     mbProc.need_resize = true;

        dsp::ProcessSpec newSpec{
            lastSampleRate,
            uint32(maxNumSamples * oversample[osIndex].getOversamplingFactor()),
            uint32(getTotalNumInputChannels())
        };

        mPi.prepare(newSpec);
        mbProc.setOversamplingFactor(oversample[osIndex].getOversamplingFactor());
        mbProc.reset();
        mbProc.prepare(newSpec);
    }

    /* flag crossover changes in linear-phase mode */
    if (parameterID.contains("crossover") && (bool)*linearPhase) {
        mbProc.crossover_changed_ID = parameterID.getTrailingIntValue();
        mbProc.updateCrossovers = true;
        // CHANGE: So if we want to manage a smooth changeover btw filter coeffs,
        // we have to just use the above flag and manage the filter change w/in
        // mbProc, where it'll swap some pointers
        updateBandCrossovers(mbProc.crossover_changed_ID);
    }
}

void MaximizerAudioProcessor::valueTreeRedirected(ValueTree &changed)
{
    auto child = changed.getChildWithProperty("id", "numBands");
    if (child.isValid()) {
        int num = child.getProperty("value");
        if (num != numBands)
            updateNumBands(num);
    }
}

void MaximizerAudioProcessor::updateNumBands(int newNumBands)
{
    numBands = newNumBands;

    suspendProcessing(true);
    mbProc.updateAllCrossovers(numBands);
    suspendProcessing(false);

    /*write numBands change to tree state*/
    auto val = apvts.state.getChildWithProperty("id", "numBands");
    val.setProperty("value", numBands, nullptr);

    if (onPresetChange && getActiveEditor())
        onPresetChange();
}

void MaximizerAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                           juce::MidiBuffer &midiMessages)
{
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
    auto &outBlock = context.getOutputBlock();

    mixer.pushDrySamples(dsp::AudioBlock<float>(buffer));

    float gain_raw = pow(10.f, (*gain_dB / 20.f));
    float output_raw = pow(10.f, (*output_dB / 20.f));

    bool gainRamped = false;
    if (gain_raw != lastInputGain) {
        gainRamped = true;
        m_lastGain = lastInputGain;
        buffer.applyGainRamp(0, buffer.getNumSamples(), lastInputGain,
                             gain_raw);
        lastInputGain = gain_raw;
    } else
        buffer.applyGain(gain_raw);

    bool boostRamped = false;
    if (*boost) {
        if (*boost != lastBoost)
            buffer.applyGainRamp(0, numSamples, 1.f, 3.98f);
        else
            buffer.applyGain(3.98f);
    } else if (*boost != lastBoost)
        buffer.applyGainRamp(0, numSamples, 3.98f, 1.f);
    lastBoost = (bool)*boost;

    inputMeter.copyBuffer(buffer);

    dsp::AudioBlock<float> osBlock;
    if (totalNumOutputChannels > 1)
        osBlock = oversample[osIndex].processSamplesUp(
            dsp::AudioBlock<float>(buffer));
    else
        osBlock = oversampleMono[osIndex].processSamplesUp(
            dsp::AudioBlock<float>(buffer));

    processDCOffset(osBlock);

    if (!*bandSplit)
        mPi.process(osBlock);
    else if (*bandSplit) {
        mbProc.processBands(osBlock);
    }

    if (totalNumOutputChannels > 1)
        oversample[osIndex].processSamplesDown(outBlock);
    else
        oversampleMono[osIndex].processSamplesDown(outBlock);

    processDCBlock(block);

    if (*width != 1.0 && totalNumOutputChannels > 1) {
        if (*monoWidth && *bandSplit &&
            (*mbProc.bandWidth[0] > 1.f || *mbProc.bandWidth[1] > 1.f ||
             *mbProc.bandWidth[2] > 1.f || *mbProc.bandWidth[3] > 1.f))
            widener.widenBuffer(
                buffer, *width,
                false); /*regular stereo widen if using mono band wideners*/
        else if (totalNumInputChannels < 2)
            widener.widenBuffer(
                buffer, *width,
                true); /*if mono->stereo channel config, mono widener*/
        else
            widener.widenBuffer(buffer, *width, *monoWidth); /*regular*/
    }

    int totalLatency = 0;

    if (*linearPhase && *bandSplit)
        totalLatency = osIndex > 1 ? 573 : 2047;
    else
        totalLatency = oversample[osIndex].getLatencyInSamples();

    setLatencySamples(totalLatency);

    bypassDelay.setDelay(totalLatency);

    mixer.setWetLatency(totalLatency);

    if (*autoGain) {
        if (gainRamped)
            buffer.applyGainRamp(0, numSamples, 1.0 / (m_lastGain * halfPi),
                                 1.0 / (gain_raw * halfPi));
        else
            buffer.applyGain(1.0 / (gain_raw * halfPi));

        if ((ClipType)clip->load() == ClipType::Deep)
            buffer.applyGain(halfPi * 0.794);
        else if ((ClipType)clip->load() == ClipType::Warm)
            buffer.applyGain(halfPi);

        if (*boost) {
            if (boostRamped)
                buffer.applyGainRamp(0, numSamples, 1.f, 1.f / 3.98f);
            else
                buffer.applyGain(1.f / 3.98f);
        } else if (!*boost && boostRamped)
            buffer.applyGainRamp(0, numSamples, 1.f / 3.98f, 1.f);
    }

    if (output_raw != lastOutGain) {
        buffer.applyGainRamp(0, numSamples, lastOutGain, output_raw);
        lastOutGain = output_raw;
    } else
        buffer.applyGain(output_raw);

    outputMeter.copyBuffer(buffer);

    mixer.setWetMixProportion(*mix);
    mixer.mixWetSamples(dsp::AudioBlock<float>(buffer));

    if (totalNumInputChannels < totalNumOutputChannels) {
        if (*width <= 1.0 &&
            (*mbProc.bandWidth[0] <= 1.0 && *mbProc.bandWidth[1] <= 1.0 &&
             *mbProc.bandWidth[2] <= 1.0 && *mbProc.bandWidth[3] <= 1.0))
            // copy left->right if no wideners are active & mono->stereo
            buffer.copyFrom(1, 0, buffer.getReadPointer(0), numSamples);
    }

    if (*delta)
        processDelta(buffer, gain_raw * halfPi, output_raw);
    if (*bypass || lastBypass)
        processBlockBypassed(buffer, midiMessages);

    /*mute buffer if it's over ~15dB*/
    if (buffer.getMagnitude(0, numSamples) >= 6.f)
        buffer.clear();
}

void MaximizerAudioProcessor::processBlockBypassed(AudioBuffer<float> &buffer,
                                                   MidiBuffer &)
{
    if (!bufferCopied)
        return;

    bool byp = (bool)*bypass;

    /*push to our delay buffer & read delayed samples*/
    if (getTotalNumInputChannels() <
        getTotalNumOutputChannels()) /*mono->stereo*/
    {
        auto in = bypassBuffer.getWritePointer(0);
        auto in_1 = bypassBuffer.getWritePointer(1);
        for (int i = 0; i < bypassBuffer.getNumSamples(); ++i) {
            bypassDelay.pushSample(0, in[i]);
            in[i] = bypassDelay.popSample(0);
        }
        FloatVectorOperations::copy(in_1, in, bypassBuffer.getNumSamples());
    } else { /*everything else*/
        for (int ch = 0; ch < bypassBuffer.getNumChannels(); ++ch) {
            auto in = bypassBuffer.getWritePointer(ch);
            for (int i = 0; i < bypassBuffer.getNumSamples(); ++i) {
                bypassDelay.pushSample(ch, in[i]);
                in[i] = bypassDelay.popSample(ch);
            }
        }
    }

    /*copy as usual if no change*/
    if (byp && lastBypass)
        buffer.makeCopyOf(bypassBuffer, true);
    /*fade bypass in*/
    else if (byp && !lastBypass) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            buffer.applyGainRamp(ch, 0, buffer.getNumSamples(), 1.0, 0.0);
            buffer.addFromWithRamp(ch, 0, bypassBuffer.getReadPointer(ch),
                                   buffer.getNumSamples(), 0.0, 1.0);
        }
    }
    /*fade bypass out*/
    else if (!byp && lastBypass) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            buffer.applyGainRamp(ch, 0, buffer.getNumSamples(), 0.0, 1.0);
            buffer.addFromWithRamp(ch, 0, bypassBuffer.getReadPointer(ch),
                                   buffer.getNumSamples(), 1.0, 0.0);
        }
    }

    lastBypass = byp;
    bufferCopied = false;
}

void MaximizerAudioProcessor::processDCOffset(dsp::AudioBlock<float> &block)
{
    bool t = (bool)distType->load();

    switch (distState) {
    case Asym:
    case Sym:
        if (t != lastAsym) {
            for (auto &s : smoothOffset)
                s.setTargetValue(t ? 0.1f : 0.f);
            distState = t ? SymToAsym : AsymToSym;
        }
        break;
    case SymToAsym:
        if (!smoothOffset[0].isSmoothing() && !smoothOffset[1].isSmoothing() &&
            t)
            distState = Asym;
        // else
        // {
        //     for (auto &s : smoothOffset)
        //         s.setTargetValue(t ? 0.1f : 0.f);
        //     distState = t ? SymToAsym : AsymToSym;
        // }
        break;
    case AsymToSym:
        if (!smoothOffset[0].isSmoothing() && !smoothOffset[1].isSmoothing() &&
            !t)
            distState = Sym;
        // else
        // {
        //     for (auto &s : smoothOffset)
        //         s.setTargetValue(t ? 0.1f : 0.f);
        //     distState = t ? SymToAsym : AsymToSym;
        // }
        break;
    }

    // apply DC offset
    for (size_t ch = 0; ch < block.getNumChannels(); ++ch) {
        auto in = block.getChannelPointer(ch);
        for (size_t i = 0; i < block.getNumSamples(); ++i) {
            in[i] += smoothOffset[ch].getNextValue();
        }
    }

    lastAsym = t;
}

void MaximizerAudioProcessor::processDCBlock(dsp::AudioBlock<float> &block)
{
    if (distState != Sym) {
        const double r = 1.0 - (1.0 / 1000.0);

        for (int channel = 0; channel < block.getNumChannels(); ++channel) {
            auto in = block.getChannelPointer(channel);

            for (int i = 0; i < numSamples; ++i) {
                /*DC blocker*/
                const auto yn = in[i];
                in[i] = yn - xm1[channel] + r * ym1[channel];
                xm1[channel] = yn;
                ym1[channel] = in[i];
            }
        }
    }
}

void MaximizerAudioProcessor::processDelta(AudioBuffer<float> &buffer,
                                           float inGain, float outGain)
{
    if (getTotalNumInputChannels() < getTotalNumOutputChannels()) {
        auto *data = buffer.getWritePointer(0);
        auto *dryData = bypassBuffer.getWritePointer(0);

        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            bypassDelay.pushSample(0, dryData[i]);
            dryData[i] = bypassDelay.popSample(0);
            if (!*autoGain) {
                data[i] = ((data[i] / inGain) / outGain) - dryData[i];
                // buffer.setSample(1, i, data[i]);
            } else {
                data[i] = (data[i] / outGain) - dryData[i];
                // buffer.setSample(1, i, data[i]);
            }
        }

        buffer.copyFrom(1, 0, data, buffer.getNumSamples());
    } else {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            auto *data = buffer.getWritePointer(ch);
            auto *dryData = bypassBuffer.getWritePointer(ch);

            for (int i = 0; i < buffer.getNumSamples(); ++i) {
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

/*only needed for when linear-phase crossovers are the priority*/
void MaximizerAudioProcessor::updateBandCrossovers(int crossover_changed_id)
{
    mbProc.updateCrossoverLin(crossover_changed_id);
    mbProc.updateCrossoverNonLin(crossover_changed_id);
}

//==============================================================================
bool MaximizerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *MaximizerAudioProcessor::createEditor()
{
    return new MaximizerAudioProcessorEditor(*this);
}

AudioProcessorEditor *MaximizerAudioProcessor::getEditor() const noexcept
{
    return dynamic_cast<AudioProcessorEditor *>(getActiveEditor());
}

//==============================================================================
void MaximizerAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    auto state = apvts.copyState();
    auto xml = state.createXml();
    xml->setAttribute("numBands", numBands);
    xml->setAttribute("preset", currentPreset);
    copyXmlToBinary(*xml, destData);
}

void MaximizerAudioProcessor::setStateInformation(const void *data,
                                                  int sizeInBytes)
{
    auto xmlState = getXmlFromBinary(data, sizeInBytes);
    if (xmlState.get() != nullptr) {
        numBands = xmlState->getIntAttribute("numBands", numBands);
        if (numBands < 1)
            numBands = 1;
        currentPreset = xmlState->getStringAttribute("preset");
        apvts.replaceState(ValueTree::fromXml(*xmlState));
        auto numBandsTree = apvts.state.getChildWithProperty("id", "numBands");
        if (numBandsTree.isValid())
            updateNumBands(numBandsTree.getProperty("value"));
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new MaximizerAudioProcessor();
}

AudioProcessorValueTreeState::ParameterLayout
MaximizerAudioProcessor::createParams()
{
    NormalisableRange<float> curveRange(0.8f, 1.2f, 0.001f, 1.f);
    curveRange.setSkewForCentre(1.0);
    NormalisableRange<float> bandRange(40.f, 18000.f, 0.01f);
    NormalisableRange<float> widthRange(0.f, 2.f, 0.01f);
    widthRange.setSkewForCentre(1.0);
    NormalisableRange<float> mixRange(0.f, 1.f, 0.01f);

    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.emplace_back(std::make_unique<strix::FloatParameter>(
        ParameterID("gain", 1), "Input Gain", -12.0, 12.0, 0.0));
    params.emplace_back(std::make_unique<strix::FloatParameter>(
        ParameterID("output", 1), "Output Gain", -12.0, 0.0, -1.0));
    params.emplace_back(std::make_unique<strix::FloatParameter>(
        ParameterID("curve", 1), "Curve", curveRange, 1.f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction([curveRange](float value, int) {
                float curve =
                    jmap(curveRange.convertTo0to1(value), -100.f, 100.f);
                if (fabs(curve) >= 9.999)
                    return String(roundToInt(curve));
                else
                    return String(curve, 1);
            })
            .withValueFromStringFunction([curveRange](const String &str) {
                return jmap(str.getFloatValue(), -100.f, 100.f,
                            curveRange.start, curveRange.end);
            })));
    params.emplace_back(std::make_unique<AudioParameterChoice>(
        ParameterID("distType", 1), "Saturation Type",
        StringArray("Symmetric", "Asymmetric"), 0));
    params.emplace_back(std::make_unique<AudioParameterChoice>(
        ParameterID("clipType", 1), "Saturation Limit",
        StringArray("Finite", "Clip", "Infinite", "Deep", "Warm"), 0));
    params.emplace_back(std::make_unique<AudioParameterBool>(
        ParameterID("bandSplit", 1), "Band Split On/Off", false));

    /*create 3 crossover freq params*/
    for (int i = 0; i < 3; ++i) {
        params.emplace_back(std::make_unique<strix::FloatParameter>(
            ParameterID("crossover" + std::to_string(i), 1),
            "Crossover " + std::to_string(i + 1) + " Freq", bandRange,
            i == 0   ? 150.0
            : i == 1 ? 2500.0
                     : 40.0,
            AudioParameterFloatAttributes()
                .withStringFromValueFunction([](float value, int) {
                    float cutoff = (int)value;
                    return String(cutoff, 0);
                })
                .withValueFromStringFunction([](const String &text) {
                    return text.contains("k") || text.contains("K")
                               ? text.getFloatValue() * 1000.f
                               : text.getFloatValue();
                })));
    }
    /*create params for solo/mono/bypass for each band*/
    for (int i = 0; i < 4; ++i) {
        params.emplace_back(std::make_unique<strix::FloatParameter>(
            ParameterID("bandInGain" + std::to_string(i), 1),
            "Band " + std::to_string(i + 1) + " Input Gain", -12.0, 12.0, 0.0));
        params.emplace_back(std::make_unique<strix::FloatParameter>(
            ParameterID("bandOutGain" + std::to_string(i), 1),
            "Band " + std::to_string(i + 1) + " Output Gain", -12.0, 12.0,
            0.0));
        params.emplace_back(std::make_unique<strix::FloatParameter>(
            ParameterID("bandWidth" + std::to_string(i), 1),
            "Band " + std::to_string(i + 1) + " Width", widthRange, 1.0,
            AudioParameterFloatAttributes()
                .withStringFromValueFunction([widthRange](float value, int) {
                    float percentage =
                        jmap(widthRange.convertTo0to1(value), 0.f, 200.f);

                    return String(percentage, 0);
                })
                .withValueFromStringFunction([widthRange](const String &text) {
                    return text.getFloatValue() / 100.f;
                })));
        params.emplace_back(std::make_unique<AudioParameterBool>(
            ParameterID("soloBand" + std::to_string(i), 1),
            "Solo Band " + std::to_string(i + 1), false));
        params.emplace_back(std::make_unique<AudioParameterBool>(
            ParameterID("muteBand" + std::to_string(i), 1),
            "Mute Band " + std::to_string(i + 1), false));
        params.emplace_back(std::make_unique<AudioParameterBool>(
            ParameterID("bypassBand" + std::to_string(i), 1),
            "Bypass Band " + std::to_string(i + 1), false));
    }

    params.emplace_back(std::make_unique<AudioParameterBool>(
        ParameterID("hq", 1), "HQ", false));
    params.emplace_back(std::make_unique<AudioParameterBool>(
        ParameterID("renderHQ", 1), "Render HQ", false));
    params.emplace_back(std::make_unique<AudioParameterBool>(
        ParameterID("linearPhase", 1), "Linear Phase", false));
    params.emplace_back(std::make_unique<strix::FloatParameter>(
        ParameterID("width", 1), "Width", widthRange, 1.f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction([widthRange](float value, int) {
                float percentage =
                    jmap(widthRange.convertTo0to1(value), 0.f, 200.f);

                return String(percentage, 0);
            })
            .withValueFromStringFunction([widthRange](const String &text) {
                return text.getFloatValue() / 100.f;
            })));
    params.emplace_back(std::make_unique<AudioParameterBool>(
        ParameterID("monoWidth", 1), "Widen Mono", false));
    params.emplace_back(std::make_unique<strix::FloatParameter>(
        ParameterID("mix", 1), "Dry/Wet Mix", mixRange, 1.f,
        AudioParameterFloatAttributes()
            .withStringFromValueFunction([](float value, int) {
                float percentage = jmap(value, 0.f, 100.f);

                return String(percentage, 0);
            })
            .withValueFromStringFunction([widthRange](const String &text) {
                return text.getFloatValue() / 100.f;
            })));
    params.emplace_back(std::make_unique<AudioParameterBool>(
        ParameterID("autoGain", 1), "Auto Gain", false));
    params.emplace_back(std::make_unique<AudioParameterBool>(
        ParameterID("boost", 1), "Input Boost", false));
    params.emplace_back(std::make_unique<AudioParameterBool>(
        ParameterID("bypass", 1), "Bypass", false));
    params.emplace_back(std::make_unique<AudioParameterBool>(
        ParameterID("delta", 1), "Delta", false));

    return {params.begin(), params.end()};
}

void MaximizerAudioProcessor::checkActivation()
{
    PluginHostType host;
    File dir = File(File::getSpecialLocation(File::userApplicationDataDirectory)
                        .getFullPathName() +
                    "/Arboreal Audio/PiMax/License/license.aal");

    // auto uuid =
    // String(OnlineUnlockStatus::MachineIDUtilities::getLocalMachineIDs().strings[0].hashCode64());
    // auto newID = String(SystemStats::getUniqueDeviceID().hashCode64());

#if JUCE_WINDOWS
    String timeFile =
        "HKEY_CURRENT_USER\\SOFTWARE\\Arboreal Audio\\PiMax\\TrialKey";
#elif JUCE_MAC
    File timeFile =
        File(File::getSpecialLocation(File::userApplicationDataDirectory)
                 .getFullPathName() +
             "/Arboreal Audio/PiMax/License/trialkey.aal");
#elif JUCE_LINUX
    File timeFile =
        File(File::getSpecialLocation(File::userApplicationDataDirectory)
                 .getFullPathName() +
             "/Arboreal Audio/PiMax/License/trialkey.aal");
#endif
    if (dir.exists() && !checkUnlock()) {
        auto xml = parseXML(dir);
        isUnlocked = (xml->hasAttribute("uuid") &&
                      xml->getStringAttribute("uuid").isNotEmpty());
        if (xml->hasAttribute("key"))
            isUnlocked = (xml->getStringAttribute("key") != "" &&
                          xml->getStringAttribute("key") != " ");
    }
#if JUCE_WINDOWS
    /*set trial key if non-existant*/
    if (!WindowsRegistry::valueExists(timeFile)) {
        auto trialStart = Time::getCurrentTime();
        WindowsRegistry::setValue(timeFile,
                                  String(trialStart.toMilliseconds()));
        trialRemaining_ms = RelativeTime::days(7).inMilliseconds();
    } else {
        auto trialEnd =
            Time(WindowsRegistry::getValue(timeFile).getLargeIntValue());
        trialEnd += RelativeTime::days(7);
        trialEnded = (trialEnd <= Time::getCurrentTime());
        if (!trialEnded)
            trialRemaining_ms = trialEnd.toMilliseconds() -
                                Time::getCurrentTime().toMilliseconds();
    }
#elif JUCE_MAC || JUCE_LINUX
    if (!timeFile.exists()) {
        timeFile.create();
        auto trialStart = Time::getCurrentTime();
        XmlElement xml{"TrialKey"};
        xml.setAttribute("key", String(trialStart.toMilliseconds()));
        xml.writeTo(timeFile);
        timeFile.setReadOnly(true);
        trialRemaining_ms = RelativeTime::days(7).inMilliseconds();
    } else {
        auto xml = parseXML(timeFile);

        auto trialEnd = Time(xml->getStringAttribute("key").getLargeIntValue());
        trialEnd += RelativeTime::days(7);
        trialEnded = (trialEnd <= Time::getCurrentTime());
        if (!trialEnded)
            trialRemaining_ms = trialEnd.toMilliseconds() -
                                Time::getCurrentTime().toMilliseconds();
    }
#endif
}
