// WaveshaperComponent.h

#pragma once
#include <JuceHeader.h>
#include "LookAndFeel.h"
#include "../PluginProcessor.h"

struct WaveshaperComponent : Component,
                             Timer,
                             AudioProcessorValueTreeState::Listener
{
    WaveshaperComponent(MaximizerAudioProcessor&);
    ~WaveshaperComponent() override;
    void timerCallback() override;
    void parameterChanged(const String& parameterID, float newValue) override;
    void paint(Graphics& g) override;

private:
    MaximizerAudioProcessor& audioProcessor;

    std::atomic<bool> paramChanged = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveshaperComponent)
};