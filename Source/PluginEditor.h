/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "UI/UI.h"
#include "DownloadManager.h"

//==============================================================================
/**
 */

class MaximizerAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    MaximizerAudioProcessorEditor(MaximizerAudioProcessor &);
    ~MaximizerAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics &) override;
    void resized() override;
    bool hitTest(int x, int y) override
    {
        if (activationComp.isFormVisible())
        {
            if (activationComp.getBounds().transformedBy(activationComp.getTransform()).contains(x, y))
                return true;
            else
                return false;
        }
        else
            return AudioProcessorEditor::hitTest(x, y);
    }

    inline void updateBandDisplay(int newNumBands) noexcept
    {
        responseCurveComponent.update(newNumBands);
    }

    void resetWindowSize()
    {
        setSize(720, 480);
        writeUISize(720, 480);
    }

private:
    MaximizerAudioProcessor &audioProcessor;
#if JUCE_WINDOWS || JUCE_LINUX
    OpenGLContext opengl;
#endif

    ResponseCurveComponent responseCurveComponent;
    UI ui;
    WaveshaperComponent waveshaperComponent;
    Splash splash;
    ActivationComponent activationComp;
    DownloadManager downloadManager;

    Slider curve__slider;
    CurveSliderLNF curveLNF;

    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> renderButtonAttach, hqAttachment, bandSplitAttachment, linearPhaseAttachment, distAttachment, autoGainAttach, bypassAttach, boostAttach;
    std::vector<std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment>> muteBandAttach, soloBandAttach, bypassBandAttach;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> gainAttachment, outVolAttachment, curveAttachment, widthAttachment, mixAttachment;
    std::vector<std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment>> sliderAttachment, bandInGainAttach, bandOutGainAttach, bandWidthAttach;
    std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment> clipAttachment;

    std::vector<Component *> getComps();

    TopButtonLNF unlockLNF;
    TextButton unlockButton{"Unlock"};

    std::unique_ptr<TooltipWindow> tooltip = nullptr;

    std::unique_ptr<MenuComponent> menu = nullptr;

    void writeUISize(int width, int height)
    {
        auto uiSizeFile = File(File::getSpecialLocation(File::userApplicationDataDirectory).getFullPathName() + "/Arboreal Audio/PiMax/config.xml");
        if (!uiSizeFile.existsAsFile())
            uiSizeFile.create();

        width = jmin(width, 1440);
        height = jmin(height, 960);

        auto uiXml = parseXML(uiSizeFile);
        if (!uiXml)
            uiXml.reset(new XmlElement{"Config"});

        uiXml->setAttribute("uiWidth", width);
        uiXml->setAttribute("uiHeight", height);
        uiXml->writeTo(uiSizeFile);
#if JUCE_MAC
        auto gbFile = File("~/Music/Audio Music Apps/Arboreal Audio/PiMax/config.xml");
        if (!gbFile.existsAsFile())
            gbFile.create();
        uiXml->writeTo(gbFile);
#endif
    }

    void readUISize(int &width, int &height)
    {
        File uiSizeFile;
        PluginHostType host;
        if (!host.isGarageBand())
            uiSizeFile = File(File::getSpecialLocation(File::userApplicationDataDirectory).getFullPathName() + "/Arboreal Audio/PiMax/config.xml");
        else
            uiSizeFile = File("~/Music/Audio Music Apps/Arboreal Audio/PiMax/config.xml");

        if (uiSizeFile.existsAsFile())
        {
            auto xml = parseXML(uiSizeFile);
            if (xml->hasTagName("Config") || xml->hasTagName("UISize"))
            {
                width = xml->getIntAttribute("uiWidth", width);
                height = xml->getIntAttribute("uiHeight", height);
            }
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MaximizerAudioProcessorEditor)
};
