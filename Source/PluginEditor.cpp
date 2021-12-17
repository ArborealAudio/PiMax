/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"

ResponseCurveComponent::ResponseCurveComponent(MaximizerAudioProcessor& p) : audioProcessor(p)
{
    sliders.reserve(3);
    lowPass.reserve(3);
    lowPassCoeffs.reserve(3);
    highPass.reserve(3);
    highPassCoeffs.reserve(3);
    solo.reserve(4);
    mute.reserve(4);
    bypass.reserve(4);
    bandInGain.reserve(4); bandOutGain.reserve(4); bandWidth.reserve(4);

    if (p.lastDownSampleRate >! 0.0)
        sampleRate = 88200.0;
    else
        sampleRate = 2.0 * p.lastDownSampleRate;

    for (int i = 0; i < 3; ++i)
    {
        sliders.emplace_back(std::make_unique<CrossoverSlider>());
        addChildComponent(*sliders[i]);
        sliders[i]->setMouseDragSensitivity(400);
        sliders[i]->setTooltip("Adjusts crossover frequency of the adjacent bands.");
        sliders[i]->onValueChange = [this] { sliderValueChanged = true; };
        lowPass.emplace_back();
        highPass.emplace_back();
        *lowPass[i].coefficients = *dsp::IIR::Coefficients<float>::makeLowPass(sampleRate,
            *audioProcessor.crossovers[i]);
        *highPass[i].coefficients = *dsp::IIR::Coefficients<float>::makeHighPass(sampleRate,
            *audioProcessor.crossovers[i]);
        p.apvts.addParameterListener("crossover" + String(i), this);
    }

    for (int i = 0; i < 4; ++i)
    {
        solo.emplace_back(std::make_unique<TextButton>());
        solo[i]->setClickingTogglesState(true);
        solo[i]->setButtonText("S");
        solo[i]->setColour(TextButton::buttonColourId, Colour(0xff30414d));
        solo[i]->setColour(TextButton::buttonOnColourId, Colours::lightgreen);
        solo[i]->setTooltip("Solos the current band.");
        solo[i]->setAlwaysOnTop(true);
        solo[i]->onClick = [this]() {repaint(); };
        addChildComponent(*solo[i]);
        mute.emplace_back(std::make_unique<TextButton>());
        mute[i]->setClickingTogglesState(true);
        mute[i]->setButtonText("M");
        mute[i]->setColour(TextButton::buttonColourId, Colour(0xff30414d));
        mute[i]->setColour(TextButton::buttonOnColourId, Colours::red);
        mute[i]->setTooltip("Mutes the current band.");
        mute[i]->onClick = [this]() {repaint(); };
        addChildComponent(*mute[i]);
        bypass.emplace_back(std::make_unique<TextButton>());
        bypass[i]->setClickingTogglesState(true);
        bypass[i]->setButtonText("B");
        bypass[i]->setColour(TextButton::buttonColourId, Colour(0xff30414d));
        bypass[i]->setColour(TextButton::buttonOnColourId, Colours::mediumpurple);
        bypass[i]->setTooltip("Bypasses saturation for the current band.");
        bypass[i]->onClick = [this]() {repaint(); };
        addChildComponent(*bypass[i]);

        bandInGain.emplace_back(std::make_unique<BandParamSlider>());
        bandInGain[i]->setLabelType(KnobLNF::LabelType::InGain);
        bandInGain[i]->setPopupDisplayEnabled(true, true, this);
        bandInGain[i]->setTooltip("Controls input gain for the current band.");
        bandInGain[i]->onMouseOver = [this, i]()
        {
            bandInGain[i]->setLNFImage(createComponentSnapshot(getLocalBounds()));
        };
        addChildComponent(*bandInGain[i]);
        bandOutGain.emplace_back(std::make_unique<BandParamSlider>());
        bandOutGain[i]->setLabelType(KnobLNF::LabelType::OutGain);
        bandOutGain[i]->setPopupDisplayEnabled(true, true, this);
        bandOutGain[i]->setTooltip("Controls output gain for the current band.");
        bandOutGain[i]->onMouseOver = [this, i]()
        {
            bandOutGain[i]->setLNFImage(createComponentSnapshot(getLocalBounds()));
        };
        addChildComponent(*bandOutGain[i]);
        bandWidth.emplace_back(std::make_unique<BandParamSlider>());
        bandWidth[i]->setLabelType(KnobLNF::LabelType::Width);
        bandWidth[i]->setPopupDisplayEnabled(true, true, this);
        bandWidth[i]->setTooltip("Controls stereo width for the current band. Follows the toggle state of "
        "the main Width knob for widening a mono source.");
        bandWidth[i]->onMouseOver = [this, i]()
        {
            bandWidth[i]->setLNFImage(createComponentSnapshot(getLocalBounds()));
        };
        addChildComponent(*bandWidth[i]);
    }

    p.apvts.addParameterListener("bandSplit", this);

    numBands = p.numBands;
    
    if (numBands < 3)
        addChildComponent(addButton);
    addButton.setButtonText("+");
    addButton.setClickingTogglesState(false);
    addButton.setTooltip("Adds a new crossover at the current frequency.");
    addButton.onClick = [this, &p]
    {
        setBand();
        p.updateNumBands(numBands);
    };

    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    audioProcessor.apvts.removeParameterListener("bandSplit", this);
    for (int i = 0; i < 3; ++i)
    {
        audioProcessor.apvts.removeParameterListener("crossover" + std::to_string(i), this);
    }
    sampleRate = 0;
    stopTimer();
}

