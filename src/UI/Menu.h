/**
 * Menu.h
 * Component for managing additional options
 */

#pragma once

#include "LookAndFeel.h"
#include "../PluginEditor.h"

class MenuComponent : public Component
{
    DrawableButton menuButton;
    std::unique_ptr<Drawable> icon;

    bool openGLEnabled = false;

    PopupLNF lnf;

  public:
    MenuComponent(MaximizerAudioProcessorEditor &editor,
                  MaximizerAudioProcessor &p)
        : menuButton("Menu", DrawableButton::ButtonStyle::ImageFitted)
    {
        addAndMakeVisible(menuButton);
        icon = Drawable::createFromImageData(
            BinaryData::Hamburger_icon_svg, BinaryData::Hamburger_icon_svgSize);
        menuButton.setImages(icon.get());
        menuButton.onClick = [&] {
            PopupMenu m;
            m.setLookAndFeel(&lnf);
#if !JUCE_MAC
            openGLEnabled = (bool)strix::readConfigFile(CONFIG_PATH, "openGL");
            m.addItem(1, "OpenGL", true, openGLEnabled);
#endif
            m.addItem(2, "Default UI Size");
            bool displayingTooltip =
                (bool)strix::readConfigFile(CONFIG_PATH, "tooltip");
            m.addItem(3, "Show Tooltips", true, displayingTooltip);
            m.addItem(4, "Alt. Asym Type", true, p.atomics.altAsymType);

            m.showMenuAsync(PopupMenu::Options()
                                .withMinimumWidth(175)
                                .withStandardItemHeight(35),
                            [&](int result) {
                                switch (result) {
                                case 0:
                                    break;
                                case 1:
                                    if (openGLCallback)
                                        openGLCallback(editor, !openGLEnabled);
                                    break;
                                case 2:
                                    if (windowResetCallback)
                                        windowResetCallback(editor);
                                    break;
                                case 3:
                                    if (tooltipCallback)
                                        tooltipCallback(editor);
                                    break;
                                case 4:
                                    if (asymTypeCallback)
                                        asymTypeCallback(editor);
                                    break;
                                }
                            });

            m.setLookAndFeel(nullptr);
        };
    }

    void (*windowResetCallback)(MaximizerAudioProcessorEditor &);
    void (*openGLCallback)(MaximizerAudioProcessorEditor &, bool);
    void (*tooltipCallback)(MaximizerAudioProcessorEditor &);
    void (*asymTypeCallback)(MaximizerAudioProcessorEditor &);

    void resized() override { menuButton.setBounds(getLocalBounds()); }
};
