#pragma once

enum class FontStyle
{
    Light,
    Regular,
    Bold
};

static const Font getCustomFont(FontStyle style)
{
    if (style == FontStyle::Light) {
        static auto typeface = Typeface::createSystemTypefaceFor(BinaryData::SoraThin_ttf,
            BinaryData::SoraThin_ttfSize);

        return Font(typeface);
    }
    else if (style == FontStyle::Regular) {
        static auto typeface = Typeface::createSystemTypefaceFor(BinaryData::SoraRegular_ttf,
            BinaryData::SoraRegular_ttfSize);

        return Font(typeface);
    }
    else if (style == FontStyle::Bold) {
        static auto typeface = Typeface::createSystemTypefaceFor(BinaryData::SoraSemiBold_ttf,
            BinaryData::SoraSemiBold_ttfSize);

        return Font(typeface);
    }
    
}

class InputMeterLNF : public foleys::LevelMeterLookAndFeel
{
public:

    juce::Rectangle<float> getMeterClipIndicatorBounds(juce::Rectangle<float> bounds,
        foleys::LevelMeter::MeterFlags meterType) const override
    {
        Rectangle<float> rect;
        return rect;
    }

};

class OutputMeterLNF : public foleys::LevelMeterLookAndFeel
{
    juce::Rectangle<float> getMeterClipIndicatorBounds(juce::Rectangle<float> bounds,
        foleys::LevelMeter::MeterFlags meterType) const override
    {
        const auto margin = bounds.getWidth() * 0.05f;
        const auto w = bounds.getWidth() - margin * 2.0f;
        return juce::Rectangle<float>(bounds.getX() + margin,
            bounds.getY() + margin - 1, w, w * 0.55f);
    }

};

class CrossoverSliderLNF : public LookAndFeel_V4
{

public:

    Slider::SliderLayout getSliderLayout(Slider& slider) override
    {
        Slider::SliderLayout layout;

        auto bounds = slider.getLocalBounds();

        layout.sliderBounds = bounds.removeFromBottom(125);
        layout.textBoxBounds = { labelPos, 35, 65, 20 };
        return layout;
    }

    void drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
        float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider& slider) override
    {
        float thumbX = jmin(float(width - 3), jmax(sliderPos, 1.f));
        Rectangle<float> sliderThumb{ thumbX - 1.5f, float(y), 3.f, 125.f };
        g.setColour(Colours::floralwhite);
        g.fillRoundedRectangle(sliderThumb, 3.f);

        sliderValue = slider.getValue();

        if (slider.isMouseOverOrDragging()) {
            g.setColour(Colours::navajowhite);
            g.drawRoundedRectangle(sliderThumb.expanded(1.f), 3.f, 2.f);
        }

        lastSliderPos = thumbX;
        sliderWidth = width;
    }

    void drawLabel(Graphics& g, Label& label) override
    {
        if (!label.isBeingEdited())
        {
            g.setColour(Colours::floralwhite);
            g.setFont(getCustomFont(FontStyle::Bold).withHeight(15.f).withExtraKerningFactor(0.05f));

            auto textArea = label.getLocalBounds();
            labelPos = jmin(sliderWidth - textArea.getWidth() - 15,
                jmax(int(lastSliderPos) - (textArea.getWidth() / 2), 15));

            label.setBounds(labelPos, 35, 65, 20);

            if (sliderValue < 1000.f)
            {
                String labelText(label.getText()); labelText.append(" Hz", 3);
                g.drawText(labelText, textArea, Justification::centred, false);
            }
            else
            {
                String labelText(sliderValue / 1000.f, 2); labelText.append(" kHz", 4);
                g.drawText(labelText, textArea, Justification::centred, false);
            }
        }
    }

    void drawTextEditorOutline(Graphics& g, int width, int height, TextEditor& editor) override
    {
    }
    void fillTextEditorBackground(Graphics& g, int width, int height, TextEditor& editor) override
    {
        g.fillAll(Colours::black);
    }