void ResponseCurveComponent::paint(Graphics& g)
{
    if (*audioProcessor.bandSplit) {

        ColourGradient gradient(Colour(0xa7a7a7a7).withAlpha(0.125f),
            getLocalBounds().getCentreX(), getLocalBounds().getCentreY(),
            Colours::transparentWhite, getLocalBounds().getX(), getLocalBounds().getY(), true);
        g.setGradientFill(gradient);
        g.fillAll();

        for (int i = 0; i < numBands; ++i) {
            sliders[i]->setVisible(true);
            if (i == 0 && numBands == 1) {
                sliders[1]->setVisible(false);
                sliders[2]->setVisible(false);
            }
            else if (i == 1 && numBands == 2) {
                sliders[2]->setVisible(false);
            }
        }

        auto responseArea = getLocalBounds();

        auto w = responseArea.getWidth();

        auto slider0Pos = sliders[0]->getPositionOfValue(sliders[0]->getValue());
        auto slider1Pos = sliders[1]->getPositionOfValue(sliders[1]->getValue());
        auto slider2Pos = sliders[2]->getPositionOfValue(sliders[2]->getValue());
        
        if (sliderValueChanged)
            setSliderLimits(slider0Pos, slider1Pos, slider2Pos);

        drawResponseCurve(g, responseArea.toFloat(), w);

        g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);

        drawBandArea(g, slider0Pos, slider1Pos, slider2Pos, responseArea.toFloat());

        drawBandParams(g, slider0Pos, slider1Pos, slider2Pos, w);

        if (isMouseOver() || addButton.isMouseOver())
            drawAddButton(g, responseArea.toFloat());
    }
    else
    {
        for (auto& s : sliders)
            s->setVisible(false);
        for (auto& b : solo)
            b->setVisible(false);
        for (auto& b : mute)
            b->setVisible(false);
        for (auto& b : bypass)
            b->setVisible(false);
    }
}

inline void ResponseCurveComponent::setSliderLimits(float slider0Pos, float slider1Pos,
    float slider2Pos) noexcept
{
    sliderValueChanged = false;

    double w = getLocalBounds().getWidth();

    auto slider1Lim = (slider0Pos + 50.0);
    auto slider2Lim = (slider1Pos + 50.0);

    if (numBands > 1 && (slider1Pos <= slider1Lim)) {
        sliders[0]->setValue(mapToLog10(double(slider1Pos - 50.0) / double(w), 25.0, 24000.0),
            sendNotificationAsync);
        sliders[1]->setValue(mapToLog10(double(slider1Lim) / double(w), 25.0, 24000.0),
            sendNotificationAsync);
    }
    
    if (numBands > 2 && (slider2Pos <= slider2Lim)) {
        sliders[1]->setValue(mapToLog10(double(slider2Pos - 50.0) / double(w), 25.0, 24000.0),
            sendNotificationAsync);
        sliders[2]->setValue(mapToLog10(double(slider2Lim) / double(w), 25.0, 24000.0),
            sendNotificationAsync);
    }
}

inline void ResponseCurveComponent::drawResponseCurve(Graphics& g, const Rectangle<float>& responseArea,
    float w) noexcept
{
    std::vector<double> magLVec, magM1Vec, magM2Vec, magHVec;

    magLVec.resize(w); magM1Vec.resize(w); magM2Vec.resize(w); magHVec.resize(w);

    const auto& lp = lowPass;
    const auto& hp = highPass;

    for (int i = 0; i < w; ++i)
    {
        double magL = 1.f, magH = 1.f, prevMagH = 1.f;

        auto freq = mapToLog10(double(i) / double(w), 25.0, 24000.0);

        if (numBands > 0) {
            magL *= lp[0].coefficients->getMagnitudeForFrequency(freq, sampleRate);
            magLVec[i] = 20.0 * log10(magL);
            prevMagH *= hp[0].coefficients->getMagnitudeForFrequency(freq, sampleRate);
            magM1Vec[i] = 20.0 * log10(prevMagH);
        }
        if (numBands > 1) {
            prevMagH *= lp[1].coefficients->getMagnitudeForFrequency(freq, sampleRate);
            magM1Vec[i] = 20.0 * log10(prevMagH);
            magH *= hp[1].coefficients->getMagnitudeForFrequency(freq, sampleRate);
            prevMagH = magH;
            magM2Vec[i] = 20.0 * log10(magH);
        }
        if (numBands > 2) {
            prevMagH *= lp[2].coefficients->getMagnitudeForFrequency(freq, sampleRate);
            magM2Vec[i] = 20.0 * log10(prevMagH);
            magH *= hp[2].coefficients->getMagnitudeForFrequency(freq, sampleRate);
            magHVec[i] = 20.0 * log10(magH);
        }
    }

    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -12.0, 12.0, outputMin, outputMax);
    };

    g.setColour(Colours::floralwhite);

    std::array<Path, 4> responseCurve
    { {
        Path(),
        Path(),
        Path(),
        Path()
    } };

    for (int i = 0; i < numBands + 1; ++i) {

        if (i == 0) {
            responseCurve[i].startNewSubPath(responseArea.getX(), map(magLVec.front()));
            for (size_t j = 1; j < magLVec.size(); ++j)
            {
                responseCurve[i].lineTo(responseArea.getX() + j, map(magLVec[j]));
            }
            g.strokePath(responseCurve[i], PathStrokeType(2.f));
        }
        else if (i == 1) {
            responseCurve[i].startNewSubPath(responseArea.getX(), map(magM1Vec.front()));
            for (size_t j = 1; j < magM1Vec.size(); ++j)
            {
                responseCurve[i].lineTo(responseArea.getX() + j, map(magM1Vec[j]));
            }
            g.strokePath(responseCurve[i], PathStrokeType(2.f));
        }
        else if (i == 2) {
            responseCurve[i].startNewSubPath(responseArea.getX(), map(magM2Vec.front()));
            for (size_t j = 1; j < magM2Vec.size(); ++j)
            {
                responseCurve[i].lineTo(responseArea.getX() + j, map(magM2Vec[j]));
            }
            g.strokePath(responseCurve[i], PathStrokeType(2.f));
        }
        else if (i == 3) {
            responseCurve[i].startNewSubPath(responseArea.getX(), map(magHVec.front()));
            for (size_t j = 1; j < magHVec.size(); ++j)
            {
                responseCurve[i].lineTo(responseArea.getX() + j, map(magHVec[j]));
            }
            g.strokePath(responseCurve[i], PathStrokeType(2.f));
        }
    }
}

