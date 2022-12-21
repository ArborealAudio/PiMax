/**
 * Menu.h
 * Component for managing additional options
 */

#pragma once

class MenuComponent : public Component
{
    DrawableButton menuButton;
    std::unique_ptr<Drawable> icon;

    bool openGL = false;

public:
    MenuComponent() : menuButton("Menu", DrawableButton::ButtonStyle::ImageFitted)
    {
        addAndMakeVisible(menuButton);
        icon = Drawable::createFromImageData(BinaryData::Hamburger_icon_svg, BinaryData::Hamburger_icon_svgSize);
        menuButton.setImages(icon.get());
        menuButton.onClick = [&]
        {
            PopupMenu m;
            m.setLookAndFeel(new PopupLNF());
#if !JUCE_MAC
            openGL = (bool)readConfigFile("openGL");
            m.addItem(1, "OpenGL", true, openGL);
#endif
            m.addItem(2, "Default UI Size");
            bool displayingTooltip = (bool)readConfigFile("tooltip");
            m.addItem(3, "Show Tooltips", true, displayingTooltip);

            m.showMenuAsync(PopupMenu::Options().withMinimumWidth(175).withStandardItemHeight(35),
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
                                case 3:
                                    if (tooltipCallback)
                                        tooltipCallback();
                                    break;
                                }
                            });
        };
    }

    std::function<void()> windowResizeCallback;
    std::function<void(bool)> openGLCallback;
    std::function<void()> tooltipCallback;

    void resized() override
    {
        menuButton.setBounds(getLocalBounds());
    }
};