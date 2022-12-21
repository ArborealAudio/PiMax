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

//[Headers] You can add your own extra header files here...
#include "../PluginEditor.h"
//[/Headers]

#include "UI.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
#define FONT getCustomFont(FontStyle::Regular)
//[/MiscUserDefs]

//==============================================================================
UI::UI (MaximizerAudioProcessor& p) : audioProcessor(p), gain__slider(false), outVol__slider(true),
                                      presetComp(p.apvts),
                                      inputMeter(p.getInputMeterSource()),
                                      outputMeter(p.getOutputMeterSource())
{
    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);

    inputMeter.setMeterType(strix::VolumeMeterComponent::Volume);
    inputMeter.setMeterColor(Colours::lightgreen.withAlpha(0.5f));
    inputMeter.setInterceptsMouseClicks(false, false);
    inputMeter.clipIndicator = false;

    outputMeter.setMeterType(strix::VolumeMeterComponent::Volume);
    outputMeter.setMeterColor(Colours::lightgreen.withAlpha(0.5f));
    outputMeter.setInterceptsMouseClicks(true, false);
    outputMeter.clipIndicator = true;

    inputMeter.setBounds(33, 91, 40, 218);
    outputMeter.setBounds(538, 91, 40, 216);

    gain__slider.setSliderStyle(juce::Slider::LinearVertical);
    inGainLNF.textOnLeft = false;
    gain__slider.setLookAndFeel(&inGainLNF);
    gain__slider.setPaintingIsUnclipped(true);
    gain__slider.setSliderSnapsToMousePosition(false);
    gain__slider.setTooltip("Sets the input gain. In band split mode, the gain is applied to the entire signal before the multiband process.");
    addAndMakeVisible(gain__slider);
    gain__slider.setBounds(33, 112, 100, 200);

    outVol__slider.setSliderStyle(juce::Slider::LinearVertical);
    outGainLNF.textOnLeft = true;
    outVol__slider.setLookAndFeel(&outGainLNF);
    outVol__slider.setPaintingIsUnclipped(true);
    outVol__slider.setSliderSnapsToMousePosition(false);
    outVol__slider.setTooltip("Output gain after all other processing, before the Mix blend.");
    addAndMakeVisible(outVol__slider);
    outVol__slider.setBounds(478, 110, 100, 200);

    addAndMakeVisible(autoGain);
    autoGain.setClickingTogglesState(true);
    autoGain.setLookAndFeel(&autoGainLNF);
    autoGainLNF.setType(TopButtonLNF::Type::Auto);
    autoGain.setButtonText("Auto");
    autoGain.setTooltip("Toggles automatic gain compensation for both the full signal and any multiband gain.");
    autoGain.setBounds(538, 50, 40, 25);

    addAndMakeVisible(boost);
    boost.setClickingTogglesState(true);
    boost.setLookAndFeel(&boostLNF);
    boostLNF.setType(TopButtonLNF::Type::Boost);
    boost.setButtonText("Input Boost");
    boost.setTooltip("12dB input boost, and increases Curve value exponentially.");
#if JUCE_WINDOWS
    boost.setSize(55, 23);
#else
    boost.setSize(60, 23);
