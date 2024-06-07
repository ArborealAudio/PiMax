
#pragma once

#include <JuceHeader.h>
#define CONFIG_PATH "/Arboreal Audio/PiMax/config.xml"
#include "LookAndFeel.h"
#include "WaveshaperComponent.h"
#include "ResponseCurveComponent.h"
#include "Splash.h"
#include "Menu.h"
#include "../Presets/PresetComp.h"

//==============================================================================
class UI : public juce::Component
{
public:
    //==============================================================================
    UI(MaximizerAudioProcessor &);
    ~UI() override;

    //==============================================================================
    struct WidthSlider : public Slider
    {
        bool altDown = false;

        std::function<void()> onAltClick;

        void mouseDown(const MouseEvent &event) override
        {
            if (ModifierKeys::currentModifiers.isAltDown())
            {
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

        void mouseDrag(const MouseEvent &event) override
        {
            event.source.enableUnboundedMouseMovement(true);
            Slider::mouseDrag(event);
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

        void mouseDrag(const MouseEvent &event) override
        {
            event.source.enableUnboundedMouseMovement(true);
            Slider::mouseDrag(event);
        }

    private:
        bool textOnLeft = false;
    };

    String getCurrentPreset() noexcept
    {
        return presetComp.getCurrentPreset();
    }

    void paint(juce::Graphics &g) override;
    void resized() override;

    GainSlider gain__slider, outVol__slider;

    WidthSlider widthSlider, mixSlider;
    TextButton distButton, bandSplit__textButton, linearPhaseButton, hq, renderHQ, autoGain, bypass, boost;
    ComboBox clipBox;
    PresetComp presetComp;

    GainSliderLNF inGainLNF, outGainLNF;
    ComboBoxLNF hqBoxLNF, clipBoxLNF;
    BottomButtonLNF bottomButtonLNF;
    TopButtonLNF distLNF, bandSplitLNF, autoGainLNF, bypassLNF, boostLNF;

    KnobLNF widthLNF, mixLNF;
    strix::VolumeMeterComponent inputMeter, outputMeter;

private:
    MaximizerAudioProcessor &audioProcessor;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UI)
};

