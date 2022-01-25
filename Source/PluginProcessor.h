/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#define USE_SIMD_SAT 0
#define USE_SIMD_FIR 0
#define USE_CONVOLUTION 1

#include <JuceHeader.h>
#include "Processors/MaximPizer.h"
#include "Processors/Waveshapers.h"
#include "Processors/StereoWidener.h"
#include "Processors/MultiBand Processor.h"
#include "ff_meters-master/ff_meters.h"
#include "Presets/PresetManager.h"
#include "OnlineActivation.h"

#ifndef NDEBUG
#define X(textToWrite)                                                         \
  JUCE_BLOCK_WITH_FORCED_SEMICOLON(juce::String tempDbgBuf;                    \
                                   tempDbgBuf << textToWrite;                  \
                                   juce::Logger::writeToLog(tempDbgBuf);)

#define X_EVERY(n, textToWrite)                                                \
  {                                                                            \
    static int j = 0;                                                          \
    if ((j % static_cast<int>(n)) == 0) {                                      \
      X(textToWrite);                                                          \
    }                                                                          \
    ++j;                                                                       \
  }
#endif

//==============================================================================
/**
*/
class MaximizerAudioProcessor : public AudioProcessor,
                                public AudioProcessorValueTreeState::Listener,
                                public ValueTree::Listener,
                                HighResolutionTimer
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
    void hiResTimerCallback() override;
    void valueTreeRedirected(ValueTree& treeWhichHasBeenChanged) override;
    std::function<void()> onPresetChange;

    inline void updateOversample() noexcept;

    void updateNumBands(int newNumBands) noexcept;

    inline var checkUnlock()
    {
        return isUnlocked;
    }

    foleys::LevelMeterSource& getInputMeterSource() { return inputMeter; }
    foleys::LevelMeterSource& getOutputMeterSource() { return outputMeter; }

    AudioProcessorValueTreeState apvts;

    std::atomic<float>* bandSplit, *monoWidth, *delta,
        *gain_dB, *curve, *output_dB, *clip, *distIndex, *autoGain, *linearPhase;
    std::array<std::atomic<float>*, 3> crossovers;

    int numBands = 2, lastNumBands = 2;

    double lastDownSampleRate = 0.0;

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

    int osIndex = 0;

    int numSamples = 0;

    int lastUIWidth, lastUIHeight;

    String currentPreset = "Default";

    var isUnlocked = false;
    var trialEnded = false;
    int64 trialRemaining_ms = 0;

    bool hasUpdated = false;
    std::function<void(bool)> updateStatus = [this](bool newUpdate)
    {
        hasUpdated = newUpdate;
    };

private:

    void checkActivation();

    void processDelta(AudioBuffer<float>& buffer, float inGain, float outGain);

    AudioProcessorValueTreeState::ParameterLayout createParams();

    ValueTree numBandsTree
    { "PARAM", {{"id", "numBands"}, {"value", numBands}} };
    
    std::unique_ptr<PresetManager> manager = std::make_unique<PresetManager>(apvts);

    MaximPizer mPi;
    double xm1[2]{ 0.0, 0.0 };
    double ym1[2]{ 0.0, 0.0 };

    StereoWidener widener;

    MultibandProcessor m_Proc;

    int filterLength = 0;
    dsp::DryWetMixer<float> mixer;

    double lastSampleRate = 0.0, lastK = 0.0;

    float m_lastGain = 0.0;

    std::atomic<float>* hq, *renderHQ, *width, *mix, *bypass, *boost;
    
    float lastInputGain = 0.0, lastOutGain = 0.0;

    foleys::LevelMeterSource inputMeter, outputMeter;
    
    AudioBuffer<float> bypassBuffer;
    bool lastBypass = false;
    bool bufferCopied = false;
    
    std::atomic<bool> needs_resize = false, crossover_changed = false;
    int crossover_changedID = 0;
    
    dsp::DelayLine<float> bypassDelay {44100};

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MaximizerAudioProcessor)
};
