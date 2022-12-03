/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"

//==============================================================================
MaximizerAudioProcessorEditor::MaximizerAudioProcessorEditor(MaximizerAudioProcessor &p)
    : AudioProcessorEditor(&p),
      responseCurveComponent(p),
      audioProcessor(p),
      ui(p),
      waveshaperComponent(p),
      activationComp(p.isUnlocked, p.trialRemaining_ms),
      downloadManager(p.hasUpdated),
      tooltip(nullptr, 2000),
      menu(p.apvts)
{
    getLookAndFeel().setDefaultSansSerifTypeface(getCustomFont(FontStyle::Regular).getTypeface());

    tooltip.setColour(TooltipWindow::backgroundColourId, Colours::darkslategrey);

#if JUCE_WINDOWS || JUCE_LINUX
    opengl.setImageCacheSize((size_t)64 * 1024000);
    if (readConfigFile("openGL"))
    {
        opengl.detach();
        opengl.attachTo(*this);
    }
#endif

    menu.setAlwaysOnTop(true);
    menu.windowResizeCallback = [&]
    { resetWindowSize(); };
#if JUCE_WINDOWS || JUCE_LINUX
    menu.openGLCallback = [&](bool state)
    {
        if (state)
            opengl.attachTo(*this);
        else
            opengl.detach();
        DBG("OpenGL: " << (int)opengl.isAttached() << ", w/ cache size: " << opengl.getImageCacheSize());
        writeConfigFile("openGL", state);
    };
#endif

    for (auto *comp : getComps())
    {
        addAndMakeVisible(comp);
    }

    waveshaperComponent.setInterceptsMouseClicks(false, true);

    gainAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "gain",
                                                                                      ui.gain__slider);
    outVolAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "output",
                                                                                        ui.outVol__slider);
    curveAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "curve",
                                                                                       curve__slider);
    widthAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "width",
                                                                                       ui.widthSlider);
    clipAttachment = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment>(p.apvts,
                                                                                        "clipType", ui.clipBox);
    autoGainAttach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,
                                                                                      "autoGain", ui.autoGain);
    bypassAttach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,
                                                                                    "bypass", ui.bypass);
    distAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,
                                                                                      "distType", ui.distButton);
    bandSplitAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,
                                                                                           "bandSplit", ui.bandSplit__textButton);
    for (int i = 0; i < 3; ++i)
    {
        sliderAttachment.emplace_back(std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts,
                                                                                                       "crossover" + std::to_string(i), *responseCurveComponent.sliders[i]));
    }
    for (int i = 0; i < 4; ++i)
    {
        bandInGainAttach.emplace_back(std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts,
                                                                                                       "bandInGain" + std::to_string(i), *responseCurveComponent.bandInGain[i]));
        bandOutGainAttach.emplace_back(std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts,
                                                                                                        "bandOutGain" + std::to_string(i), *responseCurveComponent.bandOutGain[i]));
        bandWidthAttach.emplace_back(std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts,
                                                                                                      "bandWidth" + std::to_string(i), *responseCurveComponent.bandWidth[i]));
        soloBandAttach.emplace_back(std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,
                                                                                                     "soloBand" + std::to_string(i), *responseCurveComponent.solo[i]));
        muteBandAttach.emplace_back(std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,
                                                                                                     "muteBand" + std::to_string(i), *responseCurveComponent.mute[i]));
        bypassBandAttach.emplace_back(std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,
                                                                                                       "bypassBand" + std::to_string(i), *responseCurveComponent.bypass[i]));
    }
    hqAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "hq", ui.hq);
    renderButtonAttach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,
                                                                                          "renderHQ", ui.renderHQ);
    linearPhaseAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,
                                                                                             "linearPhase", ui.linearPhaseButton);
    mixAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "mix", ui.mixSlider);
    boostAttach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts, "boost", ui.boost);

    setResizable(true, true);
    getConstrainer()->setMinimumSize(450, 300);
    getConstrainer()->setFixedAspectRatio(1.5);

    int width = 720, height = 480;

    readUISize(width, height);

    setSize(width, height);

    curve__slider.setSliderStyle(juce::Slider::LinearHorizontal);
    curve__slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    curve__slider.setLookAndFeel(&curveLNF);
    curve__slider.setSliderSnapsToMousePosition(false);
    curve__slider.setPopupDisplayEnabled(true, true, this, 2000);
    curve__slider.setTooltip("In Finite, Clip, and Infinite mode, negative values give you saturation which features dynamic expansion and noisier harmonics when pushed further. In Symmetric mode, this can generate dropout-like artifacts.\n"
                             "In Deep mode, negative curve values create a lower knee for the saturation curve.\n\n"
                             "Positive values, in Finite, Clip, and Infinite mode, will give you a more compressed and warmer saturation.\n"
                             "In Deep mode, positive values will raise the clipping threshold and create a sharper knee.\n"
                             "And in Warm mode, the curve slider controls the degree of warmth in the coloration.");
    curve__slider.setSize(297, 30);
    curve__slider.setCentrePosition(getLocalBounds().getCentreX(), getHeight() * 0.75f);

    p.onPresetChange = [&]
    { updateBandDisplay(p.numBands); };

    addAndMakeVisible(menu);
    menu.setBoundsRelative(0.12f, 0.05f, 0.03f, 0.03f);

    splash.onLogoClick = [&]
    {
        splash.setImage(createComponentSnapshot(getLocalBounds()));
        splash.setPluginWrapperType(p.wrapperType);
    };

    addChildComponent(unlockButton);
    unlockLNF.setType(TopButtonLNF::Type::Regular);
    unlockButton.setLookAndFeel(&unlockLNF);
    if (!p.isUnlocked)
        unlockButton.setVisible(true);
    unlockButton.onClick = [&]
    {
        if (!activationComp.isFormVisible())
        {
            activationComp.setImage(createComponentSnapshot(getLocalBounds()));
            activationComp.setVisible(true);
        }
    };
    unlockButton.setBoundsRelative(0.12f, 0.12f, 0.08f, 0.05f);

    addChildComponent(activationComp);
    activationComp.onUnlock = [&](var unlocked)
    {
        p.isUnlocked = unlocked;
        unlockButton.setEnabled(false);
        unlockButton.setVisible(false);
    };
    activationComp.centreWithSize(240, 360);

    if (!p.checkUnlock())
    {
        downloadManager.setVisible(false);
        activationComp.setImage(createComponentSnapshot(getLocalBounds()));
        activationComp.setVisible(true);
    }

    addChildComponent(downloadManager);
    downloadManager.centreWithSize(250, 200);
    if (p.hasUpdated)
        downloadManager.setVisible(false);
    downloadManager.onUpdateChange = [&](bool updated)
    {
        p.hasUpdated = updated;
    };
}