private:
    float lastSliderPos = 0.0;
    float sliderValue = 0.0;
    int labelPos = 0, sliderWidth = 0;

};

class GainSliderLNF : public LookAndFeel_V4
{
    float lastSliderPos = 0.0;
    float labelPos = 0.0;
    Rectangle<int> sliderBounds;

public:
    bool textOnLeft;


    Slider::SliderLayout getSliderLayout(Slider& slider) override
    {
        Slider::SliderLayout layout;

        auto bounds = slider.getLocalBounds();
        sliderBounds = bounds;
        auto s = bounds.removeFromTop(slider.getHeight() - 11);
        Rectangle<int> sliderBounds, textBoxBounds;

        if (!textOnLeft) {
            sliderBounds = s.removeFromLeft(40);
            //s.expand(0, 10);
            textBoxBounds = { 50, (int)labelPos, 35, 15 };
            //textBoxBounds = s;
        }
        else if (textOnLeft) {
            sliderBounds = s.removeFromRight(40);
            //s.expand(0, 10);
            textBoxBounds = { 10, (int)labelPos, 35, 15 };
            //textBoxBounds = s;
        }

        layout.sliderBounds = sliderBounds;
        layout.textBoxBounds = textBoxBounds;

        return layout;
    }

    void drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
        float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider& slider) override
    {
        /*g.setColour(Colours::transparentWhite);
        g.fillRect(x, y, width, height);*/

        Rectangle<float> sliderThumb{ float(x) + 1.f, sliderPos, float(width) * 0.95f, 5.f };
        g.setColour(Colours::floralwhite);
        g.fillRoundedRectangle(sliderThumb, 3.f);

        if (slider.isMouseOverOrDragging()) {
            g.setColour(Colours::navajowhite);
            g.drawRoundedRectangle(sliderThumb.expanded(1.f), 3.f, 2.f);
        }

        lastSliderPos = sliderPos;
    }

    void drawLabel(Graphics& g, Label& label) override
    {
        label.setPaintingIsUnclipped(true);
        if (!label.isBeingEdited())
        {
            //const Font font(Font(Font::getDefaultSansSerifFontName(), 13.f, Font::plain));
            g.setColour(Colours::floralwhite);
#if JUCE_WINDOWS
            g.setFont(getCustomFont(FontStyle::Regular).withHeight(14.f));
#elif JUCE_MAC
            g.setFont(getCustomFont(FontStyle::Regular).withHeight(12.f));
#endif
            
            auto textArea = label.getLocalBounds();

            labelPos = jmax(int(lastSliderPos - textArea.getCentreY()),
                textArea.getY() - textArea.getCentreY()) + 3;
            if (!textOnLeft)
                label.setBounds(50, labelPos, 35, 15);
            else
                label.setBounds(10, labelPos, 35, 15);

            String labelText(label.getText()); labelText.append(" dB", 3);

            g.drawFittedText(labelText, label.getLocalBounds(), Justification::centred, 1, 0.5f);

        }

        else if (label.isBeingEdited())
        {
            auto editor = label.getCurrentTextEditor();

            editor->applyFontToAllText(getCustomFont(FontStyle::Regular).withHeight(13.f));
            editor->setPaintingIsUnclipped(true);

            /*const Font font(juce::Font(Font::getDefaultSansSerifFontName(), 12.f, juce::Font::plain).withTypefaceStyle("Regular"));

            g.setColour(Colours::floralwhite);
            g.setFont(font);*/

            auto textArea = label.getLocalBounds();

            auto ylimit = jmax(sliderBounds.getY(), textArea.getY());

            //int labelPos = lastSliderPos + 7;

            /*if (!textOnLeft)
                editor->setBounds(0, labelPos, 50, 20);*/
            //else
            editor->setBounds(textArea.getX(), ylimit, textArea.getWidth(), textArea.getHeight());

            //auto ylimit = jlimit(textArea.getY() + 15, textArea.getBottom() - 25, editor->getY());
            //Rectangle<int> constraints(editor->getX(), labelPos, editor->getWidth(), editor->getHeight());
            //editor->setBounds(editor->getBounds().constrainedWithin(constraints));

            g.setColour(Colours::black);
            g.fillRect(editor->getBounds());


        }

    }

    void drawTextEditorOutline(Graphics& g, int width, int height, TextEditor& editor) override
    {
    }
    void fillTextEditorBackground(Graphics& g, int width, int height, TextEditor& editor) override
    {
    }

};