inline void ResponseCurveComponent::drawBandArea(Graphics& g, float slider0Pos, float slider1Pos,
    float slider2Pos, const Rectangle<float>& responseArea) noexcept
{
    auto w = responseArea.getWidth();
    auto h = responseArea.getHeight();

    {
        Rectangle<float> area{ 0, 0, slider0Pos, h };

        if (solo[0]->getToggleState()) {
            g.setColour(Colours::lightgreen.withAlpha(0.1f));
            g.fillRect(area);
        }
        if (mute[0]->getToggleState()) {
            g.setColour(Colours::red.withAlpha(0.1f));
            g.fillRect(area);
        }
        if (bypass[0]->getToggleState()) {
            g.setColour(Colours::mediumpurple.withAlpha(0.1f));
            g.fillRect(area);
        }
    }

    {
        Rectangle<float> area; 
        numBands > 1 ? area.setBounds(slider0Pos, 0, slider1Pos - slider0Pos, h ) : 
                       area.setBounds(slider0Pos, 0, w - slider0Pos, h );

        if (solo[1]->getToggleState()) {
            g.setColour(Colours::lightgreen.withAlpha(0.1f));
            g.fillRect(area);
        }
        if (mute[1]->getToggleState()) {
            g.setColour(Colours::red.withAlpha(0.1f));
            g.fillRect(area);
        }
        if (bypass[1]->getToggleState()) {
            g.setColour(Colours::mediumpurple.withAlpha(0.1f));
            g.fillRect(area);
        }
    }

    if (numBands > 1)
    {
        Rectangle<float> area;
        numBands > 2 ? area.setBounds(slider1Pos, 0, slider2Pos - slider1Pos, h) :
                       area.setBounds( slider1Pos, 0, w - slider1Pos, h );

        if (solo[2]->getToggleState()) {
            g.setColour(Colours::lightgreen.withAlpha(0.1f));
            g.fillRect(area);
        }
        if (mute[2]->getToggleState()) {
            g.setColour(Colours::red.withAlpha(0.1f));
            g.fillRect(area);
        }
        if (bypass[2]->getToggleState()) {
            g.setColour(Colours::mediumpurple.withAlpha(0.1f));
            g.fillRect(area);
        }
    }

    if (numBands > 2)
    {
        Rectangle<float> area{ slider2Pos, 0, w - slider2Pos, h };

        if (solo[3]->getToggleState()) {
            g.setColour(Colours::lightgreen.withAlpha(0.1f));
            g.fillRect(area);
        }
        if (mute[3]->getToggleState()) {
            g.setColour(Colours::red.withAlpha(0.1f));
            g.fillRect(area);
        }
        if (bypass[3]->getToggleState()) {
            g.setColour(Colours::mediumpurple.withAlpha(0.1f));
            g.fillRect(area);
        }
    }
}

