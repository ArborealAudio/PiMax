/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI.h"
#include "gin/gin.h"
#include "UI/Splash.h"
#include "DownloadManager.h"

//==============================================================================
/**
*/

struct CrossoverSlider : Slider
{
    CrossoverSlider()
    {
        setSliderStyle(SliderStyle::LinearHorizontal);
        setSliderSnapsToMousePosition(false);
        setLookAndFeel(&sliderLNF);
    }

    ~CrossoverSlider() override
    {
        setLookAndFeel(nullptr);
    }

    bool hitTest(int x, int y) override
    {
        auto sliderBounds = getLocalBounds();

        Rectangle<float> sliderThumb{ float(getPositionOfValue(getValue())),
            float(sliderBounds.getBottom() - 125),
            3.0, float(sliderBounds.getHeight()) };
        Rectangle<float> textBox{ float(getPositionOfValue(getValue()) - 30), 35, 65, 20 };
        return sliderThumb.expanded(5).contains(x, y) || textBox.contains(x, y);
    }

    double proportionOfLengthToValue(double proportion) override
    {
        return mapToLog10(proportion, 25.0, 24000.0);
    }

    double valueToProportionOfLength(double value) override
    {
        return mapFromLog10(value, 25.0, 24000.0);
    }

    void mouseDown(const MouseEvent& e) override
    {
        if (ModifierKeys::currentModifiers.isRightButtonDown()) {
            wasRightClicked = true;
        }
        else
            Slider::mouseDown(e);
    }

    void mouseDrag(const MouseEvent& e) override
    {
        mouseDragging = true;
        Slider::mouseDrag(e);
    }

    void stoppedDragging() override
    {
        Slider::stoppedDragging();
        mouseDragging = false;
    }

    void mouseDoubleClick(const MouseEvent& event) override
    {
        if (ModifierKeys::currentModifiers.isRightButtonDown())
            return;
        else
            Slider::mouseDoubleClick(event);
    }

    bool wasRightClicked = false;
    bool mouseDragging = false;

private:

    CrossoverSliderLNF sliderLNF;
};

struct BandParamSlider : Slider
{
    BandParamSlider()
    {
        setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
        setSliderSnapsToMousePosition(false);
        setColour(TooltipWindow::textColourId, Colours::floralwhite);
        setLookAndFeel(&knobLNF);
    }

    ~BandParamSlider() override
    {
        setLookAndFeel(nullptr);
    }

    void setLabelType(KnobLNF::LabelType type)
    {
        knobLNF.setLabelType(type);
    }

    inline void setLNFImage(const Image& newImage)
    {
        knobLNF.setImage(newImage);
    }

    std::function<void()> onMouseOver;

    void mouseEnter(const MouseEvent& event) override
    {
        if (onMouseOver != nullptr && !painted) {
            painted = true;
            onMouseOver();
            Slider::mouseEnter(event);
        }
    }

    void mouseExit(const MouseEvent& event) override
    {
        painted = false;
        knobLNF.painted = false;
        Slider::mouseExit(event);
    }

private:
    KnobLNF knobLNF;

    bool painted = false;
};

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

struct ResponseCurveComponent : Component,
                                AudioProcessorValueTreeState::Listener,
                                Timer
{
    ResponseCurveComponent(MaximizerAudioProcessor&);
    ~ResponseCurveComponent() override;

    void parameterChanged(const String& parameterID, float newValue) override;
    void timerCallback() override;
    void paint(Graphics& g) override;
    inline void setSliderLimits(float slider0Pos, float slider1Pos,
        float slider2Pos) noexcept;
    inline void drawResponseCurve(Graphics& g, const Rectangle<float>& responseArea, float w) noexcept;
    inline void drawBandArea(Graphics& g, float slider0Pos, float slider1Pos, float slider2Pos,
        const Rectangle<float>& responseArea) noexcept;
    inline void drawBandParams(Graphics& g, float slider0Pos, float slider1Pos, float slider2Pos,
        float width) noexcept;
    inline void drawAddButton(Graphics& g, const Rectangle<float>& responseArea) noexcept;
    inline void setBand() noexcept;
    inline void removeBand(int index) noexcept;
    inline void update(int newNumBands) noexcept
    {
        numBands = newNumBands;
    }
    void resized() override;

    std::vector<std::unique_ptr<CrossoverSlider>> sliders;
    std::vector<std::unique_ptr<TextButton>> solo, mute, bypass;
    std::vector<std::unique_ptr<BandParamSlider>> bandInGain, bandOutGain, bandWidth;

private:
    MaximizerAudioProcessor& audioProcessor;

    TextButton addButton;
    
    bool sliderValueChanged = false, paintAddButton = true;

    std::vector<dsp::IIR::Filter<float>> lowPass, highPass;
    std::vector<dsp::IIR::Coefficients<float>> lowPassCoeffs, highPassCoeffs;
    std::atomic<bool> paramChanged = false;
    int numBands = 2;
    double sampleRate = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResponseCurveComponent)
};


class MaximizerAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    MaximizerAudioProcessorEditor (MaximizerAudioProcessor&);
    ~MaximizerAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    inline void updateBandDisplay(int newNumBands) noexcept
    {
        responseCurveComponent.update(newNumBands);
    }

private:
    OpenGLContext opengl;

    ResponseCurveComponent responseCurveComponent;
    UI ui;
    WaveshaperComponent waveshaperComponent;
    Splash splash;
    ActivationComponent activationComp;
    DownloadManager downloadManager;

    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> renderButtonAttach, hqAttachment,
        bandSplitAttachment, linearPhaseAttachment, distAttachment, autoGainAttach, bypassAttach;
    std::vector<std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment>> muteBandAttach, soloBandAttach, bypassBandAttach;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> gainAttachment, outVolAttachment, curveAttachment,
        widthAttachment, mixAttachment;
    std::vector<std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment>> sliderAttachment,
        bandInGainAttach, bandOutGainAttach, bandWidthAttach;
    std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment> clipAttachment;

    std::vector<Component*> getComps();

    TooltipWindow tooltip;
    
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    MaximizerAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MaximizerAudioProcessorEditor)
};