struct ComboBoxLNF : public LookAndFeel_V4
{
    enum class Type
    {
        Preset,
        HQ,
        Clip
    };

    Type type;

    void setType(Type newType)
    {
        type = newType;
    }
    //ComboBoxLNF()
    //{
    //    setDefaultSansSerifTypeface(getTypefaceForFont(getCustomFont(FontStyle::Regular)));
    //}

    //Typeface::Ptr getTypefaceForFont(const Font& f)
    //{
    //    return Typeface::createSystemTypefaceFor(f);
    //}

    void drawComboBox(Graphics& g, int width, int height, bool,
        int, int, int, int, ComboBox& box) override
    {
        //auto cornerSize = box.findParentComponentOfClass<ChoicePropertyComponent>() != nullptr ? 0.0f : 3.0f;
        Rectangle<int> boxBounds(0, 0, width, height);

        if (type == Type::HQ || type == Type::Preset) {
            if (box.getSelectedId() == 1) {
                g.setColour(Colour(0xa7a7a7a7));
                g.drawRoundedRectangle(boxBounds.toFloat().reduced(1.f), 7.f, 1.f);
            }
            else {
                g.setColour(Colours::ghostwhite);
                g.fillRoundedRectangle(boxBounds.toFloat(), 7.f);
            }
        }
        else {
            Path p;
            p.addRoundedRectangle((float)boxBounds.getX(), (float)boxBounds.getY(),
                (float)boxBounds.getWidth(), (float)boxBounds.getHeight(),
                7.f, 7.f, false, true, false, true);
            if (box.getSelectedId() == 1)
                g.setColour(Colours::seagreen);
            else if (box.getSelectedId() == 2)
                g.setColour(Colours::seagreen.withAlpha(0.33f));
            else
                g.setColour(Colours::teal.withAlpha(0.5f));

            g.fillPath(p);

        }
    }

    void positionComboBoxText(ComboBox& box, Label& label) override
    {
        label.setBounds(1, 1,
            box.getWidth(),
            box.getHeight() - 2);

        label.setFont(getComboBoxFont(box));
    }

    void drawLabel(Graphics& g, Label& label) override
    {
        String text;
        if (type == Type::HQ) {
            text = "HQ: ";
            text.append(label.getText(), 3);
        }
        else if (type == Type::Preset) {
            text = "Preset: ";
            text.append(label.getText(), 24);
        }
        else
            text = label.getText();

        if (type == Type::HQ)
            text.contains("Off") ? g.setColour(Colour(0xa7a7a7a7)) : g.setColour(Colours::black);
        else
            g.setColour(Colours::floralwhite);
        //const Font font{ Font::getDefaultSansSerifFontName(), 15.f, Font::plain };
        g.setFont(getCustomFont(FontStyle::Regular).withHeight(15.f));
        g.drawFittedText(text, label.getLocalBounds(), type == Type::Preset ? Justification::centredLeft :
            Justification::centred, 1, 1.f);

    }

    void drawPopupMenuBackground(Graphics& g, int width, int height) override
    {
        g.fillAll(Colours::black);
    }

};

struct BottomButtonLNF : public LookAndFeel_V4
{
    //BottomButtonLNF()
    //{
    //    setDefaultSansSerifTypeface(getTypefaceForFont(getCustomFont(FontStyle::Regular)));
    //}

    //Typeface::Ptr getTypefaceForFont(const Font& f)
    //{
    //    return Typeface::createSystemTypefaceFor(f);
    //}

