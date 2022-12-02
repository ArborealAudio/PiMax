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

namespace strix
{
    const Identifier rootId = "root";
    const Identifier propertyId = "property";

    inline void setProperty(ValueTree &source, const Identifier &id, const var &v)
    {
        jassert(id.isValid() && !v.isVoid());
        source.setProperty(id, v, nullptr);
    }

    inline void appendValueTree(ValueTree &parent, const Identifier &id, const var &v)
    {
        if (!parent.isValid() || id.isNull() || v.isVoid())
            return;

        if (auto *ar = v.getArray())
        {
            for (const auto &item : *ar)
            {
                ValueTree child(rootId);
                parent.appendChild(child, nullptr);
                appendValueTree(child, propertyId, item);
            }

            return;
        }

        if (auto *object = v.getDynamicObject())
        {
            ValueTree child(id);
            parent.appendChild(child, nullptr);

            for (const auto &prop : object->getProperties())
                appendValueTree(parent, prop.name, prop.value);

            return;
        }

        ValueTree child(id);
        parent.appendChild(child, nullptr);
        setProperty(child, Identifier(String("property") + String(parent.getNumProperties())), v);
    }

    inline ValueTree createValueTreeFromJSON(const var &json)
    {
        if (json.isVoid())
            return {};

        ValueTree root(rootId);
        appendValueTree(root, rootId, json);
        return root;
    }

    inline ValueTree createValueTreeFromJSON(const String &data)
    {
        return createValueTreeFromJSON(JSON::parse(data));
    }
}

struct UnlockStatus
{
    UnlockStatus() = default;

    juce::URL getURL()
    {
        return URL("https://3pvj52nx17.execute-api.us-east-1.amazonaws.com");
    }

    String readReplyForKey(const juce::String &key, bool activate)
    {
        URL url(getURL()
                    .withNewSubPath("/default/licenses/" + key));

        if (activate)
            url = url.withNewSubPath("/default/licenses/activate/" + key);

        DBG("Trying to unlock via URL: " << url.toString(true));
        if (auto stream = URL(url).createInputStream(URL::InputStreamOptions(URL::ParameterHandling::inAddress)
                                                         .withExtraHeaders("x-api-key: Fb5mXNfHiNaSKABQEl0PiFmYBthvv457bOCA1ou2").withConnectionTimeoutMs(10000)))
        {
            return stream->readEntireStreamAsString();
        }

        return {};
    }

    /*check for valid order no*/
    String readReplyForOrderNo(const juce::String &orderNo)
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

    /*0 = failed key | 1 = success | 2 = activations maxed*/
    inline int authorize(const String &key)
    {
        if (key.isEmpty())
            return 0;

        auto keyResponse = readReplyForKey(key, false);
        DBG("keyResponse: " << keyResponse);

        auto keyTree = strix::createValueTreeFromJSON(keyResponse);

        auto success = keyTree.getChildWithName("success").getProperty("property0");
        if (!success)
            return 0;

        // auto orderId = keyTree.getChildWithName("orderId").getProperty("property0");

        // auto orderResponse = readReplyForOrderNo(orderId);

        // auto orderTree = strix::createValueTreeFromJSON(orderResponse);

        // auto orderEmail = orderTree.getChildWithName("email").getProperty("property0").toString();
        // owner = orderTree.getChildWithName("first_name").getProperty("property0").toString();
        //
        // owner.append(" ", 1);
        // owner += orderTree.getChildWithName("last_name").getProperty("property0").toString();

        auto activationResponse = strix::createValueTreeFromJSON(readReplyForKey(key, true));
        auto activationCount = activationResponse.getChildWithName("timesActivated").getProperty("property0");
        auto activationLim = activationResponse.getChildWithName("timesActivatedMax").getProperty("property0");
        if (activationCount >= activationLim)
            return 2;

        state.setProperty("value", success, nullptr);

        m_key = key;

        return (int)success;
    }

    inline void writeHashKey()
    {
        if (isUnlocked())
        {
            auto dir = File(File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory)
                                .getFullPathName() +
                            "/Arboreal Audio/PiMax/License/license.aal");

            if (!dir.exists())
                dir.create();

            auto id = SystemStats::getUniqueDeviceID();

            XmlElement xml{"Key"};
            xml.setAttribute("uuid", String(id.hashCode64()));
            xml.setAttribute("key", m_key);

            xml.writeTo(dir);
            dir.setReadOnly(true);

#if JUCE_MAC
            auto dirGB = File("~/Music/Audio Music Apps/Arboreal Audio/PiMax/License/license.aal");
            auto gbuuid = File("~/Music/Audio Music Apps").getFileIdentifier();

            if (!dirGB.exists())
                dirGB.create();

            XmlElement xmlGB{"Key"};
            xmlGB.setAttribute("uuid", String(id.hashCode64()));
            xmlGB.setAttribute("GBuuid", String(gbuuid));

            xmlGB.writeTo(dirGB);
            dirGB.setReadOnly(true);
#endif
        }
    }

private:
    ValueTree state{"UNLOCKED", {{"value", 0}}};

    String m_key;
};

