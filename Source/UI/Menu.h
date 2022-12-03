/**
 * Menu.h
 * Component for managing additional options
 */

#pragma once

class MenuComponent : public Component
{
    AudioProcessorValueTreeState &vts;

    LookAndFeel_V4 lnf;

    DrawableButton menuButton;
    std::unique_ptr<Drawable> icon;

    bool openGL = false;

public:
    MenuComponent(AudioProcessorValueTreeState &a) : vts(a),
    menuButton("Menu", DrawableButton::ButtonStyle::ImageFitted)
    {
        openGL = (bool)readConfigFile("openGL");
        addAndMakeVisible(menuButton);
        icon = Drawable::createFromImageData(BinaryData::Hamburger_icon_svg, BinaryData::Hamburger_icon_svgSize);
        menuButton.setImages(icon.get());
        lnf.setColour(PopupMenu::backgroundColourId, Colours::darkslategrey.withAlpha(0.9f));
        lnf.setColour(PopupMenu::highlightedBackgroundColourId, Colours::grey);
        menuButton.onClick = [&]
        {
            PopupMenu m;
            m.setLookAndFeel(&lnf);
            openGL = (bool)readConfigFile("openGL");
            m.addItem(1, "OpenGL", true, openGL);
            m.addItem(2, "Default UI Size");

            m.showMenuAsync(PopupMenu::Options().withMinimumWidth(175)
                                                .withStandardItemHeight(35),
            [&](int result)
            {
            switch (result)
            {
            case 0:
                break;
            case 1:
                if (openGLCallback)
                    openGLCallback(!openGL);
                break;
            case 2:
                if (windowResizeCallback)
                    windowResizeCallback();
                break;
            }
            });
        };
    }

    std::function<void()> windowResizeCallback;
    std::function<void(bool)> openGLCallback;

    void resized() override
    {
        menuButton.setBounds(getLocalBounds());
    }
};