inline void ResponseCurveComponent::drawBandParams(Graphics& g, float slider0Pos, float slider1Pos,
    float slider2Pos, float width) noexcept
{
    if (isMouseOverOrDragging(true) && !sliders[0]->isMouseOverOrDragging() && !sliders[1]->isMouseOverOrDragging()
        && !sliders[2]->isMouseOverOrDragging()) {

        #ifndef FLEX_ARGS
        #define FLEX_ARGS FlexBox::Direction::row, FlexBox::Wrap::wrap, FlexBox::AlignContent::spaceAround,\
            FlexBox::AlignItems::center, FlexBox::JustifyContent::center
        #endif

        FlexBox flex(FLEX_ARGS);
        FlexBox buttonsFlexBox(FLEX_ARGS);
        FlexBox knobsFlexBox(FLEX_ARGS);

        for (auto& b : solo)
            b->setVisible(false);
        for (auto& b : mute)
            b->setVisible(false);
        for (auto& b : bypass)
            b->setVisible(false);
        for (auto& s : bandInGain)
            s->setVisible(false);
        for (auto& s : bandOutGain)
            s->setVisible(false);
        for (auto& s : bandWidth)
            s->setVisible(false);

        paintAddButton = true;
        auto mousePos = getMouseXYRelative();
        auto mouseX = mousePos.getX();

        if (mouseX <= slider0Pos) {
            solo[0]->setVisible(true);
            mute[0]->setVisible(true);
            bypass[0]->setVisible(true);

            bandInGain[0]->setVisible(true);
            bandOutGain[0]->setVisible(true);
            bandWidth[0]->setVisible(true);

            Array<FlexItem> buttons;
            Array<FlexItem> knobs;

            buttons.add (FlexItem(20, 20, *solo[0]));
            buttons.add (FlexItem(20, 20, *mute[0]));
            buttons.add (FlexItem(20, 20, *bypass[0]));
            knobs.add (FlexItem(32, 32, *bandInGain[0]));
            knobs.add (FlexItem(32, 32, *bandOutGain[0]));
            knobs.add (FlexItem(32, 32, *bandWidth[0]));

            buttonsFlexBox.items = buttons;
            knobsFlexBox.items = knobs;

            FlexItem buttonsItem(slider0Pos, 60, buttonsFlexBox);
            FlexItem knobsItem(slider0Pos, 96, knobsFlexBox);

            flex.items.add(buttonsItem, knobsItem);

            Rectangle<float> area{ 0, 51, slider0Pos, 145 };

            flex.performLayout(area);
        }
        else if ((mouseX > slider0Pos && mouseX < slider1Pos && numBands > 1)
            || (mouseX > slider0Pos && numBands < 2)) {
            solo[1]->setVisible(true);
            mute[1]->setVisible(true);
            bypass[1]->setVisible(true);

            bandInGain[1]->setVisible(true);
            bandOutGain[1]->setVisible(true);
            bandWidth[1]->setVisible(true);

            Array<FlexItem> buttons;
            Array<FlexItem> knobs;

            buttons.add(FlexItem(20, 20, *solo[1]));
            buttons.add(FlexItem(20, 20, *mute[1]));
            buttons.add(FlexItem(20, 20, *bypass[1]));
            knobs.add(FlexItem(32, 32, *bandInGain[1]));
            knobs.add(FlexItem(32, 32, *bandOutGain[1]));
            knobs.add(FlexItem(32, 32, *bandWidth[1]));

            buttonsFlexBox.items = buttons;
            knobsFlexBox.items = knobs;

            FlexItem buttonsItem(numBands > 1 ? slider1Pos - slider0Pos : width - slider0Pos,
                60, buttonsFlexBox);
            FlexItem knobsItem(numBands > 1 ? slider1Pos - slider0Pos : width - slider0Pos,
                96, knobsFlexBox);

            flex.items.add(buttonsItem, knobsItem);

            Rectangle<float> area{ slider0Pos, 51,
                numBands > 1 ? slider1Pos - slider0Pos : width - slider0Pos,
                145 };

            flex.performLayout(area);

        }
        else if ((mouseX > slider1Pos && numBands == 2) ||
            (mouseX > slider1Pos && mouseX < slider2Pos && numBands > 2)) {

            solo[2]->setVisible(true);
            mute[2]->setVisible(true);
            bypass[2]->setVisible(true);

            bandInGain[2]->setVisible(true);
            bandOutGain[2]->setVisible(true);
            bandWidth[2]->setVisible(true);

            Array<FlexItem> buttons;
            Array<FlexItem> knobs;

            buttons.add(FlexItem(20, 20, *solo[2]));
            buttons.add(FlexItem(20, 20, *mute[2]));
            buttons.add(FlexItem(20, 20, *bypass[2]));
            knobs.add(FlexItem(32, 32, *bandInGain[2]));
            knobs.add(FlexItem(32, 32, *bandOutGain[2]));
            knobs.add(FlexItem(32, 32, *bandWidth[2]));

            buttonsFlexBox.items = buttons;
            knobsFlexBox.items = knobs;

            FlexItem buttonsItem(numBands > 2 ? slider2Pos - slider1Pos : width - slider1Pos,
                60, buttonsFlexBox);
            FlexItem knobsItem(numBands > 2 ? slider2Pos - slider1Pos : width - slider1Pos,
                96, knobsFlexBox);

            flex.items.add(buttonsItem, knobsItem);

            Rectangle<float> area{ slider1Pos, 51,
                numBands > 2 ? slider2Pos - slider1Pos : width - slider1Pos,
                145 };

            flex.performLayout(area);

        }
        else if (mouseX > slider2Pos && numBands > 2) {
            solo[3]->setVisible(true);
            mute[3]->setVisible(true);
            bypass[3]->setVisible(true);

            bandInGain[3]->setVisible(true);
            bandOutGain[3]->setVisible(true);
            bandWidth[3]->setVisible(true);

            Array<FlexItem> buttons;
            Array<FlexItem> knobs;

            buttons.add(FlexItem(20, 20, *solo[3]));
            buttons.add(FlexItem(20, 20, *mute[3]));
            buttons.add(FlexItem(20, 20, *bypass[3]));
            knobs.add(FlexItem(32, 32, *bandInGain[3]));
            knobs.add(FlexItem(32, 32, *bandOutGain[3]));
            knobs.add(FlexItem(32, 32, *bandWidth[3]));

            buttonsFlexBox.items = buttons;
            knobsFlexBox.items = knobs;

            FlexItem buttonsItem(width - slider2Pos, 60, buttonsFlexBox);
            FlexItem knobsItem(width - slider2Pos, 96, knobsFlexBox);

            flex.items.add(buttonsItem, knobsItem);

            Rectangle<float> area{ slider2Pos, 51, width - slider2Pos, 145 };

            flex.performLayout(area);
        }
    }
    else {
        for (auto& b : solo)
            b->setVisible(false);
        for (auto& b : mute)
            b->setVisible(false);
        for (auto& b : bypass)
            b->setVisible(false);
        for (auto& s : bandInGain)
            s->setVisible(false);
        for (auto& s : bandOutGain)
            s->setVisible(false);
        for (auto& s : bandWidth)
            s->setVisible(false);
    }
}

