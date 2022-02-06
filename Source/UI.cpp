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
#include "PluginEditor.h"
//[/Headers]

#include "UI.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
#define FONT getCustomFont(FontStyle::Regular)
//[/MiscUserDefs]

//==============================================================================
UI::UI (MaximizerAudioProcessor& p) : audioProcessor(p), gain__slider(false), outVol__slider(true),
                                      presetComp(p.apvts)
{
    //[Constructor_pre] You can add your own custom stuff here..
    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);

    outputLNF.setColour(foleys::LevelMeter::lmMeterGradientLowColour, Colours::seagreen);
    outputLNF.setColour(foleys::LevelMeter::lmMeterGradientMidColour, Colours::lightyellow);
    outputLNF.setColour(foleys::LevelMeter::lmMeterGradientMaxColour, Colours::red.interpolatedWith(Colours::pink, 0.25));
    outputLNF.setColour(foleys::LevelMeter::lmBackgroundClipColour, Colours::red);
    outputLNF.setColour(foleys::LevelMeter::lmBackgroundColour, Colours::transparentWhite);
    outputLNF.setColour(foleys::LevelMeter::lmMeterBackgroundColour, Colours::transparentWhite);
    outputLNF.setColour(foleys::LevelMeter::lmTicksColour, Colours::white);
    inputLNF.setColour(foleys::LevelMeter::lmMeterBackgroundColour, Colours::transparentWhite);
    inputLNF.setColour(foleys::LevelMeter::lmMeterGradientLowColour, Colours::seagreen);
    inputLNF.setColour(foleys::LevelMeter::lmMeterGradientMidColour, Colours::lightyellow);
    inputLNF.setColour(foleys::LevelMeter::lmMeterGradientMaxColour, Colours::red.interpolatedWith(Colours::pink, 0.25));
    inputLNF.setColour(foleys::LevelMeter::lmBackgroundColour, Colours::transparentWhite);
    inputLNF.setColour(foleys::LevelMeter::lmTicksColour, Colours::white);

    inputMeter.setLookAndFeel(&inputLNF);
    inputMeter.setRefreshRateHz(30);
    inputMeter.setMeterSource(&audioProcessor.getInputMeterSource());
    inputMeter.setInterceptsMouseClicks(false, false);

    outputMeter.setLookAndFeel(&outputLNF);
    outputMeter.setRefreshRateHz(30);
    outputMeter.setMeterSource(&audioProcessor.getOutputMeterSource());
    outputMeter.setInterceptsMouseClicks(true, false);

    inputMeter.setBounds(33, 88, 40, 215);
    outputMeter.setBounds(538, 88, 40, 213);
    

    gain__slider.setSliderStyle(juce::Slider::LinearVertical);
    inGainLNF.textOnLeft = false;
    gain__slider.setLookAndFeel(&inGainLNF);
    gain__slider.setPaintingIsUnclipped(true);
    gain__slider.setSliderSnapsToMousePosition(false);
    gain__slider.setTooltip("Sets the input gain. In band split mode, the gain is applied to the entire signal"
        " before the multiband process.");
    addAndMakeVisible(gain__slider);
    gain__slider.setBounds(33, 97, 100, 210);

    outVol__slider.setSliderStyle(juce::Slider::LinearVertical);
    outGainLNF.textOnLeft = true;
    outVol__slider.setLookAndFeel(&outGainLNF);
    outVol__slider.setPaintingIsUnclipped(true);
    outVol__slider.setSliderSnapsToMousePosition(false);
    outVol__slider.setTooltip("Output gain after all other processing, before the Mix blend.");
    addAndMakeVisible(outVol__slider);
    outVol__slider.setBounds(478, 95, 100, 210);

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
#if !JUCE_MAC
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

    addAndMakeVisible(curve__slider);
    curve__slider.setBufferedToImage(true);
    curve__slider.setSliderStyle(juce::Slider::LinearHorizontal);
    curve__slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    curve__slider.setLookAndFeel(&curveLNF);
    curve__slider.setSliderSnapsToMousePosition(false);
    curve__slider.setPopupDisplayEnabled(true, true, nullptr, 2000);
    curve__slider.setTooltip("Negative values: saturation which features dynamic expansion and noisier harmonics when"
        " pushed further. In Symmetric mode, this can generate dropout-like artifacts.\n\n"
        "Positive values: a more compressed and warmer saturation.");
    curve__slider.setSize(248, 25);

    //curve__slider.setBounds(getLocalBounds().getCentreX(), 275, 248, 40);

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
    StringArray clipMode{ "finite", "clip", "infinite" };
    clipBox.addItemList(clipMode, 1);
    clipBox.setTooltip("Finite = saturation which folds back towards zero instead of soft clipping.\n\n"
        "Clip = clip at digital maximum, like traditional soft clipping. Good for transient-heavy material.\n\n"
        "Infinite = foldback saturation which folds through zero and towards the opposite pole, resulting in"
        " distortion which can affect the pitch of the sound.");
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
    //bandSplit__textButton.setBounds(244, 328, 88, 32);

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
    widthSlider.altDown = *p.monoWidth; widthLNF.altDown = *p.monoWidth;
    widthSlider.onAltClick = [&]
    {
        widthLNF.altDown = widthSlider.altDown;
        auto monoWidth = p.apvts.getParameterAsValue("monoWidth");
        monoWidth = widthSlider.altDown;
    };
    widthSlider.setBounds(415, 300, 50, 75);

    addAndMakeVisible(mixSlider);
    mixSlider.setSliderStyle(Slider::SliderStyle::RotaryVerticalDrag);
    mixSlider.setLookAndFeel(&mixLNF);
    mixLNF.setLabelType(KnobLNF::LabelType::Mix);
    mixSlider.setSliderSnapsToMousePosition(false);
    mixSlider.setTooltip("Blends processed output with the dry input signal. Alt / Option-click to enable Delta mode,"
        " which subtracts the dry signal from the processed signal, allowing you to hear what PiMax is adding to the input signal.");
    mixSlider.altDown = *p.delta; mixLNF.altDown = *p.delta;
    mixSlider.onAltClick = [&]
    {
        mixLNF.altDown = mixSlider.altDown;
        auto delta = p.apvts.getParameterAsValue("delta");
        delta = mixSlider.altDown;
    };
    mixSlider.setBounds(480, 300, 50, 75);

    addAndMakeVisible(presetComp);
    presetComp.setCurrentPreset(p.currentPreset);
    presetComp.box.onChange = [&]
    {
        presetComp.valueChanged(false);
        p.currentPreset = getCurrentPreset();
    };
    presetComp.setBounds(30, 374, 185, 24);

    //[/Constructor_pre]
    
    //[UserPreSize]
    auto children = getChildren();
    for (auto child : children)
    {
        child->setTransform(AffineTransform::scale(1.2));
    }
    setSize(720, 480);
    //[/UserPreSize]

    //[Constructor] You can add your own custom stuff here..
    curve__slider.setCentrePosition(getLocalBounds().getCentreX(), 1.2 * 295.f);
    bandSplit__textButton.setCentrePosition(getLocalBounds().getCentreX(), 1.2 * 344.f);
    boost.setCentrePosition(getLocalBounds().getCentreX(), 1.2 * 80.f);
    //[/Constructor]
}