struct UnlockForm : Component
{
    UnlockForm(UnlockStatus &s, int64 trialTime) : status(s),
                                                   userInstructions("Register PiMax"), trialRemaining_ms(trialTime)
    {
        addAndMakeVisible(key);
        key.setTextToShowWhenEmpty("serial number", Colours::grey);
        key.applyFontToAllText(getCustomFont(FontStyle::Regular).withHeight(15.f));

        lnf.setType(TopButtonLNF::Type::Regular);
        addAndMakeVisible(reg);
        reg.setLookAndFeel(&lnf);
        addAndMakeVisible(close);
        close.setLookAndFeel(&lnf);

        reg.onClick = [this]
        { runAuth(); };
        close.onClick = [this]
        { dismiss(); };
    }

    ~UnlockForm()
    {
        reg.setLookAndFeel(nullptr);
        close.setLookAndFeel(nullptr);
    }

    void paint(Graphics &g) override
    {
        g.fillAll(Colours::darkslategrey.withAlpha(0.25f));
        g.setColour(Colour(0xa7a7a7a7));
        g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.5f), 5.f, 1.5f);
        g.setFont(getCustomFont(FontStyle::Regular).withHeight(20.f));

        if (!successRepaint)
        {
            g.drawFittedText(userInstructions, getLocalBounds().withTrimmedBottom(300), Justification::centred, 4);
            if (key.isTextInputActive())
                key.applyColourToAllText(Colours::white);

            if ((int)trialRemaining_ms > 0)
            {
                auto timeRemaining = (Time::getCurrentTime() + RelativeTime::milliseconds(trialRemaining_ms) - Time::getCurrentTime());
                auto daysRemaining = timeRemaining.getDescription();
                Rectangle<int> textBounds{45, 60, 150, 25};
                g.setFont(getCustomFont(FontStyle::Regular).withHeight(15.f));
                g.drawFittedText(daysRemaining + " left in free trial", textBounds, Justification::centred, 2);
            }
            else
            {
                Rectangle<int> textBounds{45, 45, 150, 85};
                g.setFont(getCustomFont(FontStyle::Regular).withHeight(15.f));
                g.setColour(Colours::darkred);
                g.fillRoundedRectangle(textBounds.toFloat(), 5.f);
                g.setColour(Colour(0xffa7a7a7));
                g.drawFittedText("Please purchase a license @\n"
                                 "arborealaudio.com\n"
                                 "to continue using PiMax",
                                 textBounds, Justification::centred, 3);
                close.setEnabled(false);
                if (textBounds.contains(getMouseXYRelative()))
                {
                    setMouseCursor(MouseCursor::PointingHandCursor);
                    if (isMouseButtonDown() && !clickedLink)
                    {
                        URL("https://arborealaudio.com/plugins/pimax").launchInDefaultBrowser();
                        clickedLink = true;
                        return;
                    }
                    else if (!isMouseButtonDown() && clickedLink)
                        clickedLink = false;
                }
                else
                    setMouseCursor(MouseCursor::NormalCursor);
            }
        }
        else
        {
            key.setVisible(false);
            reg.setVisible(false);
            close.setEnabled(true);
            g.drawText("Thank you for your purchase.", getLocalBounds(), Justification::centred);
        }
    }

    void resized() override
    {
        key.centreWithSize(150, 25);
        reg.centreWithSize(68, 25);
        close.centreWithSize(50, 25);
        // key.setBounds(key.getBounds().translated(0, -60));
        reg.setBounds(reg.getBounds().translated(0, 60));
        close.setBounds(close.getBounds().translated(0, 110));
    }

    void runAuth()
    {
        auto result = status.authorize(key.getText());

        if (result == 0)
        {
            key.unfocusAllComponents();
            key.setText("", false);
            key.setTextToShowWhenEmpty("invalid serial number", Colours::red);
            repaint();
        }
        else if (result == 1)
        {
            successRepaint = true;
            repaint();
        }
        else
        {
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

    TextEditor key;

    TopButtonLNF lnf;
    TextButton reg{"Register"}, close{"Close"};

    bool successRepaint = false, clickedLink = false;

    UnlockStatus &status;
};

struct ActivationComponent : Component, Timer
{
    ActivationComponent(var unlocked, int64 trialTime) : unlockForm(status, trialTime), isUnlocked(unlocked)
    {
        addChildComponent(unlockForm);
        if (!isUnlocked)
            unlockForm.setVisible(true);

        startTimer(100);
    }

    ~ActivationComponent() { stopTimer(); }

    bool isFormVisible() { return unlockForm.isVisible(); }

    void setImage(const Image &newImage)
    {
        img = newImage;

        auto bounds = getParentComponent()->getLocalArea(this, unlockForm.getBounds());
        img = img.getClippedImage(bounds);

        needBlur = true;

        showForm();
    }

    void paint(Graphics &g) override
    {
        if (unlockForm.isVisible())
        {
            if (needBlur)
            {
                Blur::blurImage<4, true>(img);
                needBlur = false;
            }
            g.drawImage(img, unlockForm.getBounds().toFloat(), RectanglePlacement::centred);
        }
    }

    void resized() override
    {
        unlockForm.setBounds(getLocalBounds());
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
    }

    bool hitTest(int x, int y) override
    {
        if (unlockForm.isVisible())
            return true;
        else
            return false;
    }

    std::function<void(var)> onUnlock;

private:
    void showForm()
    {
        if (!unlockForm.isVisible())
            unlockForm.setVisible(true);
        unlockForm.toFront(true);
        unlockForm.unfocusAllComponents();
    }

    inline void unlockApp()
    {
        status.writeHashKey();
        repaint();
    }

    Image img;

    UnlockStatus status;
    UnlockForm unlockForm;

    var isUnlocked = false;

    bool needBlur = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ActivationComponent)
};