#endif

    addAndMakeVisible(bypass);
    bypass.setClickingTogglesState(true);
    bypass.setLookAndFeel(&bypassLNF);
    bypassLNF.setType(TopButtonLNF::Type::Bypass);
    bypass.setButtonText("Byp");
    bypass.setTooltip("Bypasses the processing.");
    bypass.setBounds(538, 15, 40, 25);

    addAndMakeVisible(distButton);
    distButton.setClickingTogglesState(true);
    if (!distButton.getToggleState()) {
        distButton.setButtonText("symmetric");
    }
    else if (distButton.getToggleState()) {
        distButton.setButtonText("asymmetric");
    }
    distButton.onClick = [this]()
    {
        if (!distButton.getToggleState()) {
            distButton.setButtonText("symmetric");
        }
        else if (distButton.getToggleState()) {
            distButton.setButtonText("asymmetric");
        }
    };
    distButton.setTooltip("Symmetric: odd-order harmonics, and you can avoid 0 dbFS overshoots when processing the full signal.\n\n"
        "Asymmetric: even- and odd-order harmonics, a smoother-sounding saturation.");
    distLNF.setType(TopButtonLNF::Type::Toggle);
    distButton.setLookAndFeel(&distLNF);
    distButton.setBounds(30, 328, 90, 32);

    addAndMakeVisible(clipBox);
    clipBox.setLookAndFeel(&clipBoxLNF);
    clipBoxLNF.setType(ComboBoxLNF::Type::Clip);
    StringArray clipMode{ "finite", "clip", "infinite", "deep", "warm" };
    clipBox.addItemList(clipMode, 1);
    clipBox.setTooltip("Finite: saturation which folds back towards zero instead of soft clipping.\n\n"
        "Clip: clip at digital maximum, like traditional soft clipping. Good for transient-heavy material.\n\n"
        "Infinite: foldback saturation which folds through zero and towards the opposite pole, resulting in"
        " distortion which can affect the pitch of the sound.\n\n"
        "Deep: Highly colorful distortion that digs deep into the signal, with a gentle boost to the low-end and top-end.\n\n"
        "Warm: The subtlest saturation, with adjustable high-frequency absorption and low-end boosting.");
    clipBox.setBounds(122, 328, 60, 32);

    addAndMakeVisible(bandSplit__textButton);
    bandSplit__textButton.setClickingTogglesState(true);
    bandSplit__textButton.setButtonText("Band Split");
    bandSplitLNF.setType(TopButtonLNF::Type::Regular);
    bandSplit__textButton.setLookAndFeel(&bandSplitLNF);
    bandSplit__textButton.setTooltip("Splits the signal into up to four controllable frequency bands.\n\n"
        "Useful for when intermodulation distortion is an issue, such as processing a full mix with heavy"
        " low-end content.");
    bandSplit__textButton.setSize(88, 32);

    addAndMakeVisible(hq);
    hq.setClickingTogglesState(true);
    hq.setButtonText("HQ");
    hq.setTooltip("Enable 4x oversampling at the cost of latency and increased CPU.");
    hq.setLookAndFeel(&bottomButtonLNF);
    hq.setBounds(485, 374, 38, 24);

    addAndMakeVisible(renderHQ);
    renderHQ.setClickingTogglesState(true);
    renderHQ.setButtonText("Render HQ");
    renderHQ.setLookAndFeel(&bottomButtonLNF);
    renderHQ.setTooltip("Only use oversampling when rendering.");
    renderHQ.setBounds(523, 374, 60, 24);

    addAndMakeVisible(linearPhaseButton);
    linearPhaseButton.setButtonText("Linear Phase");
    linearPhaseButton.setLookAndFeel(&bottomButtonLNF);
    linearPhaseButton.setClickingTogglesState(true);
    linearPhaseButton.setTooltip("Sets linear phase for band split filters as well as the oversampling.\n\n"
        "Useful for using Band Split in a mastering context, or if you want to blend oversampled processing with the Mix knob.\n\n"
        "Both linear phase oversampling and band split will introduce more latency and CPU use.\n"
        "(Note: adjusting crossovers in linear phase mode can produce audible stuttering in the audio, so if"
        " you need to modulate the crossovers, it's best to stick to non-linear phase.)");
    linearPhaseButton.setBounds(405, 374, 80, 24);

    addAndMakeVisible(widthSlider);
    widthSlider.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
    widthSlider.setLookAndFeel(&widthLNF);
    widthLNF.setLabelType(KnobLNF::LabelType::Width);
    widthSlider.setSliderSnapsToMousePosition(false);
    widthSlider.setTooltip("Widens or narrows the stereo image.\n\n"
        "Alt / Option-click to toggle widening for a mono source.\n\n"
    "If widening individual bands of a mono source in Band Split mode, this will work in its regular stereo mode.");
    widthLNF.altDown = p.monoWidth;
    widthSlider.onAltClick = [&] { p.apvts.getParameterAsValue("monoWidth") = widthSlider.altDown; };
    widthSlider.setBounds(415, 300, 50, 75);

    addAndMakeVisible(mixSlider);
    mixSlider.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
    mixSlider.setLookAndFeel(&mixLNF);
    mixLNF.setLabelType(KnobLNF::LabelType::Mix);
    mixSlider.setSliderSnapsToMousePosition(false);
    mixSlider.setTooltip("Blends processed output with the dry input signal. Alt / Option-click to enable Delta mode,"
        " which subtracts the dry signal from the processed signal, allowing you to hear what PiMax is adding to the input signal.");
    mixLNF.altDown = p.delta;
    mixSlider.onAltClick = [&] { p.apvts.getParameterAsValue("delta") = mixSlider.altDown; };
    mixSlider.setBounds(480, 300, 50, 75);

    addAndMakeVisible(presetComp);
    presetComp.setLookAndFeel(new PopupLNF());
    presetComp.setCurrentPreset(p.currentPreset);
    presetComp.box.onChange = [&]
    {
        presetComp.valueChanged();
        p.currentPreset = getCurrentPreset();
    };
    presetComp.setBounds(35, 374, 185, 24);
    
    auto children = getChildren();
    for (auto child : children)
    {
        child->setTransform(AffineTransform::scale(1.2f));
    }
    setSize(720, 480);

    bandSplit__textButton.setCentrePosition(getLocalBounds().getCentreX(), 1.2 * 344.f);
    boost.setCentrePosition(getLocalBounds().getCentreX(), 1.2 * 80.f);
}