UI::~UI()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    inputMeter.setLookAndFeel(nullptr);
    outputMeter.setLookAndFeel(nullptr);
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
    curve__slider.setLookAndFeel(nullptr);
    //[/Destructor_pre]

    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void UI::paint (juce::Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    g.fillAll(juce::Colour(0xff30414d));

    ColourGradient gradient(Colour(0xa7a7a7a7).withAlpha(0.25f), getLocalBounds().getCentreX(),
        getLocalBounds().getCentreY(), Colours::transparentWhite, getX(), getY(), true);
    g.setGradientFill(gradient);
    g.fillAll();

    g.setColour(Colour(0xa7a7a7a7));
    g.setFont(getCustomFont(FontStyle::Bold).withHeight(60.f));
#if JUCE_WINDOWS
    int width = 122;
#elif JUCE_MAC
    int width = 150;
#endif
    Rectangle<int> textBounds{ getLocalBounds().getCentreX() - (width / 2),
    10,
    width,
    50 };
    g.drawText("PiMax", textBounds, Justification::centred, false);

    //[/UserPrePaint]

    {
        //[UserPaintCustomArguments] Customize the painting arguments here..
        //[/UserPaintCustomArguments]
        
    }

    //{
    //    int x = 69, y = 9, width = 100, height = 28;
    //    juce::String text (TRANS("ARBOREAL"));
    //    juce::Colour fillColour = juce::Colour (0xa7a7a7a7);
    //    //[UserPaintCustomArguments] Customize the painting arguments here..
    //    //[/UserPaintCustomArguments]
    //    g.setColour (fillColour);
    //    g.setFont (juce::Font ("Menlo", 13.00f, juce::Font::bold).withExtraKerningFactor (0.385f));
    //    g.drawText (text, x, y, width, height,
    //                juce::Justification::centred, true);
    //}

    //{
    //    int x = 81, y = 30, width = 36, height = 9;
    //    juce::String text (TRANS("[audio]"));
    //    juce::Colour fillColour = juce::Colour (0xa7a7a7a7);
    //    //[UserPaintCustomArguments] Customize the painting arguments here..
    //    //[/UserPaintCustomArguments]
    //    g.setColour (fillColour);
    //    g.setFont (juce::Font (15.00f, juce::Font::plain).withTypefaceStyle ("Regular"));
    //    g.drawText (text, x, y, width, height,
    //                juce::Justification::centred, true);
    //}

    //[UserPaint] Add your own custom painting code here..
    {
        auto menuBounds = getLocalBounds().removeFromBottom(33).toFloat().reduced(1.5f);
        g.setColour(Colours::black);
        g.fillRoundedRectangle(menuBounds, 3.f);
        g.setColour(Colour(0xa7a7a7a7));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.5f), 7.f, 1.5f);
    }

    //[/UserPaint]
}

void UI::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="UI" componentName="" parentClasses="public juce::Component"
                 constructorParams="" variableInitialisers="" snapPixels="8" snapActive="1"
                 snapShown="1" overlayOpacity="0.330" fixedSize="0" initialWidth="600"
                 initialHeight="400">
  <BACKGROUND backgroundColour="ff30414d">
    <IMAGE pos="12 12 60 52" resource="g245_png" opacity="1.0" mode="1"/>
    <TEXT pos="69 9 100 28" fill="solid: a7a7a7a7" hasStroke="0" text="ARBOREAL"
          fontname="Menlo" fontsize="13.0" kerning="0.385" bold="1" italic="0"
          justification="36" typefaceStyle="Bold"/>
    <TEXT pos="81 30 36 9" fill="solid: a7a7a7a7" hasStroke="0" text="[audio]"
          fontname="Default font" fontsize="15.0" kerning="0.0" bold="0"
          italic="0" justification="36"/>
  </BACKGROUND>
  <SLIDER name="gain" id="5806b49f26a1100a" memberName="gain__slider" virtualName=""
          explicitFocusOrder="0" pos="16 70 72 240" textboxhighlight="42a2c8"
          textboxoutline="8e989b" min="-12.0" max="12.0" int="0.0" style="LinearVertical"
          textBoxPos="TextBoxAbove" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1.0" needsCallback="0"/>
  <SLIDER name="outVol" id="70bef10f60e5610c" memberName="outVol__slider"
          virtualName="" explicitFocusOrder="0" pos="588r 70 72 240" textboxhighlight="42a2c8"
          textboxoutline="8e989b" min="-12.0" max="12.0" int="0.0" style="LinearVertical"
          textBoxPos="TextBoxAbove" textBoxEditable="1" textBoxWidth="80"
          textBoxHeight="20" skewFactor="1.0" needsCallback="0"/>
  <SLIDER name="curve" id="1f540678b01e64d8" memberName="curve__slider"
          virtualName="" explicitFocusOrder="0" pos="426r 269 248 40" textboxbkgd="263238"
          textboxhighlight="42a2c8" textboxoutline="8e989b" min="-100.0"
          max="100.0" int="0.0" style="LinearHorizontal" textBoxPos="TextBoxAbove"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1.0"
          needsCallback="0"/>
  <TEXTBUTTON name="clip" id="d325900a41c8183f" memberName="clip__textButton"
              virtualName="" explicitFocusOrder="0" pos="56 328 88 32" tooltip="When on, will clip sound at digital maximum. When off, signals above the digital maximum will either soft clip or be gradually attenuated"
              buttonText="" connectedEdges="0" needsCallback="0" radioGroupId="0"/>
  <TEXTBUTTON name="invert" id="973a48b5a5695f77" memberName="invert__textButton"
              virtualName="" explicitFocusOrder="0" pos="256 328 88 32" tooltip="Very loud signals will amplify towards the opposite polarity, instead of just attenuating"
              buttonText="" connectedEdges="0" needsCallback="0" radioGroupId="0"/>
  <TEXTBUTTON name="bandSplit" id="acad427408f72f5e" memberName="bandSplit__textButton"
              virtualName="" explicitFocusOrder="0" pos="456 328 88 32" tooltip="Splits the signal into three controllable frequency bands"
              buttonText="" connectedEdges="0" needsCallback="0" radioGroupId="0"/>
  <TEXTBUTTON name="hqButton" id="7185686cad8f315" memberName="hqButton" virtualName=""
              explicitFocusOrder="0" pos="184 368 32 24" tooltip="4x oversampling with minimal latency"
              buttonText="" connectedEdges="0" needsCallback="0" radioGroupId="0"/>
  <TEXTBUTTON name="renderHQ" id="b4c5adf364c9b632" memberName="renderHQ" virtualName=""
              explicitFocusOrder="0" pos="361 367 80 24" tooltip="8x oversampling only when rendering"
              buttonText="" connectedEdges="0" needsCallback="0" radioGroupId="0"/>
  <LABEL name="clipLabel" id="ebcc52099c581d6d" memberName="clipLabel"
         virtualName="" explicitFocusOrder="0" pos="59 332 82 24" edTextCol="ff000000"
         edBkgCol="0" labelText="CLIP" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Menlo" fontsize="13.1" kerning="0.0"
         bold="0" italic="0" justification="36"/>
  <LABEL name="inversionLabel" id="e6938a9589bf1675" memberName="inversionLabel"
         virtualName="" explicitFocusOrder="0" pos="261 330 80 29" edTextCol="ff000000"
         edBkgCol="0" labelText="ALLOW INVERSION" editableSingleClick="0"
         editableDoubleClick="0" focusDiscardsChanges="0" fontname="Menlo"
         fontsize="13.1" kerning="0.0" bold="0" italic="0" justification="36"/>
  <LABEL name="bandSplitLabel" id="46ba0d89ab94242e" memberName="bandSplitLabel"
         virtualName="" explicitFocusOrder="0" pos="460 332 80 24" edTextCol="ff000000"
         edBkgCol="0" labelText="BAND SPLIT" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Menlo" fontsize="13.1" kerning="0.0"
         bold="0" italic="0" justification="36"/>
  <LABEL name="hqLabel" id="aa2967de4515d8d1" memberName="hqLabel" virtualName=""
         explicitFocusOrder="0" pos="187 370 26 20" edTextCol="ff000000"
         edBkgCol="0" labelText="HQ" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Menlo" fontsize="13.0" kerning="0.0"
         bold="0" italic="0" justification="36"/>
  <LABEL name="new label" id="7dfcf6205f8fba98" memberName="juce__label"
         virtualName="" explicitFocusOrder="0" pos="364 370 73 17" edTextCol="ff000000"
         edBkgCol="0" labelText="Render HQ" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Menlo" fontsize="13.0" kerning="0.0"
         bold="0" italic="0" justification="36"/>
  <TEXTBUTTON name="linearPhaseButton" id="c8676adcc4e372a0" memberName="linearPhaseButton"
              virtualName="" explicitFocusOrder="0" pos="555 367 31 24" buttonText="Linear Phase"
              connectedEdges="0" needsCallback="0" radioGroupId="0"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif

