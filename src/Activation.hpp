// Activation.hpp

#pragma once
#include <JuceHeader.h>

struct ActivationComponent : Component, Timer
{
    enum CheckResult
    {
        None,
        Success,
        InvalidLicense,
        WrongProduct,
        ConnectionFailed
    };

    enum CheckStatus
    {
        NotSubmitted,
        Waiting,
        Reset,
        Finished
    };

    ActivationComponent(int64 _trialRemaining) : trialRemaining(_trialRemaining)
    {
        addAndMakeVisible(editor);
        editor.setFont(Font(18.f));
        editor.onReturnKey = [&] { checkInput(); };
        editor.setTextToShowWhenEmpty("License", Colours::lightgrey);

        addAndMakeVisible(submit);
        submit.onClick = [&] { checkInput(); };

        addAndMakeVisible(close);
        close.onClick = [&] { setVisible(false); };

        addAndMakeVisible(buy);
        buy.onClick = [] {
            URL("https://arborealaudio.com/plugins/pimax")
                .launchInDefaultBrowser();
        };

        thread = std::make_unique<strix::LiteThread>(-1);
        startTimerHz(2);
    }

    ~ActivationComponent() { stopTimer(); }

    void timerCallback() override
    {
        if (m_status == Finished)
            checkResults();
        repaint();
    }

    /* called when UI submits a license key & when site check is successful.
    Basically you set the visibility of this ActivationComponent and set the
    AudioProcessor's `isUnlocked` variable */
    std::function<void(bool)> onActivationCheck;

    void checkInput()
    {
        auto text = editor.getText();
        license_text = text;
        if (text.isEmpty()) {
            editor.clear();
            editor.setTextToShowWhenEmpty(
                "Actually enter a license...",
                Colour::fromHSL((float)color / 360.f, 1.f, 0.75f, 1.f));
            color += 45;
            color %= 360;
            if (onActivationCheck)
                onActivationCheck(false);
            return;
        }
        if (thread && (m_status == NotSubmitted || m_status == Reset)) {
            m_status = Waiting;
            DBG("Checking license...\n");
            thread->addJob([this, text] {
                check_result = checkSite(text);
                m_status = Finished;
            });
        }
    }

    void checkResults()
    {
        if (check_result == Success) {
            stopTimer();
            thread->working = false;
            writeFile(license_text.toRawUTF8());
            if (onActivationCheck)
                onActivationCheck(true);
            editor.setVisible(false);
            close.setVisible(true);
            close.setEnabled(true);
            submit.setVisible(false);
        } else {
            if (onActivationCheck)
                onActivationCheck(false);

            editor.setVisible(true);
            editor.clear();
            editor.setTextToShowWhenEmpty("Invalid license...", Colours::red);
            m_status = Reset;
        }
    }

    void paint(Graphics &g) override
    {
        g.setColour(Colours::black.withAlpha(0.9f));
        g.fillRoundedRectangle(getLocalBounds().reduced(5).toFloat(), 10.f);

        g.setFont(18.f);
        g.setColour(Colours::white);
        String message;
        String trialDesc = "";
        if (m_status == NotSubmitted) {
            message = "Enter your license:";
        } else if (m_status == Waiting) {
            message = "Checking...";
        } else if (m_status == Finished || m_status == Reset) {
            g.setColour(Colours::red);
            switch (check_result) {
            case Success:
                if (check_result == Success)
                    message = "License activated! Thank you!";
                g.setColour(Colours::white);
                break;
            case InvalidLicense:
                message = "Invalid license";
                break;
            case WrongProduct:
                message = "License for wrong product";
                break;
            case ConnectionFailed:
                message = "Connection failed. Try again.";
                break;
            case None:
                message = "Activation...not run? Error!";
                break;
            }
        }
        if (trialRemaining > 0 && check_result != Success)
            trialDesc =
                RelativeTime::milliseconds(trialRemaining).getDescription() +
                " remaining in free trial";
        else if (trialRemaining <= 0 && check_result != Success)
            trialDesc = "Trial expired.";
        g.drawFittedText(message + "\n" + trialDesc,
                         getLocalBounds().removeFromTop(getHeight() * 0.3f),
                         Justification::centred, 3);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        auto w = b.getWidth();
        auto h = b.getHeight();

        auto editorPadding = BorderSize<int>(h * 0.4, 20, h * 0.4, 20);
        editor.setBoundsInset(editorPadding);

        auto buttons = b.removeFromBottom(h * 0.3f);
        submit.setBounds(buttons.removeFromLeft(w / 3).reduced(10));
        close.setBounds(buttons.removeFromLeft(w / 3).reduced(10));
        buy.setBounds(buttons.removeFromLeft(w / 3).reduced(10));
    }

    CheckResult checkSite(const String &input)
    {
        auto url =
            URL("https://3pvj52nx17.execute-api.us-east-1.amazonaws.com/default/licenses/" + input);

        DBG("Querying URL: " << url.toString(false));

        CheckResult result = CheckResult::ConnectionFailed;

        if (auto stream = url.createInputStream(
                URL::InputStreamOptions(URL::ParameterHandling::inAddress)
                    .withExtraHeaders(
                        "x-api-key: "
                        AWS_API_KEY)
                    .withConnectionTimeoutMs(10000))) {

            auto web_stream = dynamic_cast<WebInputStream *>(stream.get());
            const auto status = web_stream->getStatusCode();
            DBG("Status: " << status);

            auto response = stream->readEntireStreamAsString();
            DBG("Response: " << response);
            auto json = JSON::parse(response);
            const auto success = (bool)json.getProperty("success", var(false));

            if (success != true)
                return CheckResult::InvalidLicense;

            return CheckResult::Success;
        }

        return result;
    }

    TextEditor editor;
    TextButton submit{"Submit"}, close{"Close"}, buy{"Buy"};

  private:
    std::atomic<CheckResult> check_result = None;
    std::atomic<CheckStatus> m_status = NotSubmitted;

    String license_text;

    std::unique_ptr<strix::LiteThread> thread = nullptr;

    int64 trialRemaining = 0;

    uint16 color;

    void writeFile(const char *key)
    {
        File license{
            File::getSpecialLocation(File::userApplicationDataDirectory)
                .getFullPathName() +
            "/Arboreal Audio/PiMax/License/license.aal"};
        if (!license.existsAsFile())
            license.create();

        XmlElement xml{"Key"};
        xml.setAttribute("key", key);
        xml.writeTo(license);
    }
};
