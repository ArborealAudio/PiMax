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

            PopupMenu asym;
            asym.setLookAndFeel(&lnf);
            asym.addItem(4, "Global Bias", true, p.atomics.globalBias);
            asym.addItem(5, "Alt. Type", true, p.atomics.altAsymType);
            m.addSubMenu("Asym. Options", asym);

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
                                    if (globalBiasCallback)
                                        globalBiasCallback(editor);
                                    break;
                                case 5:
                                    if (asymTypeCallback)
                                        asymTypeCallback(editor);
                                    break;
                                }
                            });

            asym.setLookAndFeel(nullptr);
            m.setLookAndFeel(nullptr);
        };
    }

    void (*windowResetCallback)(MaximizerAudioProcessorEditor &);
    void (*openGLCallback)(MaximizerAudioProcessorEditor &, bool);
    void (*tooltipCallback)(MaximizerAudioProcessorEditor &);
    void (*globalBiasCallback)(MaximizerAudioProcessorEditor &);
    void (*asymTypeCallback)(MaximizerAudioProcessorEditor &);

    void resized() override { menuButton.setBounds(getLocalBounds()); }
};
