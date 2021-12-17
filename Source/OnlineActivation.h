/*
  ==============================================================================

    OnlineActivation.h
    Created: 30 Oct 2021 3:39:18pm
    Author:  alexb

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "UI/LookAndFeel.h"

struct UnlockStatus
{
    UnlockStatus() = default;

    juce::URL getURL()
    {
        return URL("https://arborealaudio.com/");
    }

    String readReplyForKey(const juce::String& key, bool activate)
    {
        URL url(getURL()
            .withNewSubPath("wp-json/lmfwc/v2/licenses/" + key)
            .withParameter("consumer_key", "ck_2ce7d78fab4ee7db66e6b5ee17f045111bdc3d00")
            .withParameter("consumer_secret", "cs_b8548efc0a05b35817bce7f994e20008cbafb751"));

        if (activate)
            url = url.withNewSubPath("wp-json/lmfwc/v2/licenses/activate/" + key);

        DBG("Trying to unlock via URL: " << url.toString(true));

        if (auto stream = URL(url).createInputStream(URL::InputStreamOptions(URL::ParameterHandling::inAddress)))
        {
            return stream->readEntireStreamAsString();
        }

        return {};
    }

    /*check for valid order no*/
    String readReplyForOrderNo(const juce::String& orderNo)
    {
        juce::URL url(getURL()
            .withNewSubPath("wp-json/wc/v3/orders/" + orderNo)
            .withParameter("consumer_key", "ck_afdaee27c41b45d26e864d0b2dad05d7d6bc9ee6")
            .withParameter("consumer_secret", "cs_0160660cf7a03dfab1e016a6922bf1b7a942e16f"));

        DBG("Trying to unlock via URL: " << url.toString(true));

        if (auto stream = URL(url).createInputStream(URL::InputStreamOptions(URL::ParameterHandling::inAddress)))
        {
            return stream->readEntireStreamAsString();
        }

        return {};
    }

    inline var isUnlocked()
    {
        return state.getProperty("value");
    }

    /*0 = failed key | 1 = failed orderNum | 2 = success | 3 = activations maxed*/
    inline int authorize(const String& key, const String& email)
    {
        if (key.isEmpty())
            return 0;
        if (email.isEmpty())
            return 1;

        auto keyResponse = readReplyForKey(key, false);
        DBG(keyResponse);

        auto keyTree = createValueTreeFromJSON(keyResponse);

        auto success = keyTree.getChildWithName("success").getProperty("property0");
        if (!success)
            return 0;

        auto orderId = keyTree.getChildWithName("orderId").getProperty("property0");

        auto orderResponse = readReplyForOrderNo(orderId);

        auto orderTree = createValueTreeFromJSON(orderResponse);

        auto orderEmail = orderTree.getChildWithName("email").getProperty("property0").toString();
        owner = orderTree.getChildWithName("first_name").getProperty("property0").toString();
        
        owner.append(" ", 1);
        owner += orderTree.getChildWithName("last_name").getProperty("property0").toString();
        
        if (orderEmail == email) {
            auto activationResponse = createValueTreeFromJSON(readReplyForKey(key, true));
            auto activationCount = activationResponse.getChildWithName("timesActivated").getProperty("property0");
            auto activationLim = activationResponse.getChildWithName("timesActivatedMax").getProperty("property0");
            if (activationCount >= activationLim)
                return 3;
        }

        state.setProperty("value", orderEmail == email, nullptr);

        return 1 + (orderEmail == email);
    }

    inline void writeHashKey()
    {
        if (isUnlocked())
        {
            auto dir = File(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory)
                .getFullPathName() + "/Arboreal Audio/PiMax/License/license.aal");

            if (!dir.exists())
                dir.create();

            auto ids = OnlineUnlockStatus::MachineIDUtilities::getLocalMachineIDs();

            XmlElement xml{ "Key" };

            for (int i = 0; i < ids.size(); ++i)
                xml.setAttribute("uuid", String(ids[i].hashCode64()));

            xml.setAttribute("owner", owner);

            xml.writeTo(dir);
            dir.setReadOnly(true);
            
        #if JUCE_MAC
            auto dirGB = File("~/Music/Audio Music Apps/Arboreal Audio/PiMax/License/license.aal");
            auto gbuuid = File("~/Music/Audio Music Apps").getFileIdentifier();

            if (!dirGB.exists())
                dirGB.create();
            
            XmlElement xmlGB{"Key"};
            
            for (int i = 0; i < ids.size(); ++i)
                xmlGB.setAttribute("uuid", String(ids[i].hashCode64()));
            
            xmlGB.setAttribute("GBuuid", String(gbuuid));

            xmlGB.setAttribute("owner", owner);

            xmlGB.writeTo(dirGB);
        #endif
        }
    }

    String getOwner()
    {
        return owner;
    }