inline void ResponseCurveComponent::drawAddButton(Graphics& g, const Rectangle<float>& responseArea) noexcept
{
    const auto& mouseX = getMouseXYRelative().getX();
    const auto& mouseY = addButton.getMouseXYRelative().getY();

    if (numBands > 2)
        addButton.setVisible(false);
    else if (paintAddButton) {
        addButton.setVisible(true);
        addButton.setBounds(mouseX - 10, getLocalBounds().getY() + 2, 20, 20);
        addButton.setAlpha(float(getHeight() - mouseY - 30) / float(getHeight() - 30));

        if (addButton.isMouseOver()) {
            auto freq = mapToLog10((float)getMouseXYRelative().getX() / responseArea.getWidth(),
                25.f, 24000.f);

            g.setColour(Colours::grey);
            Rectangle<float> area{ (float)getMouseXYRelative().getX() - 20.f, (float)addButton.getBottom() + 0.5f,
                40.f, 15.f };
            g.fillRoundedRectangle(area, 3.f);
            g.setColour(Colours::floralwhite);
            g.setFont(getCustomFont(FontStyle::Regular).withHeight(14.5f));
            if (freq < 1000.f) {
                String freqStr(freq, 1);
                freqStr.append(" Hz", 3);
                g.drawFittedText(freqStr, area.toNearestInt(), Justification::centred, 1, 0.f);
            }
            else {
                String freqStr(freq / 1000.f, 2);
                freqStr.append(" kHz", 4);
                g.drawFittedText(freqStr, area.toNearestInt(), Justification::centred, 1, 0.f);
            }
        }
    }
}

void ResponseCurveComponent::parameterChanged(const String& parameterID, float newValue)
{
    paramChanged = true;
}

void ResponseCurveComponent::timerCallback()
{
    if (paramChanged) {
        paramChanged = false;

        // make sure IIRs update along with crossovers

        for (int i = 0; i < numBands; ++i)
        {
            *lowPass[i].coefficients = dsp::IIR::ArrayCoefficients<float>::makeLowPass
                (sampleRate, *audioProcessor.crossovers[i]);
            *highPass[i].coefficients = dsp::IIR::ArrayCoefficients<float>::makeHighPass
                (sampleRate, *audioProcessor.crossovers[i]);
            lowPass[i].reset();
            highPass[i].reset();
        }
        repaint();
    }
    
    /*only draw add button if mouse is over component*/
    if ((isMouseOver() || addButton.isMouseOver()) && *audioProcessor.bandSplit)
        repaint();
    else
        addButton.setVisible(false);

    if (sliders[0]->wasRightClicked && numBands > 1) { removeBand(0); audioProcessor.updateNumBands(numBands); }
    else { sliders[0]->wasRightClicked = false; }
    if (sliders[1]->wasRightClicked && numBands > 1) { removeBand(1); audioProcessor.updateNumBands(numBands); }
    if (sliders[2]->wasRightClicked && numBands > 1) { removeBand(2); audioProcessor.updateNumBands(numBands); }

 }