//==============================================================================
// Binary resources - be careful not to edit any of these sections!

// JUCER_RESOURCE: g245_png, 7383, "Resources/g245.png"
//static const unsigned char resource_UI_g245_png[] = { 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,100,0,0,0,100,8,6,0,0,0,112,226,149,84,0,0,0,9,112,72,89,115,0,0,4,63,0,0,4,63,1,180,209,5,47,
//0,0,0,25,116,69,88,116,83,111,102,116,119,97,114,101,0,119,119,119,46,105,110,107,115,99,97,112,101,46,111,114,103,155,238,60,26,0,0,28,100,73,68,65,84,120,156,213,157,123,112,92,197,153,232,191,175,251,
//156,121,232,49,182,12,126,8,27,252,192,96,155,103,120,5,2,36,49,225,18,19,239,117,128,80,28,107,206,72,2,93,156,21,155,173,114,82,174,84,221,123,9,119,209,146,212,38,169,220,235,173,213,205,230,102,114,
//189,76,144,52,167,199,39,216,172,99,34,158,89,65,10,8,155,240,8,6,12,194,96,99,252,146,13,126,72,26,89,51,115,186,251,187,127,248,200,43,143,198,246,72,154,25,113,127,85,167,202,238,211,167,187,143,190,
//233,199,249,190,175,191,70,248,156,227,186,110,0,0,166,105,173,35,74,169,90,195,48,34,68,20,34,34,19,0,56,0,4,180,214,6,0,0,99,76,2,64,14,0,20,34,122,74,169,12,0,12,112,206,7,25,99,3,0,208,111,89,86,110,
//202,94,166,8,112,170,27,144,79,60,30,175,170,169,169,153,131,136,179,1,96,54,34,86,1,64,78,107,61,136,136,3,0,48,144,203,229,50,134,97,84,49,198,102,2,192,182,108,54,43,1,0,130,193,160,97,24,198,57,217,
//108,182,94,41,181,39,16,8,132,0,32,66,68,17,198,88,45,0,4,136,232,56,99,172,79,41,117,40,157,78,247,181,182,182,30,159,186,183,29,203,231,66,32,174,235,78,243,60,111,161,97,24,231,107,173,107,181,214,
//199,0,224,160,105,154,125,67,67,67,135,91,90,90,50,249,207,164,82,169,101,0,240,229,134,134,134,95,141,78,23,66,92,69,68,95,176,109,59,145,255,76,34,145,8,153,166,121,110,32,16,152,237,121,222,108,198,
//216,116,34,26,36,162,61,166,105,238,178,44,171,191,124,111,89,28,198,84,85,220,221,221,29,60,124,248,240,2,206,249,66,41,101,29,34,238,55,12,99,91,56,28,238,91,185,114,101,182,136,34,120,193,68,206,73,
//41,85,240,135,230,11,118,175,127,65,34,145,8,69,34,145,217,217,108,118,129,148,114,229,198,141,27,143,16,209,174,72,36,178,187,200,54,148,156,138,11,164,163,163,163,58,20,10,45,237,239,239,95,108,24,70,
//154,115,190,107,104,104,232,133,66,189,224,76,72,41,145,115,78,249,233,158,231,1,99,172,168,50,252,58,119,3,192,110,127,174,186,64,41,181,240,232,209,163,87,39,147,201,157,233,116,250,157,74,15,105,21,
//19,136,235,186,53,0,176,196,243,188,139,164,148,135,0,224,247,182,109,127,54,209,242,16,145,3,128,202,79,231,156,19,17,141,123,40,246,39,251,15,1,224,195,100,50,89,103,154,230,165,53,53,53,223,116,28,
//103,183,214,250,221,198,198,198,129,137,182,117,60,148,93,32,241,120,220,172,171,171,187,66,74,121,177,97,24,251,77,211,124,214,178,172,35,147,45,151,115,206,148,82,58,63,93,107,77,48,201,185,49,22,139,
//29,5,128,151,18,137,196,244,96,48,184,12,17,87,186,174,187,227,232,209,163,219,90,91,91,189,201,148,125,54,202,41,16,20,66,204,71,196,171,165,148,131,225,112,248,169,187,238,186,235,88,169,10,39,34,6,
//0,5,5,130,136,197,141,89,103,161,165,165,229,24,0,252,49,153,76,190,159,203,229,174,171,169,169,89,181,105,211,166,215,239,190,251,238,221,165,40,191,16,101,17,136,235,186,53,158,231,221,64,68,211,178,
//217,236,95,238,187,239,190,157,0,48,102,188,159,12,90,235,233,112,226,155,227,20,2,129,64,78,74,89,91,202,186,252,30,243,92,42,149,90,148,205,102,175,21,66,44,246,60,239,213,230,230,230,161,82,214,3,0,
//80,146,95,210,104,132,16,231,123,158,247,13,68,28,50,12,99,235,125,247,221,247,17,148,88,24,174,235,134,17,241,175,76,211,252,125,254,189,123,238,185,231,3,0,168,75,165,82,23,150,178,78,0,160,134,134,
//134,143,12,195,216,74,68,199,77,211,252,70,87,87,215,188,18,215,81,186,239,16,215,117,57,0,124,65,41,181,88,74,249,122,83,83,211,135,165,42,59,31,33,196,125,0,240,41,231,252,105,207,243,110,98,140,77,
//247,111,237,233,237,237,125,107,233,210,165,139,181,214,49,219,182,219,160,196,63,134,17,146,201,228,34,34,186,14,17,119,217,182,253,26,34,142,25,62,39,66,73,4,226,186,110,56,151,203,45,231,156,51,206,
//249,75,229,252,192,114,93,119,161,82,170,149,115,254,3,41,229,215,136,104,22,17,125,4,0,128,136,23,35,226,1,219,182,159,75,165,82,223,5,128,127,111,104,104,120,181,140,109,153,166,148,186,153,49,38,143,
//31,63,254,226,120,151,238,133,152,244,144,181,101,203,150,90,165,212,109,156,243,52,231,252,233,114,10,131,136,80,41,213,130,136,143,90,150,165,16,241,40,0,204,99,140,45,244,175,122,34,58,2,0,144,205,
//102,187,136,200,106,111,111,15,150,171,61,150,101,245,207,153,51,231,25,41,229,112,40,20,250,186,191,180,159,20,147,234,33,174,235,206,144,82,222,194,24,251,100,245,234,213,175,33,98,89,134,135,17,186,
//186,186,190,193,24,155,107,219,246,6,33,196,121,74,169,115,24,99,51,0,32,236,103,25,214,90,31,241,60,111,79,75,75,203,49,33,196,106,0,200,69,163,209,39,202,217,46,34,66,199,113,174,227,156,207,83,74,245,
//248,139,128,9,49,97,129,108,222,188,121,86,38,147,89,174,181,126,175,177,177,241,237,137,150,83,44,254,55,65,155,97,24,15,73,41,231,3,192,42,68,220,126,154,236,203,136,232,201,108,54,251,81,48,24,252,
//49,17,61,50,153,63,82,177,56,142,115,5,0,44,149,82,190,208,220,220,124,104,34,101,76,104,200,218,176,97,195,140,76,38,179,92,41,245,86,37,132,1,0,16,14,135,239,5,128,199,45,203,74,3,0,103,140,245,43,165,
//14,21,186,0,160,159,49,86,213,210,210,146,81,74,109,97,140,173,174,68,27,109,219,222,6,0,219,12,195,88,158,76,38,235,38,82,198,184,5,226,186,110,77,77,77,205,114,173,245,123,77,77,77,189,19,169,116,188,
//56,142,115,133,214,122,154,109,219,47,39,18,137,208,209,163,71,119,74,41,183,113,206,207,41,116,1,192,182,92,46,183,189,189,189,61,216,216,216,248,162,214,122,110,25,150,193,5,177,109,251,125,0,120,31,
//17,191,214,213,213,21,25,239,243,227,26,178,92,215,13,43,165,110,211,90,247,197,98,177,63,141,183,178,137,224,186,110,64,74,249,15,134,97,252,163,82,234,171,0,112,62,34,22,181,154,33,34,83,107,189,19,
//0,222,70,196,198,114,46,131,243,17,66,92,11,0,231,29,56,112,224,185,117,235,214,13,23,251,92,209,61,196,117,93,238,47,109,143,216,182,253,231,9,181,114,2,72,41,239,68,196,87,45,203,218,71,68,23,112,206,
//69,32,16,40,234,226,156,187,140,177,75,98,177,216,14,34,58,226,56,206,13,149,106,119,67,67,195,235,90,235,99,115,231,206,253,138,175,230,41,138,162,85,39,74,169,171,56,231,108,251,246,237,175,150,123,
//53,53,66,103,103,103,61,34,94,55,48,48,240,32,0,0,17,9,41,229,173,82,202,130,182,144,2,40,34,234,4,0,144,82,118,153,166,249,96,123,123,251,27,107,215,174,45,187,173,3,17,169,167,167,231,149,190,190,190,
//21,27,55,110,252,2,0,188,81,212,115,197,100,114,93,119,174,148,242,38,195,48,158,169,164,85,77,8,241,16,17,253,214,159,44,199,144,74,165,190,78,68,215,131,175,134,39,34,2,128,87,108,219,126,177,80,126,
//199,113,44,68,204,68,163,209,223,150,175,213,167,226,186,238,180,92,46,119,59,0,188,220,216,216,184,247,108,249,207,218,67,58,58,58,170,149,82,95,34,162,215,166,192,196,89,3,0,13,142,227,52,32,226,59,
//209,104,212,25,125,147,136,190,196,57,255,145,101,89,10,224,196,176,170,148,122,24,0,78,10,196,255,154,126,128,136,166,33,162,73,68,111,85,242,5,44,203,234,79,165,82,175,17,209,13,207,60,243,204,83,43,
//86,172,56,163,66,242,108,2,65,211,52,111,32,162,125,177,88,108,103,9,219,89,20,156,243,31,100,50,153,80,32,16,152,163,181,110,5,0,39,47,203,211,82,202,255,38,132,32,128,19,86,68,0,120,42,47,79,152,136,
//206,209,90,255,72,107,173,154,155,155,43,238,212,208,208,208,240,145,16,98,246,225,195,135,175,7,128,127,59,83,222,51,78,54,66,136,249,0,80,55,125,250,244,162,198,191,82,99,89,150,106,106,106,58,174,181,
//142,17,209,73,97,108,221,186,181,74,8,113,23,17,45,2,128,157,68,180,139,136,118,1,192,78,0,88,224,56,206,109,174,235,134,253,50,250,0,96,27,231,252,235,190,186,188,34,243,95,62,156,243,215,0,160,46,153,
//76,206,63,83,190,211,10,36,30,143,155,0,112,149,214,250,141,169,50,248,3,0,164,82,169,59,17,241,147,88,44,246,151,145,180,116,58,125,39,99,236,152,97,24,175,22,186,16,81,43,165,238,26,201,63,56,56,184,
//145,136,174,113,93,119,241,212,188,197,9,19,177,82,234,45,206,249,53,254,223,182,32,167,21,72,36,18,185,146,136,210,177,88,108,87,121,154,120,118,54,109,218,52,159,136,110,202,100,50,169,188,91,31,40,
//165,110,244,87,92,99,46,0,184,30,0,62,30,201,220,218,218,234,33,98,92,74,249,215,229,84,54,158,141,198,198,198,143,0,32,93,85,85,117,249,233,242,20,92,101,117,117,117,69,16,113,101,46,151,123,218,55,99,
//86,156,120,60,110,214,214,214,62,66,68,143,198,98,177,29,221,221,221,193,207,62,251,236,188,80,40,4,0,0,134,97,84,107,173,67,133,158,101,140,101,164,148,67,0,0,153,76,6,206,61,247,220,253,43,87,174,204,
//38,147,201,111,33,98,173,109,219,143,85,240,85,78,193,87,169,172,48,77,243,73,95,13,116,10,5,39,117,195,48,46,145,82,238,153,42,97,0,0,68,34,145,123,136,232,205,88,44,182,163,173,173,141,29,59,118,236,
//65,211,52,123,165,148,35,19,120,81,229,112,206,89,127,127,255,69,109,109,109,143,152,166,185,69,41,245,112,42,149,186,178,161,161,161,162,171,173,17,98,177,216,81,33,196,62,207,243,46,1,128,49,218,142,
//49,67,86,60,30,175,82,74,45,56,131,38,181,236,116,118,118,46,33,162,203,234,235,235,55,3,0,124,245,171,95,101,136,168,137,232,29,173,245,184,46,68,124,91,107,205,235,235,235,185,101,89,138,136,254,89,
//107,221,220,209,209,81,61,85,239,167,181,126,7,0,22,21,106,195,24,129,68,34,145,75,16,241,64,37,212,213,133,232,238,238,14,26,134,177,6,17,255,207,45,183,220,34,1,0,110,185,229,22,201,57,239,0,128,249,
//156,243,133,227,185,0,96,190,105,154,143,142,184,239,216,182,125,144,115,254,108,32,16,104,158,138,247,3,56,209,75,24,99,125,161,80,104,105,254,189,83,230,144,238,238,238,96,127,127,255,157,68,52,41,39,
//182,201,32,132,248,54,34,30,96,140,61,229,47,119,75,237,72,240,94,111,111,239,214,139,47,190,248,251,90,235,23,26,27,27,203,102,226,61,19,174,235,206,84,74,125,141,115,254,196,104,143,252,83,230,16,223,
//215,182,127,170,132,225,171,217,231,238,216,177,227,95,22,47,94,124,41,231,92,70,163,209,127,40,101,29,66,136,166,197,139,23,47,51,12,227,87,82,202,255,145,72,36,222,159,138,185,210,178,172,79,83,169,
//212,128,214,122,1,0,124,48,146,126,138,64,56,231,11,149,82,83,178,204,117,93,183,70,107,221,50,60,60,252,211,182,182,54,221,209,209,177,147,115,190,74,8,241,96,169,235,210,90,111,182,44,107,72,8,177,57,
//24,12,126,27,0,126,86,234,58,138,108,199,46,34,90,8,163,4,114,114,200,114,93,119,154,148,114,101,54,155,125,162,20,222,19,227,69,8,97,19,145,105,24,198,31,0,0,182,111,223,190,187,173,173,109,140,107,141,
//235,186,51,114,185,220,220,98,202,12,4,2,251,10,185,173,118,117,117,69,2,129,192,57,0,0,82,202,251,165,148,201,230,230,230,247,38,251,14,227,37,145,72,132,130,193,224,93,90,235,223,141,248,14,159,156,
//212,61,207,91,8,0,251,167,66,24,0,0,190,210,175,94,41,21,85,74,53,47,89,178,228,187,249,121,132,16,231,121,158,183,150,49,118,89,49,151,82,106,173,16,226,188,209,101,184,174,27,224,156,255,196,175,39,
//202,24,251,52,20,10,29,172,216,139,142,162,165,165,37,131,136,7,252,94,2,0,163,134,44,68,60,63,24,12,22,84,115,87,2,219,182,223,5,128,119,1,0,28,199,105,4,128,49,86,54,206,121,21,17,13,104,173,139,26,
//86,25,99,231,49,198,194,163,211,44,203,202,9,33,14,17,209,147,167,83,235,87,152,143,77,211,188,12,0,222,2,240,123,72,60,30,175,66,196,218,129,129,129,41,249,165,140,166,171,171,235,114,0,88,106,24,198,
//191,230,223,179,44,235,67,173,245,115,156,243,234,98,46,173,245,179,13,13,13,31,229,151,227,121,222,47,24,99,45,19,177,121,151,154,76,38,211,71,68,145,120,60,94,5,224,207,33,201,100,114,17,0,44,141,197,
//98,221,83,217,184,173,91,183,86,13,14,14,254,80,41,245,63,155,154,154,14,140,164,59,142,115,59,34,94,61,209,114,137,72,115,206,95,89,189,122,245,31,70,210,132,16,203,137,232,26,219,182,255,215,100,219,
//61,89,132,16,127,5,0,239,70,163,209,143,13,0,0,68,156,205,24,235,155,226,118,65,58,157,94,3,0,191,27,45,12,223,232,116,99,52,26,125,24,38,168,58,111,107,107,99,75,150,44,105,115,93,247,229,17,99,86,52,
//26,125,33,153,76,94,41,132,88,30,141,70,95,40,201,11,76,156,131,0,48,27,0,62,30,153,67,102,51,198,42,230,184,80,136,141,27,55,126,69,41,101,218,182,125,138,1,199,178,44,37,132,120,223,113,156,31,194,196,
//93,95,53,34,126,48,34,140,17,148,82,27,16,241,17,215,117,223,247,237,38,83,130,82,170,143,115,126,53,0,0,250,95,231,119,103,50,153,205,83,181,194,234,232,232,152,21,8,4,254,59,99,172,173,210,102,226,174,
//174,174,203,25,99,171,13,195,120,56,95,96,149,98,253,250,245,225,250,250,250,187,56,231,143,27,233,116,58,2,0,185,41,92,238,98,42,149,106,245,60,175,179,169,169,169,31,0,160,171,171,107,30,34,174,97,140,
//149,101,147,63,17,73,173,181,211,216,216,184,183,177,177,241,109,199,113,174,150,82,222,9,0,155,202,81,223,217,88,183,110,221,112,50,153,244,136,40,98,104,173,35,254,134,252,41,33,149,74,221,73,68,123,
//155,154,154,78,154,137,25,99,203,0,224,15,158,231,253,123,57,234,12,6,131,95,100,140,93,14,254,246,232,193,193,65,167,182,182,246,145,100,50,185,45,22,139,237,40,71,157,103,195,48,140,1,41,101,196,32,
//162,8,17,13,78,69,35,58,58,58,22,18,209,13,159,125,246,217,223,141,78,175,173,173,125,57,157,78,55,155,166,89,22,199,54,34,58,204,24,75,142,252,191,181,181,213,19,66,252,2,17,215,186,174,251,119,150,101,
//21,237,105,88,42,180,214,131,0,16,49,24,99,181,74,169,195,149,110,64,123,123,123,208,48,140,191,97,140,197,243,29,215,86,173,90,117,28,0,126,89,201,246,68,163,209,61,142,227,252,65,74,25,3,128,13,149,
//172,27,0,0,17,7,24,99,117,134,31,200,165,226,191,136,153,51,103,198,16,241,229,134,134,134,143,124,55,126,11,17,167,44,238,136,82,106,107,52,26,125,50,149,74,253,64,8,113,125,52,26,45,203,112,121,6,134,
//137,168,222,32,34,147,49,86,214,189,215,249,248,106,246,11,118,236,216,241,107,63,233,10,165,212,102,34,170,136,55,125,62,129,64,96,9,99,236,74,68,124,219,117,221,95,40,165,30,74,36,18,189,149,84,203,
//231,114,57,143,115,110,26,68,100,42,165,138,51,80,151,128,142,142,142,106,68,188,207,48,140,31,143,104,115,13,195,232,70,196,187,17,241,203,149,106,199,104,136,232,56,17,61,14,0,96,89,214,17,33,196,227,
//193,96,240,126,0,168,216,87,124,117,117,181,151,203,229,12,3,17,13,206,121,197,122,136,105,154,211,136,136,41,165,190,45,132,0,0,0,41,165,36,162,39,166,106,133,147,76,38,87,50,198,254,102,164,61,0,0,68,
//84,81,61,151,82,74,18,145,105,104,173,141,92,46,87,177,30,18,141,70,247,187,174,251,144,97,24,85,35,105,217,108,246,30,173,245,98,0,56,41,144,182,182,54,182,108,217,178,155,253,0,1,37,3,17,247,174,94,
//189,250,205,209,30,252,156,243,43,180,214,175,134,66,161,119,70,210,164,148,21,181,34,14,13,13,121,129,64,192,156,146,240,76,190,63,82,26,224,196,198,22,34,154,53,119,238,220,248,232,60,139,23,47,254,
//18,17,45,80,74,189,89,202,186,25,99,215,166,82,169,16,0,188,50,42,237,95,180,214,15,166,211,233,119,38,186,55,176,84,24,140,49,25,12,6,167,68,48,190,241,200,54,12,227,71,35,30,38,35,4,2,129,207,148,82,
//95,43,195,130,227,124,41,229,43,163,19,44,203,250,180,171,171,235,81,195,48,190,215,222,222,254,247,149,216,63,146,79,117,117,181,41,165,244,12,34,146,74,169,211,250,154,150,11,215,117,195,82,202,239,
//2,192,175,10,153,89,45,203,234,125,226,137,39,254,183,231,121,19,218,60,121,58,76,211,44,24,4,199,87,161,92,60,115,230,204,53,0,240,207,165,172,179,24,56,231,134,82,202,51,16,209,51,12,163,210,61,4,149,
//82,15,0,192,243,254,38,201,147,196,227,113,51,28,14,7,0,0,6,7,7,61,0,40,233,16,146,201,100,96,196,65,109,120,120,56,55,58,220,146,109,219,155,133,16,223,115,28,231,54,219,182,159,43,101,189,103,99,104,
//104,200,228,156,75,195,143,222,89,209,30,146,76,38,239,2,128,92,254,75,39,147,201,255,132,136,55,50,198,42,162,241,53,12,35,40,132,248,211,40,123,8,113,206,127,41,165,108,115,28,103,79,254,143,165,156,
//4,2,1,83,107,237,25,136,152,65,196,240,217,31,41,13,190,186,251,218,79,63,253,244,239,11,220,190,136,49,246,155,64,32,80,17,85,78,38,147,153,161,181,94,1,0,47,140,164,89,150,53,44,132,248,39,0,248,190,
//235,186,63,42,69,176,181,34,9,35,98,198,144,82,14,32,98,69,214,220,174,235,206,148,82,254,151,96,48,248,227,66,19,167,82,106,19,0,172,204,100,50,149,26,66,61,165,148,200,79,140,70,163,251,133,16,142,148,
//242,187,61,61,61,63,204,95,112,148,3,34,138,104,173,7,13,207,243,6,67,161,208,172,114,87,232,186,110,64,41,245,61,198,216,175,191,245,173,111,21,156,23,252,37,231,175,203,221,150,98,136,70,163,175,9,33,
//46,222,191,127,127,35,84,160,77,140,177,90,173,117,159,81,85,85,53,160,148,42,123,15,241,60,239,126,68,124,53,26,141,190,229,247,148,191,174,212,246,234,241,226,71,29,122,180,161,161,65,8,33,254,107,103,
//103,231,242,166,166,166,23,202,89,167,148,50,18,14,135,251,13,0,232,7,128,64,34,145,8,149,203,106,152,76,38,87,34,98,149,109,219,79,2,0,228,114,185,107,57,231,207,79,129,70,181,40,132,16,215,114,206,175,
//71,196,127,117,93,247,231,136,248,176,235,186,187,45,203,42,139,155,237,250,245,235,195,140,49,83,74,57,104,88,150,149,115,28,231,184,105,154,231,130,111,65,43,37,157,157,157,75,16,113,185,97,24,39,189,
//70,16,241,85,34,90,35,132,184,181,212,245,149,2,34,242,0,32,1,112,66,171,176,105,211,166,159,103,179,217,181,91,182,108,105,187,227,142,59,74,110,204,155,53,107,214,57,0,144,182,44,43,55,18,196,190,143,
//136,102,67,137,5,146,72,36,166,115,206,31,64,196,159,141,182,194,249,123,79,166,196,193,121,34,220,125,247,221,187,29,199,249,237,240,240,240,223,2,192,79,74,93,62,231,124,14,156,112,5,58,225,74,234,135,
//52,26,179,121,100,178,4,131,193,11,16,145,16,241,158,84,42,149,209,90,143,246,234,240,0,224,119,197,108,125,240,163,156,126,31,17,3,19,105,7,34,102,181,214,237,197,108,66,114,28,103,41,0,220,156,151,28,
//96,140,153,90,235,5,237,237,237,193,50,168,86,102,131,239,70,107,0,0,164,211,233,190,72,36,114,67,169,231,17,219,182,183,117,118,118,254,204,52,205,90,41,229,41,187,95,25,99,183,34,226,149,0,48,38,178,
//104,62,82,202,69,140,177,247,60,207,123,114,34,237,48,77,243,27,0,112,17,20,216,211,87,128,85,0,240,190,214,250,227,188,246,30,15,135,195,143,149,90,24,137,68,34,4,0,211,56,231,255,209,67,90,91,91,143,
//59,142,51,24,137,68,102,195,137,88,232,37,195,247,66,60,48,58,77,8,113,62,0,204,229,156,199,11,63,117,42,134,97,188,171,148,186,50,16,8,172,153,72,27,180,214,135,211,233,116,81,90,99,41,229,227,134,97,
//60,112,228,200,145,103,43,161,100,172,174,174,174,207,229,114,3,209,104,116,24,96,148,247,59,17,237,201,102,179,11,160,196,2,201,39,30,143,155,68,244,29,34,250,213,72,35,206,134,239,192,214,89,206,118,
//141,208,220,220,188,203,113,156,63,206,156,57,51,6,0,143,150,187,62,165,212,124,198,216,39,35,255,63,41,16,211,52,119,73,41,87,150,115,249,11,0,16,137,68,98,90,235,63,23,178,14,10,33,206,39,162,47,32,
//98,69,206,53,33,34,210,90,191,153,31,165,39,26,141,110,73,165,82,15,165,82,169,171,27,26,26,202,22,86,36,145,72,132,136,168,94,107,253,31,62,105,35,255,240,93,56,143,26,134,113,198,88,28,147,193,247,46,
//89,176,99,199,142,49,91,13,124,238,7,128,221,74,169,93,149,184,0,96,55,231,124,204,48,136,136,148,203,229,126,65,68,141,137,68,162,164,22,203,209,132,195,225,133,140,177,195,163,79,94,56,69,103,196,24,
//219,9,0,139,0,160,228,222,31,91,182,108,169,29,26,26,106,225,156,255,180,208,86,53,159,126,198,216,13,80,32,200,126,57,96,140,49,34,42,168,89,190,247,222,123,15,59,142,179,57,24,12,62,0,0,63,133,50,4,
//173,81,74,45,132,81,102,107,128,60,129,68,34,145,221,253,253,253,87,57,142,115,110,169,119,226,14,13,13,221,239,111,1,62,173,151,121,111,111,239,63,45,89,178,100,110,165,236,51,74,41,217,219,219,187,239,
//116,247,109,219,126,201,113,156,171,146,201,228,173,177,88,236,249,82,214,237,111,139,174,229,156,159,50,103,143,25,171,125,181,65,181,101,89,5,163,178,77,4,255,139,252,138,104,52,250,143,165,42,179,82,
//248,155,136,30,65,196,245,209,104,116,127,169,202,21,66,44,215,90,15,198,98,177,215,71,167,143,249,37,14,12,12,108,175,169,169,249,230,134,13,27,102,172,89,179,102,210,182,0,199,113,102,19,209,74,95,117,
//114,10,174,235,206,201,229,114,51,39,91,71,41,9,4,2,159,142,238,197,171,86,173,58,238,56,206,6,34,250,78,79,79,79,91,41,84,241,201,100,178,78,107,61,39,157,78,143,249,46,58,93,52,160,27,24,99,134,109,
//219,47,77,166,226,145,144,123,136,232,54,52,52,188,147,119,111,142,175,241,253,203,233,158,159,34,174,202,100,50,241,150,150,150,83,134,86,33,132,173,181,214,177,88,44,63,84,212,184,17,66,124,89,107,157,
//45,20,106,183,224,142,164,218,218,218,119,137,104,222,68,163,51,143,32,165,188,91,107,221,155,47,12,0,128,92,46,103,32,162,4,128,161,207,211,69,68,178,186,186,122,76,212,83,206,249,70,0,184,212,113,156,
//75,39,243,55,217,176,97,195,12,173,245,92,211,52,11,6,247,41,40,144,59,238,184,99,208,52,205,29,134,97,124,113,34,7,108,1,156,212,242,94,157,78,167,221,66,247,27,27,27,247,114,206,133,31,73,244,115,1,
//17,145,148,210,177,44,107,204,68,111,89,150,50,77,243,151,136,120,255,68,35,9,17,17,214,212,212,92,203,24,235,45,20,43,11,224,12,65,48,143,30,61,186,45,18,137,252,231,141,27,55,46,2,128,49,91,139,207,
//68,34,145,8,113,206,239,231,156,255,252,76,135,104,249,246,133,41,139,88,55,94,44,203,218,151,74,165,158,54,77,243,62,152,128,171,208,111,126,243,155,11,149,82,53,233,116,186,231,116,121,206,248,235,23,
//66,44,0,128,107,167,77,155,182,117,60,113,23,29,199,249,14,0,236,180,109,251,233,182,182,54,182,104,209,162,138,57,81,148,139,186,186,58,242,247,173,96,42,149,250,190,148,242,149,198,198,198,151,139,125,
//222,15,153,190,74,74,249,231,230,230,230,79,78,151,239,172,195,145,16,226,86,34,58,110,219,246,31,139,169,56,153,76,126,17,17,111,181,109,251,39,241,120,220,136,68,34,15,34,226,148,69,166,43,21,90,107,
//68,196,224,192,192,192,250,234,234,234,48,99,236,97,0,248,113,177,223,107,93,93,93,55,153,166,25,88,189,122,245,105,123,7,64,17,129,148,7,6,6,254,24,137,68,86,38,147,201,69,103,139,221,235,186,238,12,
//165,84,67,40,20,122,4,0,40,16,8,84,3,128,199,24,171,88,36,233,114,193,24,3,41,165,93,87,87,87,101,89,86,127,50,153,236,100,140,253,109,91,91,219,15,207,160,121,0,0,128,206,206,206,197,156,243,250,233,
//211,167,231,199,20,30,67,81,19,246,99,143,61,54,55,16,8,220,172,181,126,246,116,70,30,127,55,237,15,148,82,207,143,14,10,230,247,152,138,28,21,81,110,24,99,31,54,52,52,156,220,207,239,56,206,26,68,60,
//116,166,208,229,137,68,98,122,40,20,90,161,148,42,42,212,120,209,43,40,215,117,175,145,82,158,55,56,56,248,116,161,137,218,113,156,85,136,56,39,26,141,254,223,98,203,252,255,157,246,246,246,224,185,231,
//158,251,67,198,88,188,80,76,149,158,158,30,99,223,190,125,183,115,206,247,69,163,209,162,236,49,69,71,70,184,231,158,123,222,228,156,231,106,106,106,190,148,191,20,118,93,247,2,68,252,74,38,147,169,136,
//205,226,243,194,218,181,107,179,68,244,75,173,117,107,126,112,228,182,182,54,118,224,192,129,155,12,195,200,245,246,246,22,29,1,181,104,37,30,34,234,68,34,241,98,48,24,188,45,149,74,125,17,0,78,186,240,
//16,209,50,34,146,161,80,168,201,113,156,137,126,87,20,115,106,102,218,48,140,158,211,173,225,243,113,93,247,2,207,243,110,60,155,125,133,136,216,68,221,105,17,209,0,128,154,186,186,186,89,0,48,242,253,
//130,75,150,44,185,14,0,166,13,15,15,63,123,182,57,230,148,242,198,219,0,63,20,223,215,181,214,31,142,138,55,133,169,84,106,169,148,114,220,90,90,206,121,213,233,206,63,47,144,247,26,173,245,246,88,44,
//118,198,149,202,8,254,89,134,31,73,41,139,90,9,113,206,165,82,106,220,102,91,68,28,136,197,98,39,181,182,169,84,234,74,173,245,226,234,234,234,103,199,235,54,52,161,175,240,100,50,89,135,136,183,1,192,
//182,74,122,136,251,199,224,237,137,70,163,175,156,53,51,156,248,30,50,12,227,247,150,101,85,108,119,111,42,149,90,6,0,151,41,165,158,159,72,168,221,9,69,215,137,197,98,71,165,148,47,0,192,21,190,21,176,
//82,84,35,226,120,14,4,30,210,90,87,42,96,50,118,118,118,94,9,0,151,49,198,94,152,104,220,227,9,159,244,217,220,220,124,40,155,205,62,171,181,190,48,149,74,221,208,214,214,86,242,131,142,243,33,162,106,
//198,88,209,2,65,196,33,68,156,244,233,155,103,131,136,48,153,76,94,103,154,230,133,74,169,231,45,203,250,116,162,101,77,234,143,216,210,210,114,44,16,8,60,71,68,179,46,186,232,162,155,123,122,122,202,
//109,233,171,1,127,179,104,49,16,81,154,136,202,218,67,122,122,122,12,33,196,87,252,32,112,167,253,78,43,150,73,255,170,45,203,74,31,56,112,224,57,195,48,194,125,125,125,43,92,215,157,54,217,50,207,64,
//117,48,24,28,87,15,41,167,64,18,137,196,244,131,7,15,222,206,24,11,102,179,217,231,138,93,253,157,137,146,12,51,235,214,173,27,126,239,189,247,158,227,156,127,162,148,186,61,145,72,148,220,45,213,167,
//58,155,205,22,29,15,69,74,57,164,181,46,203,144,149,76,38,23,153,166,185,130,49,118,96,245,234,213,207,151,202,117,170,100,67,140,191,214,126,187,171,171,235,104,40,20,186,65,8,49,131,115,254,218,232,
//184,230,147,5,17,217,56,163,190,13,49,198,74,218,67,186,187,187,131,71,142,28,185,150,49,86,159,203,229,94,42,100,59,153,12,37,159,136,27,27,27,247,122,158,247,20,17,133,148,82,223,76,165,82,23,78,212,
//200,149,7,142,215,152,21,12,6,75,54,135,16,17,186,174,187,184,191,191,127,149,105,154,129,129,129,129,238,123,239,189,183,164,194,0,40,97,15,25,141,127,248,214,191,37,147,201,249,140,177,171,55,110,220,
//120,97,50,153,252,243,100,38,188,120,60,30,38,162,113,13,11,67,67,67,67,161,80,104,210,67,214,134,13,27,102,56,142,115,29,17,85,105,173,255,116,38,123,198,100,41,235,170,40,22,139,237,142,199,227,251,
//167,77,155,118,57,0,172,16,66,236,211,90,191,51,17,193,212,213,213,85,43,165,198,53,105,230,114,185,161,96,48,56,225,30,178,97,195,134,25,213,213,213,151,106,173,231,106,173,63,152,55,111,222,182,114,
//111,0,45,187,67,154,175,25,126,227,153,103,158,233,237,239,239,95,170,181,94,225,56,206,97,56,241,149,95,116,36,109,165,84,53,20,167,239,58,165,110,33,196,184,223,209,117,221,153,0,112,137,231,121,245,
//68,180,187,166,166,230,119,229,216,57,85,136,138,69,112,240,79,184,124,61,145,72,188,27,8,4,150,50,198,150,11,33,210,156,243,93,123,247,238,221,117,182,19,149,137,168,154,136,198,37,144,241,224,186,110,
//0,0,46,144,82,46,244,60,239,28,0,216,153,78,167,127,219,218,218,90,209,40,119,21,15,58,227,47,15,255,226,186,238,118,173,245,2,207,243,22,206,153,51,231,74,33,196,1,0,248,56,147,201,244,21,90,66,34,98,
//13,34,142,123,157,79,68,217,238,238,238,96,33,159,128,245,235,215,135,231,205,155,55,71,41,53,95,74,89,207,24,59,204,24,251,152,49,246,98,41,87,135,227,97,74,162,0,1,156,56,165,0,78,28,100,242,65,87,87,
//87,132,136,22,154,166,121,89,40,20,186,81,8,209,15,0,7,149,82,125,135,14,29,58,188,110,221,186,225,241,170,77,70,64,196,161,116,58,93,13,0,217,245,235,215,135,103,205,154,117,142,191,167,111,54,0,76,203,
//229,114,3,68,180,7,17,223,176,109,123,202,194,229,142,80,145,125,24,227,193,117,221,176,82,106,182,82,106,142,175,142,168,70,68,15,0,206,35,162,65,68,252,19,0,12,231,114,57,15,17,61,206,185,151,205,102,
//37,192,137,16,71,217,108,214,32,34,51,16,8,152,112,226,28,220,59,25,99,31,208,9,76,56,161,122,57,8,0,7,57,231,7,167,34,36,236,153,248,220,9,36,31,223,125,38,226,135,220,139,248,97,109,195,136,104,48,198,
//12,0,8,104,173,71,118,19,75,0,200,105,173,37,34,122,136,152,241,227,225,14,32,226,128,97,24,3,83,53,20,21,203,255,3,81,181,200,18,187,126,56,59,0,0,0,0,73,69,78,68,174,66,96,130,0,0};

//const char* UI::g245_png = (const char*) resource_UI_g245_png;
//const int UI::g245_pngSize = 7383;


//[EndFile] You can add extra defines here...
//[/EndFile]