private:

    const Identifier rootId = "root";
    const Identifier propertyId = "property";

    inline void setProperty(ValueTree& source, const Identifier& id, const var& v)
    {
        jassert(id.isValid() && !v.isVoid());
        source.setProperty(id, v, nullptr);
    }

    inline void appendValueTree(ValueTree& parent, const Identifier& id, const var& v)
    {
        if (!parent.isValid() || id.isNull() || v.isVoid())
            return;

        if (auto* ar = v.getArray())
        {
            for (const auto& item : *ar)
            {
                ValueTree child(rootId);
                parent.appendChild(child, nullptr);
                appendValueTree(child, propertyId, item);
            }

            return;
        }

        if (auto* object = v.getDynamicObject())
        {
            ValueTree child(id);
            parent.appendChild(child, nullptr);

            for (const auto& prop : object->getProperties())
                appendValueTree(parent, prop.name, prop.value);

            return;
        }

        ValueTree child(id);
        parent.appendChild(child, nullptr);
        setProperty(child, Identifier(String("property") + String(parent.getNumProperties())), v);
    }

    inline ValueTree createValueTreeFromJSON(const var& json)
    {
        if (json.isVoid())
            return {};

        ValueTree root(rootId);
        appendValueTree(root, rootId, json);
        return root;
    }

    inline ValueTree createValueTreeFromJSON(const String& data)
    {
        return createValueTreeFromJSON(JSON::parse(data));
    }

    ValueTree state{ "UNLOCKED", {{"value", 0}} };

    String owner;
};

struct UnlockForm : Component
{
    UnlockForm(UnlockStatus& s, int64 trialTime) : status(s),
        userInstructions("Register PiMax"), trialRemaining_ms(trialTime)
    {
        addAndMakeVisible(key);
        key.setTextToShowWhenEmpty("serial number", Colours::grey);
        key.applyFontToAllText(getCustomFont(FontStyle::Regular).withHeight(15.f));
        addAndMakeVisible(email);
        email.setTextToShowWhenEmpty("email address", Colours::grey);
        email.applyFontToAllText(getCustomFont(FontStyle::Regular).withHeight(15.f));

        lnf.setType(TopButtonLNF::Type::Regular);
        addAndMakeVisible(reg);
        reg.setLookAndFeel(&lnf);
        addAndMakeVisible(close);
        close.setLookAndFeel(&lnf);

        reg.onClick = [this] {runAuth(); };
        close.onClick = [this] {dismiss(); };
    }

    ~UnlockForm()
    {
        reg.setLookAndFeel(nullptr);
        close.setLookAndFeel(nullptr);
    }

    void paint(Graphics& g) override
    {
        g.fillAll(Colours::darkslategrey.withAlpha(0.25f));
        g.setColour(Colour(0xa7a7a7a7));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.5f), 5.f, 1.5f);
        g.setFont(getCustomFont(FontStyle::Regular).withHeight(20.f));

        if (!successRepaint) {
            g.drawFittedText(userInstructions, getLocalBounds().withTrimmedBottom(300), Justification::centred, 4);
            if (key.isTextInputActive())
                key.applyColourToAllText(Colours::white);
            if (email.isTextInputActive())
                email.applyColourToAllText(Colours::white);

            if (trialRemaining_ms > 0) {
                auto timeRemaining = (Time::getCurrentTime() + RelativeTime::milliseconds(trialRemaining_ms)
                    - Time::getCurrentTime());
                auto daysRemaining = timeRemaining.getDescription();
                Rectangle<int> textBounds{ 45, 60, 150, 25 };
                g.setFont(getCustomFont(FontStyle::Regular).withHeight(15.f));
                g.drawFittedText(daysRemaining + " left in free trial", textBounds, Justification::centred, 2);
            }
            else {
                Rectangle<int> textBounds{ 45, 45, 150, 50 };
                g.setFont(getCustomFont(FontStyle::Regular).withHeight(15.f));
                g.setColour(Colours::darkred);
                g.fillRoundedRectangle(textBounds.toFloat(), 5.f);
                g.setColour(Colour(0xffa7a7a7));
                g.drawFittedText("Please purchase a license to continue using PiMax:\n"
                    "arborealaudio.com",
                    textBounds, Justification::centred, 3);
                close.setEnabled(false);
                if (textBounds.contains(getMouseXYRelative())) {
                    setMouseCursor(MouseCursor::PointingHandCursor);
                    if (isMouseButtonDown()) {
                        URL("https://arborealaudio.com").launchInDefaultBrowser();
                        return;
                    }
                }
                else
                    setMouseCursor(MouseCursor::NormalCursor);
            }
        }
        else {
            key.setVisible(false);
            email.setVisible(false);
            reg.setVisible(false);
            close.setEnabled(true);
            g.drawText("Thank you for your purchase.", getLocalBounds(), Justification::centred);
        }
    }

    void resized() override
    {
        key.centreWithSize(150, 25);
        email.centreWithSize(150, 25);
        reg.centreWithSize(68, 25);
        close.centreWithSize(50, 25);
        key.setBounds(key.getBounds().translated(0, -60));
        reg.setBounds(reg.getBounds().translated(0, 60));
        close.setBounds(close.getBounds().translated(0, 110));
    }

    void runAuth()
    {
        auto result = status.authorize(key.getText(), email.getText());

        if (result == 0) {
            key.unfocusAllComponents();
            key.setText("", false);
            key.setTextToShowWhenEmpty("invalid serial number", Colours::red);
            repaint();
        }
        else if (result == 1) {
            email.unfocusAllComponents();
            email.setText("", false);
            email.setTextToShowWhenEmpty("email not found", Colours::red);
            repaint();
        }
        else if (result == 2) {
            successRepaint = true;
            repaint();
        }
        else {
            key.unfocusAllComponents();
            key.setText("", false);
            key.setTextToShowWhenEmpty("Activations maxed!", Colours::red);
            repaint();
        }
    }

    void dismiss()
    {
        setVisible(false);
    }