inline void ResponseCurveComponent::setBand() noexcept
{
    numBands += 1;

    auto w = getLocalBounds().getWidth();
    auto x = addButton.getX() + 10;

    auto freq = mapToLog10(double(x) / double(w), 25.0, 24000.0);

    std::array<double, 3> value;
    std::array<Value, 3> slider;
    std::array<Value, 4> bandIn;
    std::array<Value, 4> bandOut;
    std::array<Value, 4> bandWidth;
    std::array<Value, 4> bandSolo;
    std::array<Value, 4> bandMute;
    std::array<Value, 4> bandBypass;
    for (int i = 0; i < 3; ++i)
    {
        value[i] = sliders[i]->getValue();
        slider[i] = audioProcessor.apvts.getParameterAsValue("crossover" + String(i));
    }
    for (int i = 0; i < 4; ++i)
    {
        bandIn[i] = audioProcessor.apvts.getParameterAsValue("bandInGain" + std::to_string(i));
        bandOut[i] = audioProcessor.apvts.getParameterAsValue("bandOutGain" + std::to_string(i));
        bandWidth[i] = audioProcessor.apvts.getParameterAsValue("bandWidth" + std::to_string(i));
        bandSolo[i] = audioProcessor.apvts.getParameterAsValue("soloBand" + std::to_string(i));
        bandMute[i] = audioProcessor.apvts.getParameterAsValue("muteBand" + std::to_string(i));
        bandBypass[i] = audioProcessor.apvts.getParameterAsValue("bypassBand" + std::to_string(i));
    }

    if (numBands > 1)
        sliders[numBands - 1]->setVisible(true);

    if (freq < value[0]) {
        if (numBands > 2) {
            slider[2] = value[1];
            bandIn[3] = bandIn[2].getValue();
            bandOut[3] = bandOut[2].getValue();
            bandWidth[3] = bandWidth[2].getValue();
            bandSolo[3] = bandSolo[2].getValue();
            bandMute[3] = bandMute[2].getValue();
            bandBypass[3] = bandBypass[2].getValue();
            bandIn[2] = bandIn[1].getValue();
            bandOut[2] = bandOut[1].getValue();
            bandWidth[2] = bandWidth[1].getValue();
            bandSolo[2] = bandSolo[1].getValue();
            bandMute[2] = bandMute[1].getValue();
            bandBypass[2] = bandBypass[1].getValue();

        }
        slider[1] = value[0];
        slider[0] = freq;
        bandIn[2] = bandIn[1].getValue();
        bandOut[2] = bandOut[1].getValue();
        bandWidth[2] = bandWidth[1].getValue();
        bandSolo[2] = bandSolo[1].getValue();
        bandMute[2] = bandMute[1].getValue();
        bandBypass[2] = bandBypass[1].getValue();
        bandIn[1] = bandIn[0].getValue();
        bandOut[1] = bandOut[0].getValue();
        bandWidth[1] = bandWidth[0].getValue();
        bandSolo[1] = bandSolo[0].getValue();
        bandMute[1] = bandMute[0].getValue();
        bandBypass[1] = bandBypass[0].getValue();
    }
    else if (numBands == 2) {
        if (freq > value[0]) {
            slider[1] = freq;
            bandIn[2] = 0.0;
            bandOut[2] = 0.0;
            bandWidth[2] = 1.0;
            bandSolo[2] = false;
            bandMute[2] = false;
            bandBypass[2] = false;
        }
    }
    else if (numBands == 3) {
        if (freq > value[0] && freq > value[1]) {
            slider[2] = freq;
            bandIn[3] = 0.0;
            bandOut[3] = 0.0;
            bandWidth[3] = 1.0;
            bandSolo[3] = false;
            bandMute[3] = false;
            bandBypass[3] = false;
        }
        else if (freq > value[0] && freq < value[1]) {
            slider[1] = freq;
            slider[2] = value[1];
            bandIn[3] = bandIn[2].getValue();
            bandOut[3] = bandOut[2].getValue();
            bandWidth[3] = bandWidth[2].getValue();
            bandSolo[3] = bandSolo[2].getValue();
            bandMute[3] = bandMute[2].getValue();
            bandBypass[3] = bandBypass[2].getValue();
            bandIn[2] = 0.0;
            bandOut[2] = 0.0;
            bandWidth[2] = 1.0;
            bandSolo[2] = false;
            bandMute[2] = false;
            bandBypass[2] = false;
        }
    }

    sliders[numBands - 1]->setBounds(getLocalBounds());
    
}

