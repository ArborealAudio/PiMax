/**
 * Menu.h
 * Component for managing additional options
 */

#pragma once

#include "LookAndFeel.h"
#include "UI.h"
#include <memory>
class MenuComponent : public Component
{
    DrawableButton menuButton;
    std::unique_ptr<Drawable> icon;

    bool openGLEnabled = false;

    PopupLNF lnf;

public:
    MenuComponent() : menuButton("Menu", DrawableButton::ButtonStyle::ImageFitted)
    {
        addAndMakeVisible(menuButton);
        icon = Drawable::createFromImageData(BinaryData::Hamburger_icon_svg, BinaryData::Hamburger_icon_svgSize);
        menuButton.setImages(icon.get());
        menuButton.onClick = [&]
        {
            PopupMenu m;
            m.setLookAndFeel(&lnf);
#if !JUCE_MAC
            openGLEnabled = (bool)strix::readConfigFile(CONFIG_PATH, "openGL");
            m.addItem(1, "OpenGL", true, openGL);
#endif
            m.addItem(2, "Default UI Size");
            bool displayingTooltip = (bool)strix::readConfigFile(CONFIG_PATH, "tooltip");
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
                                        openGLCallback(!openGLEnabled);
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

            m.setLookAndFeel(nullptr);
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