private:
    String userInstructions;

    int64 trialRemaining_ms;

    TextEditor key, email;

    TopButtonLNF lnf;
    TextButton reg{ "Register" },
        close{"Close"};

    bool successRepaint = false;

    UnlockStatus& status;
};

struct ActivationComponent : Component, Timer
{
    ActivationComponent(var unlocked, int64 trialTime) : unlockForm(status, trialTime), isUnlocked(unlocked)
    {
        addChildComponent(unlockButton);
        lnf.setType(TopButtonLNF::Type::Regular);
        unlockButton.setLookAndFeel(&lnf);
        addChildComponent(unlockForm);
        if (!isUnlocked) {
            unlockButton.setVisible(true);
            unlockForm.setVisible(true);
        }
        else
        {
            File dir;
            PluginHostType host;
            if (!host.isGarageBand())
                dir = File(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory)
                    .getFullPathName() + "/Arboreal Audio/PiMax/License/license.aal");
            else
                dir = File("~/Music/Audio Music Apps/Arboreal Audio/PiMax/License/license.aal");

            auto xml = parseXML(dir);
            owner = xml->getStringAttribute("owner");
        }
        unlockButton.onClick = [this] { if (onButtonClick != nullptr) onButtonClick(); };
        unlockButton.setBounds(90, 30, 62, 28);

        startTimer(100);
    }

    ~ActivationComponent() { unlockButton.setLookAndFeel(nullptr); stopTimer(); }

    void setImage(const Image& newImage)
    {
        img = newImage;
        needBlur = true;

        auto bounds = getParentComponent()->getLocalArea(this, unlockForm.getBounds());
        img = img.getClippedImage(bounds);

        if (needBlur) {
            gin::applyStackBlur(img, 35);
            needBlur = false;
        }

        showForm();
    }

    String getOwner(bool alreadyActivated)
    {
        if (!alreadyActivated)
            return status.getOwner();
        else
            return owner;
    }

    void paint(Graphics& g) override
    {
        if (unlockForm.isVisible())
        {
            g.drawImage(img, unlockForm.getBounds().toFloat(), RectanglePlacement::centred);
        }
    }

    void resized() override
    {
        unlockForm.setBounds(240, 50, 240, 360);
    }

    void timerCallback() override
    {
        if (!isUnlocked && status.isUnlocked())
        {
            isUnlocked = true;
            unlockApp();
            if (onUnlock != nullptr)
                onUnlock(true);
        }
        repaint();
    }
    
    bool hitTest(int x, int y) override
    {
        if (unlockButton.isVisible() && !unlockForm.isVisible() &&
            unlockButton.getBounds().contains((float)x, (float)y))
            return true;
        else if (unlockForm.isVisible())
            return true;
        else
            return false;
    }

    std::function<void()> onButtonClick;
    std::function<void(var)> onUnlock;

private:
    void showForm()
    {
        unlockForm.setVisible(true);
        unlockForm.toFront(true);
        unlockForm.unfocusAllComponents();
        needBlur = true;
        repaint();
    }

    inline void unlockApp()
    {
        unlockButton.setEnabled(false);
        unlockButton.setVisible(false);
        status.writeHashKey();
    }

    Image img;
    
    TopButtonLNF lnf;
    TextButton unlockButton{ "Unlock" };
    UnlockStatus status;
    UnlockForm unlockForm;

    var isUnlocked = false;

    bool needBlur = false;

    String owner;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ActivationComponent)
};

