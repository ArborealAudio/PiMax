/*
  ==============================================================================

    PresetComp.h
    Created: 19 Oct 2021 11:05:59am
    Author:  alexb

  ==============================================================================
*/

#pragma once
#include "../UI/LookAndFeel.h"

struct PresetComp : Component
{
    PresetComp(AudioProcessorValueTreeState& vts) : manager(vts)
    {
        addAndMakeVisible(box);
        box.setJustificationType(Justification::centredLeft);

        loadPresets();

        addChildComponent(editor);
        editor.setJustification(Justification::centredLeft);
    }

    void loadPresets()
    {
        PopupMenu::Item save{ "save" }, saveAs{ "save as" }, copy{ "copy state" }, paste{"paste from clipboard"};

        auto menu = box.getRootMenu();
        menu->clear();
        save.setID(1001);
        save.setTicked(false);
        save.action = [this] {savePreset(); };
        menu->addItem(save);
        saveAs.setID(1002);
        saveAs.setTicked(false);
        saveAs.action = [this] {savePresetAs(); };
        menu->addItem(saveAs);
        copy.setID(1003);
        copy.setTicked(false);
        copy.action = [this]
        {
            SystemClipboard::copyTextToClipboard(manager.getStateAsString());
            box.setText(currentPreset, NotificationType::dontSendNotification);
        };
        menu->addItem(copy);
        paste.setID(1004);
        paste.setTicked(false);
        paste.action = [this]
        {
            manager.setStateFromString(SystemClipboard::getTextFromClipboard());
            box.setText(currentPreset, NotificationType::dontSendNotification);
        };
        menu->addItem(paste);
        menu->addSeparator();

        auto presets = manager.loadFactoryPresetList();
        box.addItemList(presets, 1);
        factoryPresetSize = presets.size();

        auto user = manager.loadUserPresetList();
        userPresets.clear();
        for (int i = 0; i < user.size(); ++i)
        {
            userPresets.addItem(factoryPresetSize + i + 1, user[i]);
        }

        menu->addSeparator();
        menu->addSubMenu("User Presets", userPresets);

    }

    String getCurrentPreset() noexcept
    {
        return currentPreset;
    }

    void setCurrentPreset(String newPreset) noexcept
    {
        currentPreset = newPreset;
        box.setText(currentPreset, NotificationType::dontSendNotification);
        //valueChanged(manager.compareStates(currentPreset));
    }

    void savePreset() noexcept
    {
        manager.savePreset(currentPreset, manager.userDir);
        box.setText(currentPreset, NotificationType::dontSendNotification);
    }

    void savePresetAs() noexcept
    {
        editor.setVisible(true);
        editor.toFront(true);
        editor.setTextToShowWhenEmpty("preset name", Colours::grey);

        editor.onFocusLost = [&]
        {
#if JUCE_WINDOWS
            editor.clear(); editor.setVisible(false);
#elif JUCE_MAC
            editor.onReturnKey();
#endif
        };
        editor.onEscapeKey = [&] { editor.clear(); editor.setVisible(false); };

        editor.onReturnKey = [&]
        {
            auto name = editor.getText();
            editor.clear();
            editor.setVisible(false);

            if (manager.savePreset(name, manager.userDir)) {
                box.setText(name, NotificationType::sendNotificationSync);
                loadPresets();
                currentPreset = name;
            }
            else
                box.setText("invalid name!", NotificationType::dontSendNotification);
        };
        
    }

    void valueChanged(bool presetModified) noexcept
    {
        auto id = box.getSelectedId();
        auto idx = box.getSelectedItemIndex();
        auto preset = box.getItemText(idx);

        if (!presetModified) {
            if (id < 1000 && id > 0) {
                
                if (id <= factoryPresetSize) {
                    if (manager.loadPreset(preset, manager.presetDir))
                        box.setText(preset, NotificationType::sendNotificationSync);
                    else
                        box.setText("preset not found", NotificationType::dontSendNotification);
                }
                else {
                    if (manager.loadPreset(preset, manager.userDir))
                        box.setText(preset, NotificationType::sendNotificationSync);
                    else
                        box.setText("preset not found", NotificationType::dontSendNotification);
                }

                currentPreset = preset;
            }
            else
                return;
        }
        else
            box.setText(preset, NotificationType::dontSendNotification);
    }

    void paint(Graphics& g) override
    {}

    void resized() override
    {
        box.setBounds(getLocalBounds());
        editor.setBounds(getLocalBounds());
    }

    ComboBox box;

private:

    PopupMenu userPresets;

    int factoryPresetSize = 0;

    TextEditor editor;

    StringArray presetList;
    String currentPreset;

    PresetManager manager;
};
