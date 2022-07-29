/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 6.1.2

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2020 - Raw Material Software Limited.

  ==============================================================================
*/

#pragma once

//[Headers]     -- You can add your own extra header files here --
#include <JuceHeader.h>
#include "UI/LookAndFeel.h"
#include "Presets/PresetComp.h"
//[/Headers]


//==============================================================================
/**
    An auto-generated component, created by the Projucer.

    Describe your class and how it works here!
*/
class UI  : public juce::Component
{
public:
    //==============================================================================
    UI (MaximizerAudioProcessor&);
    ~UI() override;

    //==============================================================================
    struct WidthSlider : public Slider
    {
        bool altDown = false;

        std::function<void()> onAltClick;

        void mouseDown(const MouseEvent& event) override
        {
            if (ModifierKeys::currentModifiers.isAltDown()) {
                if (!altDown)
                    altDown = true;
                else
                    altDown = false;
                if (onAltClick != nullptr)
                    onAltClick();
            }
            else
                Slider::mouseDown(event);
        }
    };

    struct GainSlider : public Slider
    {
        GainSlider(bool textOnLeft) : textOnLeft(textOnLeft) {}

        bool hitTest(int x, int y) override
        {
            auto sliderBounds = getLocalBounds();

            if (!textOnLeft)
                sliderBounds = sliderBounds.removeFromLeft(40);
            else
                sliderBounds = sliderBounds.removeFromRight(40);

            Rectangle<float> textBox((textOnLeft ? 10 : 40),
                float(getPositionOfValue(getValue())), 46, 10);

            return sliderBounds.contains(x, y) || textBox.contains(x, y);
        }
    private:
        bool textOnLeft = false;
    };

    String getCurrentPreset() noexcept
    {
        return presetComp.getCurrentPreset();
    }

    void paint (juce::Graphics& g) override;
    void resized() override;

    // Binary resources:
    //static const char* g245_png;
    //static const int g245_pngSize;

    GainSlider gain__slider, outVol__slider;
    Slider curve__slider;
    WidthSlider widthSlider, mixSlider;
    TextButton distButton, bandSplit__textButton, linearPhaseButton, hq, renderHQ, autoGain, bypass, boost;
    ComboBox clipBox;
    PresetComp presetComp;

    // OutputMeterLNF outputLNF;
    // InputMeterLNF inputLNF;
    GainSliderLNF inGainLNF, outGainLNF;
    ComboBoxLNF hqBoxLNF, clipBoxLNF;
    BottomButtonLNF bottomButtonLNF;
    TopButtonLNF distLNF, bandSplitLNF, autoGainLNF, bypassLNF, boostLNF;
    CurveSliderLNF curveLNF;
    KnobLNF widthLNF, mixLNF;
    strix::VolumeMeterComponent inputMeter, outputMeter;
    // foleys::LevelMeter inputMeter{ foleys::LevelMeter::Minimal }, outputMeter{ foleys::LevelMeter::Minimal };
    

private:
    MaximizerAudioProcessor& audioProcessor;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (UI)
};