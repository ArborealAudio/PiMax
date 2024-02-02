/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#define USE_CONVOLUTION 1

enum class ClipType
{
    Finite,
    Clip,
    Infinite,
    Deep,
    Warm
};

#include <JuceHeader.h>
#include <clap-juce-extensions/clap-juce-extensions.h>
#include "Processors/MaximPizer.h"
#include "Processors/Waveshapers.h"
#include "Processors/StereoWidener.h"
#include "Processors/MultiBandProcessor.h"
#include "Presets/PresetManager.h"

//==============================================================================
/**
*/
class MaximizerAudioProcessor : public AudioProcessor,
                                public AudioProcessorValueTreeState::Listener,
                                public clap_juce_extensions::clap_properties,
                                public clap_juce_extensions::clap_juce_audio_processor_capabilities,
                                private ValueTree::Listener
{
public:
    //==============================================================================
    MaximizerAudioProcessor();
    ~MaximizerAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

//#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
//#endif
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed(AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    AudioProcessorEditor* getEditor() const noexcept;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    void parameterChanged(const String& parameterID, float newValue) override;
    void valueTreeRedirected(ValueTree& treeWhichHasBeenChanged) override;
    std::function<void()> onPresetChange;

    inline void updateOversample() noexcept;

    void updateNumBands(int newNumBands);

    var checkUnlock()
    {
        return isUnlocked;
    }

    strix::VolumeMeterSource &getInputMeterSource() { return inputMeter; }
    strix::VolumeMeterSource &getOutputMeterSource() { return outputMeter; }

    AudioProcessorValueTreeState apvts;

    std::atomic<float>* bandSplit, *monoWidth, *delta, *clip, *distType, *autoGain, *linearPhase;
    strix::FloatParameter *gain_dB, *curve, *output_dB;

    int numBands = 2, lastNumBands = 2;

    double lastDownSampleRate = 44100.0;

    std::array<dsp::Oversampling<float>, 3> oversample
    { {
        {dsp::Oversampling<float>(2)},
        {dsp::Oversampling<float>(2, 2, dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR, false, true)},
        {dsp::Oversampling<float>(2, 2, dsp::Oversampling<float>::FilterType::filterHalfBandFIREquiripple, true, true)}
    } };

    std::array<dsp::Oversampling<float>, 3> oversampleMono
    { {
        {dsp::Oversampling<float>(1)},
        {dsp::Oversampling<float>(1, 2, dsp::Oversampling<float>::FilterType::filterHalfBandPolyphaseIIR, false, true)},
        {dsp::Oversampling<float>(1, 2, dsp::Oversampling<float>::FilterType::filterHalfBandFIREquiripple, true, true)}
    } };

    size_t osIndex = 0;

    int numSamples = 0, maxNumSamples = 0;

    String currentPreset = "Default";

    var isUnlocked = false;
    var trialEnded = false;
    int64 trialRemaining_ms = 0;

    bool hasUpdated = false;

private:

    void checkActivation();

    SmoothedValue<float> smoothOffset[2];
    void processDCOffset(dsp::AudioBlock<float> &);
    void processDCBlock(dsp::AudioBlock<float> &);
    // DC block coeffs
    double xm1[2]{ 0.0 };
    double ym1[2]{ 0.0 };
    
    enum
    {
        SymToAsym,
        AsymToSym,
        Sym,
        Asym
    } distState;

    void processDelta(AudioBuffer<float>&, float, float);

    void updateBandCrossovers(int);

    AudioProcessorValueTreeState::ParameterLayout createParams();

    ValueTree numBandsTree
    { "PARAM", {{"id", "numBands"}, {"value", numBands}} };

    std::unique_ptr<PresetManager> manager = std::make_unique<PresetManager>(apvts);

    MaximPizer mPi;

    StereoWidener<float> widener;

    MultibandProcessor mbProc;
    SpinLock mutex;

    int filterLength = 0;
    dsp::DryWetMixer<float> mixer;

    double lastSampleRate = 44100.0;

    float m_lastGain = 1.0;

    std::atomic<float>* hq, *renderHQ, *bypass, *boost;
    strix::FloatParameter *width, *mix;

    float lastInputGain = 1.0, lastOutGain = 1.0;

    bool lastBoost = false, lastAsym = false;

    strix::VolumeMeterSource inputMeter, outputMeter;

    AudioBuffer<float> bypassBuffer;
    bool lastBypass = false;
    bool bufferCopied = false;

    dsp::DelayLine<float> bypassDelay {44100};

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MaximizerAudioProcessor)
};