MaximizerAudioProcessorEditor::~MaximizerAudioProcessorEditor()
{
#if JUCE_WINDOWS || JUCE_LINUX
    opengl.detach();
#endif
    curve__slider.setLookAndFeel(nullptr);
    unlockButton.setLookAndFeel(nullptr);
}

void MaximizerAudioProcessorEditor::paint(Graphics &g)
{
    g.fillAll(juce::Colour(0xff30414d));

    ColourGradient gradient(Colour(0xa7a7a7a7).withAlpha(0.25f), getLocalBounds().getCentreX(),
                            getLocalBounds().getCentreY(), Colours::transparentWhite, getX(), getY(), true);
    g.setGradientFill(gradient);
    g.fillAll();

    auto w = getWidth();
    auto h = getHeight();

    g.setColour(Colour(0xa7a7a7a7));
    g.setFont(getCustomFont(FontStyle::Bold).withHeight(h * 0.125f));

    int titleWidth = w * 0.208f;

    Rectangle<int> textBounds{getLocalBounds().getCentreX() - (titleWidth / 2), (int)(h * 0.0201f), titleWidth, (int)(h * 0.1042f)};
    g.drawText("PiMax", textBounds, Justification::centred, false);
#if DEBUG
    g.setColour(Colours::red);
    g.setFont(h * 0.042f);
    g.drawText("DEBUG", textBounds.translated(0, h * 0.052f), Justification::centred, false);
#elif !PRODUCTION_BUILD
    g.setColour(Colours::lime);
    g.setFont(h * 0.042f);
    g.drawText("DEV", textBounds.translated(0, h * 0.052f), Justification::centred, false);
#endif

    auto menuBounds = getLocalBounds().removeFromBottom(getHeight() * 0.069).toFloat().reduced(1.5f);
    g.setColour(Colours::black);
    g.fillRoundedRectangle(menuBounds, 3.f);
    g.setColour(Colour(0xa7a7a7a7));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.5f), 7.f, 1.5f);
}

void MaximizerAudioProcessorEditor::resized()
{
    const auto w = getWidth();
    const auto scale = (float)w / 720.f;

    responseCurveComponent.setBounds(162, 114, 402, 210);
    waveshaperComponent.setBounds(162, 114, 402, 210);
    splash.setBounds(0, 0, 720, 480);
    // unlockButton.setBoundsRelative(0.12f, 0.12f, 0.08f, 0.05f);

    auto children = getChildren();
    children.remove(this->getIndexOfChildComponent(&menu));
    children.remove(this->getIndexOfChildComponent(&unlockButton));
    children.remove(this->getIndexOfChildComponent(&activationComp));
    children.removeLast();
    for (auto &child : children)
    {
        child->setTransform(AffineTransform::scale(scale));
    }

    menu.setBoundsRelative(0.12f, 0.05f, 0.03f, 0.03f);
    unlockButton.setBoundsRelative(0.12f, 0.12f, 0.08f, 0.05f);
    activationComp.centreWithSize(240, 360);

    MessageManager::callAsync([&]
                              { writeUISize(getWidth(), getHeight()); });
}

std::vector<Component *> MaximizerAudioProcessorEditor::getComps()
{
    return {
        &ui,
        &responseCurveComponent,
        &waveshaperComponent,
        &curve__slider,
        &splash};
}