    void drawButtonBackground(Graphics& g, Button& b, const Colour& backgroundColour,
        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = b.getLocalBounds();
        g.setColour(Colours::transparentBlack);
        g.fillRect(bounds);
        g.setColour(Colour(0xa7a7a7a7));
        g.drawRoundedRectangle(bounds.toFloat().reduced(1.f), 7.f, 1.f);

        if (b.isMouseOver()) {
            g.setColour(Colour(0xa7a7a7a7));
            g.fillRoundedRectangle(bounds.toFloat().reduced(1.f), 7.f);
        }
        if (b.getToggleState()) {
            g.setColour(Colours::ghostwhite);
            g.fillRoundedRectangle(bounds.toFloat(), 7.f);
        }
    }

    void drawButtonText(Graphics& g, TextButton& b, bool , bool ) override
    {
        if (b.isMouseOver() && !b.getToggleState())
            g.setColour(Colours::white);
        else if (b.getToggleState())
            g.setColour(Colours::black);
        else
            g.setColour(Colours::grey);

#if JUCE_WINDOWS
        g.setFont(getCustomFont(FontStyle::Regular).withHeight(15.f));
#elif JUCE_MAC
        g.setFont(getCustomFont(FontStyle::Regular).withHeight(13.f));
#endif
        g.drawFittedText(b.getButtonText(), b.getLocalBounds(), Justification::centred, 2, 0.75f);
    }

};

struct TopButtonLNF : public LookAndFeel_V4
{

    enum class Type
    {
        Regular,
        Toggle,
        Auto,
        Bypass
    };

    Type type;

    void setType(Type newType)
    {
        type = newType;
    }
    /*TopButtonLNF()
    {
        setDefaultSansSerifTypeface(getTypefaceForFont(getCustomFont(FontStyle::Regular)));
    }

    Typeface::Ptr getTypefaceForFont(const Font& f)
    {
        return Typeface::createSystemTypefaceFor(f);
    }*/

    void drawButtonBackground(Graphics& g, Button& b, const Colour& backgroundColour,
        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override
    {
        auto bounds = b.getLocalBounds();

        if (type == Type::Regular) {
            if (b.isMouseOver() && !b.getToggleState()) {
                g.setColour(Colour(0xa7a7a7a7).withAlpha(0.75f));
                g.fillRoundedRectangle(bounds.toFloat(), 7.f);
            }
            else if (b.getToggleState()) {
                g.setColour(Colours::seagreen.withAlpha(0.33f));
                g.fillRoundedRectangle(bounds.toFloat(), 7.f);
            }
            else {
                g.setColour(Colour(0xa7a7a7a7));
                g.drawRoundedRectangle(bounds.toFloat().reduced(2.f), 7.f, 1.f);
            }
        }
        else if (type == Type::Toggle) {
            if (b.getToggleState())
                g.setColour(Colours::seagreen.withAlpha(0.33f));
            else
                g.setColour(Colours::seagreen);

            Path p;
            p.addRoundedRectangle((float)bounds.getX(), (float)bounds.getY(),
                (float)bounds.getWidth(), (float)bounds.getHeight(),
                7.f, 7.f, true, false, true, false);
            g.fillPath(p);
        }
        else if (type == Type::Auto) {
            if (b.isMouseOver() && !b.getToggleState()) {
                g.setColour(Colour(0xa7a7a7a7).withAlpha(0.75f));
                g.fillRoundedRectangle(bounds.toFloat(), 7.f);
            }
            else if (b.getToggleState()) {
                g.setColour(Colours::teal.withAlpha(0.5f));
                g.fillRoundedRectangle(bounds.toFloat(), 7.f);
            }
            else {
                g.setColour(Colour(0xa7a7a7a7));
                g.drawRoundedRectangle(bounds.toFloat().reduced(2.f), 7.f, 1.f);
            }
        }
        else if (type == Type::Bypass) {
            if (b.isMouseOver() && !b.getToggleState()) {
                g.setColour(Colour(0xa7a7a7a7).withAlpha(0.75f));
                g.fillRoundedRectangle(bounds.toFloat(), 7.f);
            }
            else if (b.getToggleState()) {
                g.setColour(Colours::mediumorchid.withAlpha(0.33f));
                g.fillRoundedRectangle(bounds.toFloat(), 7.f);
            }
            else {
                g.setColour(Colour(0xa7a7a7a7));
                g.drawRoundedRectangle(bounds.toFloat().reduced(2.f), 7.f, 1.f);
            }
        }

    }

    void drawButtonText(Graphics& g, TextButton& b, bool /**/, bool /**/) override
    {
        //const Font font{ Font::getDefaultSansSerifFontName(), 15.f, Font::plain };
        if (b.isMouseOver() || b.getToggleState() || type == Type::Toggle)
            g.setColour(Colours::floralwhite);
        else
            g.setColour(Colour(0xa7a7a7a7));
#if JUCE_WINDOWS
        g.setFont(getCustomFont(FontStyle::Regular).withHeight(15.f));
#elif JUCE_MAC
        g.setFont(getCustomFont(FontStyle::Regular).withHeight(14.f));
#endif
        g.drawFittedText(b.getButtonText(), b.getLocalBounds(), Justification::centred, 2, 1.f);
    }

};

class KnobLNF : public LookAndFeel_V4

{
public:

    enum class LabelType
    {
        Width,
        Mix,
        InGain,
        OutGain
    };

    void setLabelType(LabelType newType)
    {
        type = newType;
    }

    void setImage(const Image& newImage)
    {
        img = newImage;
    }

    bool painted = false;
    bool altDown = false;

private:

    bool mouseOver = false;
    bool mono = false;

    LabelType type;

    Image img;

    Slider::SliderLayout getSliderLayout(Slider& slider) override
    {
        Slider::SliderLayout layout;

        auto bounds = slider.getLocalBounds();

        layout.sliderBounds = bounds.removeFromBottom(60);
        layout.textBoxBounds = bounds.translated(0, 7);

        return layout;
    }

    void drawRotarySlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, Slider& slider) override
    {
        auto radius = (float)jmin(width / 2, height / 2) - 4.0f;
        auto centerX = (float)x + (float)width * 0.5f;
        auto centerY = (float)y + (float)height * 0.5f;
        auto rx = centerX - radius;
        auto ry = centerY - radius;
        auto rw = radius * 2.f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        g.setColour(Colour(0xff30414d));
        g.fillEllipse(rx, ry, rw, rw);
        ColourGradient gradient(Colour(0xa7a7a7a7), centerX + 7, centerY + 7, Colour(0x00000000), x, y, true);
        g.setGradientFill(gradient);
        g.fillEllipse(rx, ry, rw, rw);

        Path p;
        auto pointerLength = radius * 0.75;
        auto pointerThickness = 2.0f;
        p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        p.applyTransform(AffineTransform::rotation(angle).translated(centerX, centerY));

        g.setColour(Colour(0xffa7a7a7));
        g.fillPath(p);

        if (slider.isMouseOverOrDragging())
        {
            g.setColour(Colour(0xffa7a7a7));
            g.drawEllipse(rx, ry, rw, rw, 2.0f);
            mouseOver = true;
        }
        else
            mouseOver = false;

        Rectangle<int> textBounds{ x, (int)centerY - 4, width, 10 };
        g.setFont(getCustomFont(FontStyle::Regular).withHeight(12.f));
        g.setColour(Colours::floralwhite);
        if (type == LabelType::Width) {
            if (altDown) {
                g.setColour(Colours::burlywood);
                g.drawFittedText("M Width", textBounds, Justification::centred, 1, 1.f);
            }
            else
                g.drawFittedText("Width", textBounds, Justification::centred, 1, 1.f);
        }
        else if (type == LabelType::Mix) {
            if (altDown) {
                g.setColour(Colours::burlywood);
                g.drawFittedText("Delta", textBounds, Justification::centred, 1, 1.f);
            }
            else
                g.drawFittedText("Mix", textBounds, Justification::centred, 1, 1.f);
        }
        else if (type == LabelType::InGain)
            g.drawFittedText("In", textBounds, Justification::centred, 1, 1.f);
        else if (type == LabelType::OutGain)
            g.drawFittedText("Out", textBounds, Justification::centred, 1, 1.f);
    }