UI::~UI()
{
    gain__slider.setLookAndFeel(nullptr);
    outVol__slider.setLookAndFeel(nullptr);
    hq.setLookAndFeel(nullptr);
    renderHQ.setLookAndFeel(nullptr);
    clipBox.setLookAndFeel(nullptr);
    distButton.setLookAndFeel(nullptr);
    bandSplit__textButton.setLookAndFeel(nullptr);
    autoGain.setLookAndFeel(nullptr);
    bypass.setLookAndFeel(nullptr);
    boost.setLookAndFeel(nullptr);
    linearPhaseButton.setLookAndFeel(nullptr);
    widthSlider.setLookAndFeel(nullptr);
    mixSlider.setLookAndFeel(nullptr);
    presetComp.setLookAndFeel(nullptr);
}

//==============================================================================
void UI::paint (juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff30414d));

    ColourGradient gradient(Colour(0xa7a7a7a7).withAlpha(0.25f), getLocalBounds().getCentreX(),
        getLocalBounds().getCentreY(), Colours::transparentWhite, getX(), getY(), true);
    g.setGradientFill(gradient);
    g.fillAll();

    g.setColour(Colour(0xa7a7a7a7));
    g.setFont(getCustomFont(FontStyle::Bold).withHeight(60.f));
#if JUCE_WINDOWS
    int width = 122;
#else
    int width = 150;
#endif
    Rectangle<int> textBounds{ getLocalBounds().getCentreX() - (width / 2),
    10,
    width,
    50 };
    g.drawText("PiMax", textBounds, Justification::centred, false);
#if DEBUG
    g.setColour(Colours::red);
    g.setFont(20.f);
    g.drawText("DEBUG", textBounds.translated(0, 25), Justification::centred, false);
#elif !PRODUCTION_BUILD
    g.setColour(Colours::lime);
    g.setFont(20.f);
    g.drawText("DEV", textBounds.translated(0, 25), Justification::centred, false);
#endif

    {
        auto menuBounds = getLocalBounds().removeFromBottom(33).toFloat().reduced(1.5f);
        g.setColour(Colours::black);
        g.fillRoundedRectangle(menuBounds, 3.f);
        g.setColour(Colour(0xa7a7a7a7));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.5f), 7.f, 1.5f);
    }
}

void UI::resized()
{
}

//==============================================================================