inline void ResponseCurveComponent::removeBand(int index) noexcept
{
    std::array<Value, 3> slider;
    std::array<Value, 4> bandIn;
    std::array<Value, 4> bandOut;
    std::array<Value, 4> bandWidth;
    std::array<Value, 4> bandSolo;
    std::array<Value, 4> bandMute;
    std::array<Value, 4> bandBypass;
    std::array<double, 3> value;
    for (int i = 0; i < 3; ++i) {
        slider[i] = audioProcessor.apvts.getParameterAsValue("crossover" + std::to_string(i));
        value[i] = sliders[i]->getValue();
    }
    for (int i = 0; i < 4; ++i)
    {
        bandIn[i] = audioProcessor.apvts.getParameterAsValue("bandInGain" + std::to_string(i));
        bandOut[i] = audioProcessor.apvts.getParameterAsValue("bandOutGain" + std::to_string(i));
        bandWidth[i] = audioProcessor.apvts.getParameterAsValue("bandWidth" + std::to_string(i));
        bandSolo[i] = audioProcessor.apvts.getParameterAsValue("soloBand" + std::to_string(i));
        bandMute[i] = audioProcessor.apvts.getParameterAsValue("muteBand" + std::to_string(i));
        bandBypass[i] = audioProcessor.apvts.getParameterAsValue("bypassBand" + std::to_string(i));
    }

    sliders[index]->wasRightClicked = false;

    if (index > 0) {
        if (index < 2) {
            if (numBands > 2) {
                slider[1] = value[2];
                bandIn[2] = bandIn[3].getValue();
                bandOut[2] = bandOut[3].getValue();
                bandWidth[2] = bandWidth[3].getValue();
                bandSolo[2] = bandSolo[3].getValue();
                bandMute[2] = bandMute[3].getValue();
                bandBypass[2] = bandBypass[3].getValue();
                slider[2] = 40.0;
                bandIn[3] = 0.0;
                bandOut[3] = 0.0;
                bandWidth[3] = 1.0;
                bandSolo[3] = false;
                bandMute[3] = false;
                bandBypass[3] = false;
                sliders[2]->setVisible(false);
                numBands -= 1;
            }
            else {
                slider[1] = 40.0;
                bandIn[2] = 0.0;
                bandOut[2] = 0.0;
                bandWidth[2] = 1.0;
                bandSolo[2] = false;
                bandMute[2] = false;
                bandBypass[2] = false;
                sliders[1]->setVisible(false);
                numBands -= 1;
            }
        }
        else {
            slider[2] = 40.0;
            bandIn[3] = 0.0;
            bandOut[3] = 0.0;
            bandWidth[3] = 1.0;
            bandSolo[3] = false;
            bandMute[3] = false;
            bandBypass[3] = false;
            sliders[2]->setVisible(false);
            numBands -= 1;
        }
    }
    else {
        slider[0] = value[1];
        bandIn[0] = bandIn[1].getValue();
        if (numBands > 2) {
            slider[1] = value[2];
            bandIn[1] = bandIn[2].getValue();
            bandOut[1] = bandOut[2].getValue();
            bandWidth[1] = bandWidth[2].getValue();
            bandSolo[1] = bandSolo[2].getValue();
            bandMute[1] = bandMute[2].getValue();
            bandBypass[1] = bandBypass[2].getValue();
            bandIn[2] = bandIn[3].getValue();
            bandOut[2] = bandOut[3].getValue();
            bandWidth[2] = bandWidth[3].getValue();
            bandSolo[2] = bandSolo[3].getValue();
            bandMute[2] = bandMute[3].getValue();
            bandBypass[2] = bandBypass[3].getValue();
            slider[2] = 40.0;
            bandIn[3] = 0.0;
            bandOut[3] = 0.0;
            bandWidth[3] = 1.0;
            bandSolo[3] = false;
            bandMute[3] = false;
            bandBypass[3] = false;
            sliders[2]->setVisible(false);
            numBands -= 1;
        }
        else {
            slider[1] = 40.0;
            bandIn[1] = bandIn[2].getValue();
            bandOut[1] = bandOut[2].getValue();
            bandWidth[1] = bandWidth[2].getValue();
            bandSolo[1] = bandSolo[2].getValue();
            bandMute[1] = bandMute[2].getValue();
            bandBypass[1] = bandBypass[2].getValue();
            bandIn[2] = 0.0;
            bandOut[2] = 0.0;
            bandWidth[2] = 1.0;
            bandSolo[2] = false;
            bandMute[2] = false;
            bandBypass[2] = false;
            sliders[1]->setVisible(false);
            numBands -= 1;
        }
    }

    audioProcessor.updateNumBands(numBands);
    
}

void ResponseCurveComponent::resized()
{
    for (int i = 0; i < 3; ++i) {
        sliders[i]->setBounds(getLocalBounds());
    }
}

WaveshaperComponent::WaveshaperComponent(MaximizerAudioProcessor& p)
    : audioProcessor(p)
{
    p.apvts.addParameterListener("gain", this);
    p.apvts.addParameterListener("output", this);
    p.apvts.addParameterListener("curve", this);
    p.apvts.addParameterListener("clipType", this);
    p.apvts.addParameterListener("distType", this);
    p.apvts.addParameterListener("bandSplit", this);
    startTimerHz(60);
}

WaveshaperComponent::~WaveshaperComponent()
{
    audioProcessor.apvts.removeParameterListener("gain", this);
    audioProcessor.apvts.removeParameterListener("output", this);
    audioProcessor.apvts.removeParameterListener("curve", this);
    audioProcessor.apvts.removeParameterListener("clipType", this);
    audioProcessor.apvts.removeParameterListener("distType", this);
    audioProcessor.apvts.removeParameterListener("bandSplit", this);
}

