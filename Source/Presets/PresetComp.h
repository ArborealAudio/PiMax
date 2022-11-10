/*
  ==============================================================================

    PresetComp.h
    Created: 19 Oct 2021 11:05:59am
    Author:  alexb

  ==============================================================================
*/

#pragma once

struct PresetComp : Component
{
    PresetComp(AudioProcessorValueTreeState& vts) : manager(vts)
    {
        addAndMakeVisible(box);
        box.setJustificationType(Justification::centredLeft);
        box.setTextWhenNothingSelected("Presets");

        loadPresets();

        addChildComponent(editor);
        editor.setJustification(Justification::centredLeft);
    }

    void loadPresets()
    {
        PopupMenu::Item save{ "save" }, saveAs{ "save as" }, copy{ "copy state" }, paste{"paste state"}, presetDir{"open preset folder"};

        auto menu = box.getRootMenu();
        menu->clear();
        save.setID(1001);
        save.setTicked(false);
        save.action = [&] { savePreset(); };
        menu->addItem(save);

        saveAs.setID(1002);
        saveAs.setTicked(false);
        saveAs.action = [&] { savePresetAs(); };
        menu->addItem(saveAs);

        copy.setID(1003);
        copy.setTicked(false);
        copy.action = [&]
        {
            SystemClipboard::copyTextToClipboard(manager.getStateAsString());
        };
        menu->addItem(copy);

        paste.setID(1004);
        paste.setTicked(false);
        paste.action = [&]
        {
            manager.setStateFromString(SystemClipboard::getTextFromClipboard());
        };
        menu->addItem(paste);

        presetDir.setID(1005);
        presetDir.setTicked(false);
        presetDir.action = [&]
        {
            manager.userDir.getParentDirectory().startAsProcess();
        };
        menu->addItem(presetDir);
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
    }

    void savePreset() noexcept
    {
        if (box.getText() == "" || currentPreset == "")
            return;
        manager.savePreset(currentPreset, manager.userDir);
        setCurrentPreset(currentPreset);
    }

    void savePresetAs() noexcept
    {
        editor.setVisible(true);
        editor.toFront(true);
        editor.setText("Preset Name", false);
        editor.grabKeyboardFocus();
        editor.setHighlightedRegion({ 0, 12 });

        editor.onFocusLost = [&]
        {
            editor.clear();
            editor.setVisible(false);
        };

        editor.onEscapeKey = [&] { editor.clear(); editor.setVisible(false); };

        editor.onReturnKey = [&]
        {
            auto name = editor.getText();
            if (name == "")
            {
                box.setText("Enter a name!");
                return;
            }
            editor.clear();
            editor.setVisible(false);

            if (manager.savePreset(name, manager.userDir)) {
                loadPresets();
                setCurrentPreset(name);
            }
            else
                box.setText("invalid name!", NotificationType::dontSendNotification);
        };
    }

    void valueChanged() noexcept
    {
        auto id = box.getSelectedId();
        auto idx = box.getSelectedItemIndex();
        auto preset = box.getItemText(idx);

        if (id < 1000 && id > 0) {

            if (id <= factoryPresetSize) {
                if (manager.loadPreset(preset, true))
                    box.setText(preset, NotificationType::sendNotificationSync);
                else
                    box.setText("preset not found", NotificationType::dontSendNotification);
            }
            else {
                if (manager.loadPreset(preset, false))
                    box.setText(preset, NotificationType::sendNotificationSync);
                else
                    box.setText("preset not found", NotificationType::dontSendNotification);
            }

            currentPreset = preset;
        }
        else
            box.setText(currentPreset, NotificationType::dontSendNotification);
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