    void drawLabel(Graphics& g, Label& label) override
    {
        g.setFont(getCustomFont(FontStyle::Regular).withHeight(15.f));
        if (mono)
            g.setColour(Colours::palevioletred);
        else
            g.setColour(Colours::floralwhite);

        auto textBounds = label.getLocalBounds();
        auto text = label.getText();
        if (type == LabelType::Mix || type == LabelType::Width)
            text.append("%", 1);
        else if (type == LabelType::InGain || type == LabelType::OutGain)
            text.append(" dB", 3);
        if (mouseOver || label.isMouseOver()) {
            g.drawFittedText(text, textBounds, Justification::centred, 1, 1.f);
        }
    }

    void drawBubble(Graphics& g, BubbleComponent& comp , const Point< float >& tip,
        const Rectangle< float >& body) override
    {
        Rectangle<int> imgBounds{ comp.getX() + (int)body.getX(), comp.getY() + (int)body.getY(),
        (int)body.getWidth(), (int)body.getHeight() };
        auto clipimg = img.getClippedImage(imgBounds);
        
        if (!painted) {
            painted = true;
            gin::applyStackBlur(clipimg, 10);
            gin::applyBrightnessContrast(clipimg, -25, -25);
        }

        g.setColour(Colours::darkslategrey.withSaturation(0.35f));
        g.fillRoundedRectangle(body, 3.f);
        g.drawImage(clipimg, body, RectanglePlacement::doNotResize);

        g.setColour(comp.findColour(BubbleComponent::outlineColourId));
        g.drawRoundedRectangle(body, 3.f, 2.f);
    }

};

class CurveSliderLNF : public LookAndFeel_V4
{
    void drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos,
        float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider& slider) override
    {
        g.setColour(Colour(0xffa7a7a7));
        Rectangle<float> area(slider.getLocalBounds().toFloat().reduced(9.f, 0.f));
        g.drawRoundedRectangle(area, 3.f, 1.f);
        g.setColour(Colours::darkslategrey.withAlpha(0.5f));
        g.fillRoundedRectangle(area.reduced(1.f), 3.f);

        auto centerX = slider.getLocalBounds().getCentreX();

        Path center;
        center.startNewSubPath(centerX, y + height);
        center.lineTo(slider.getLocalBounds().getCentreX(), y);
        g.setColour(Colour(0xffa7a7a7));
        g.strokePath(center, PathStrokeType(1.f));

        if (sliderPos > centerX) {
            ColourGradient posGradient(Colours::transparentWhite, centerX, y + height / 2,
                Colours::seagreen, width, y + height / 2, false);
            
            Rectangle<int> r(centerX, y + 1 , sliderPos - centerX, height - 2);
            g.setGradientFill(posGradient);
            g.fillRect(r);
        }
        else if (sliderPos < centerX) {
            ColourGradient negGradient(Colours::transparentWhite, centerX, y + height / 2,
                Colours::teal.withAlpha(0.7f), x, y + height / 2, false);

            Rectangle<int> r (sliderPos, y + 1, centerX, height - 2);
            g.setGradientFill(negGradient);
            g.fillRect(r);
        }

        //float thumbX = jmin(float(width - 3), jmax(sliderPos, 1.f));
        Rectangle<float> sliderThumb{ sliderPos - 1.5f, (float)y + 1.f, 3.f, (float)height - 2.f };
        g.setColour(Colours::floralwhite);
        g.fillRoundedRectangle(sliderThumb, 3.f);

        if (slider.isMouseOverOrDragging()) {
            g.setColour(Colours::navajowhite);
            g.drawRoundedRectangle(sliderThumb, 2.f, 2.f);
        }
    }
};