void WaveshaperComponent::paint(Graphics& g)
{
    if (!*audioProcessor.bandSplit) {

        Path p;

        auto w = getWidth();

        auto gain = pow(10.f, (*audioProcessor.gain_dB / 20.f));
        float curve = *audioProcessor.curve;

        ColourGradient gradient(Colour(0xa7a7a7a7).withAlpha(gain / 8.f),
            getLocalBounds().getCentreX(), getLocalBounds().getCentreY(),
            Colours::transparentWhite, getLocalBounds().getX(), getLocalBounds().getY(), true);

        if (curve > 1.0) {
            curve = jmap(curve, 1.f, 1.2f, 0.f, 1.f);
            gradient.setColour(1, Colours::transparentWhite.interpolatedWith(Colours::seagreen, curve / 3.f));
        }
        else if (curve < 1.0) {
            curve = jmap(curve, 0.83f, 1.f, 0.f, 1.f);
            gradient.setColour(1, Colours::transparentWhite.interpolatedWith(Colours::teal, (1.f - curve) / 3.f));
        }

        g.setGradientFill(gradient);
        g.fillAll();

        std::vector<float> mag;
        mag.resize(w);

        for (int n = 0; n < w; ++n)
        {
            auto x = jmap(float(n) / float(w), -1.f, 1.f);

            x *= gain;

            mag[n] = strix::inflator_v2(x, *audioProcessor.curve, *audioProcessor.clip, *audioProcessor.distIndex);

            mag[n] *= pow(10.f, (*audioProcessor.output_dB / 20.f));
        }

        const float outputMin = getLocalBounds().getBottom();
        const float outputMax = getLocalBounds().getY();
        auto map = [outputMax, outputMin](float input)
        {
            return jmap(input, -1.f, 1.f, outputMin, outputMax);
        };
        p.startNewSubPath(getLocalBounds().getX(), map(mag.front()));
        for (size_t i = 1; i < mag.size(); ++i)
        {
            p.lineTo(getLocalBounds().getX() + i, map(mag[i]));
        }

        g.setColour(Colours::floralwhite);
        g.strokePath(p, PathStrokeType(2.f));

        g.drawRoundedRectangle(getLocalBounds().toFloat(), 4.f, 1.f);
    }
}

void WaveshaperComponent::parameterChanged(const String& parameterID, float newValue)
{
    paramChanged = true;
}

void WaveshaperComponent::timerCallback()
{
    if (paramChanged) {
        paramChanged = false;
        repaint();
    }
}

//==============================================================================
MaximizerAudioProcessorEditor::MaximizerAudioProcessorEditor(MaximizerAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), responseCurveComponent(p), ui(p),
    waveshaperComponent(p), activationComp(p.isUnlocked, p.trialRemaining_ms),
    downloadManager(p.hasUpdated), tooltip(nullptr, 2000)
{
    tooltip.setColour(TooltipWindow::backgroundColourId, Colours::darkslategrey);

    opengl.setImageCacheSize(static_cast<size_t>(64 * 1024 * 1024));
    opengl.attachTo(*this);

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }

    waveshaperComponent.setInterceptsMouseClicks(false, true);

    gainAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "gain",
        ui.gain__slider);
    outVolAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "output",
        ui.outVol__slider);
    curveAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "curve",
        ui.curve__slider);
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
    hqAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,
        "hq", ui.hq);
    renderButtonAttach = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,
        "renderHQ", ui.renderHQ);
    linearPhaseAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.apvts,
        "linearPhase", ui.linearPhaseButton);
    mixAttachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.apvts, "mix",
        ui.mixSlider);    

    setResizable(true, true);
    getConstrainer()->setMinimumSize(450, 300);
    getConstrainer()->setFixedAspectRatio(1.5);

    setSize (p.lastUIWidth, p.lastUIHeight);

    p.onPresetChange = [&] { updateBandDisplay(p.numBands); };

    splash.onLogoClick = [this]
    {
        splash.setImage(createComponentSnapshot(getLocalBounds()));
    };

    addChildComponent(activationComp);
    activationComp.onButtonClick = [this]
    {
        activationComp.setImage(createComponentSnapshot(getLocalBounds()));
    };
    activationComp.onUnlock = [&](var unlocked)
    {
        p.isUnlocked = unlocked;
        splash.setOwner(activationComp.getOwner(false));
    };

    if (!p.checkUnlock()) {
        downloadManager.setVisible(false);
        activationComp.setImage(createComponentSnapshot(getLocalBounds()));
        activationComp.setVisible(true);
    }
    else {
        splash.setOwner(activationComp.getOwner(true));
    }

    addChildComponent(downloadManager);
    downloadManager.centreWithSize(250, 200);
    if (p.hasUpdated)
        downloadManager.setVisible(false);
    downloadManager.onUpdateChange = [&](bool updated)
    {
        p.updateStatus(updated);
    };
}

MaximizerAudioProcessorEditor::~MaximizerAudioProcessorEditor()
{
    opengl.detach();
}

void MaximizerAudioProcessorEditor::paint (Graphics& g)
{
}

void MaximizerAudioProcessorEditor::resized()
{
    auto scale = (float)getWidth() / 720.f;
    responseCurveComponent.setBounds(162, 114, 402, 210);
    waveshaperComponent.setBounds(162, 114, 402, 210);
    splash.setBounds(0, 0, 720, 480);
    activationComp.setBounds(0, 0, 720, 480);

    auto children = getChildren();
    children.removeLast(1);
    for (auto& child : children)
    {
        child->setTransform(AffineTransform::scale(scale));
    }

    audioProcessor.lastUIWidth = getWidth();
    audioProcessor.lastUIHeight = getHeight();
}

std::vector<Component*> MaximizerAudioProcessorEditor::getComps()
{
    return
    {
        &ui,
        &responseCurveComponent,
        &waveshaperComponent,
        &splash
    };
}
