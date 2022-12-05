/*
  ==============================================================================

    DownloadManager.h
    Created: 3 Nov 2021 4:43:12pm
    Author:  alexb

  ==============================================================================
*/

#pragma once

const String versionURL
#if PRODUCTION_BUILD
    {"https://arborealaudio.com/versions/PiMax-latest.json"};
#else
    {"https://arborealaudio.com/versions/test/PiMax-latest.json"};
#endif

const String downloadURLWin{"https://arborealaudioinstallers.s3.amazonaws.com/pimax/PiMax-windows.exe"};

const String downloadURLMac{"https://arborealaudioinstallers.s3.amazonaws.com/pimax/PiMax-mac.dmg"};

struct DownloadManager : Component
{
    DownloadManager(bool hasUpdated) : updated(hasUpdated)
    {
        lnf.setType(TopButtonLNF::Type::Regular);
        addAndMakeVisible(yes);
        yes.setLookAndFeel(&lnf);
        addAndMakeVisible(no);
        no.setLookAndFeel(&lnf);
        addChildComponent(retry);
        retry.setLookAndFeel(&lnf);

        future = std::async(std::launch::async, [&]{checkForUpdate();});

        onUpdateCheck = [&](bool needsUpdate)
        {
            if (needsUpdate && !updated)
            {
                DBG("need update");
                setVisible(true);
            }
            else
            {
                setVisible(false);
            }
        };
        // if (checkForUpdate() && !updated)
        // {
        //     DBG("need update");
        //     setVisible(true);
        // }
        // else
        // {
        //     setVisible(false);
        // }

        no.onClick = [this]
        { setVisible(false); updated = true; onUpdateChange(updated); };
        yes.onClick = [this]
        { downloadFinished.store(false); downloadUpdate(); };
    }

    ~DownloadManager()
    {
        yes.setLookAndFeel(nullptr);
        no.setLookAndFeel(nullptr);
        retry.setLookAndFeel(nullptr);
    }

    void checkForUpdate()
    {
        if (auto stream = URL(versionURL).createInputStream(URL::InputStreamOptions(URL::ParameterHandling::inAddress).withConnectionTimeoutMs(10000)))
        {
            auto data = JSON::parse(stream->readEntireStreamAsString());

            auto changesObj = data.getProperty("changes", var());
            if (changesObj.isArray())
            {
                std::vector<String> chVec;
                for (size_t i = 0; i < changesObj.size(); ++i)
                {
                    chVec.emplace_back(changesObj[i].toString());
                }

                StringArray changesList{chVec.data(), (int)chVec.size()};
                changes = changesList.joinIntoString(", ");
            }
            else
                changes = changesObj;

            auto latestVersion = data.getProperty("version", var());

            DBG("Current: " << String(ProjectInfo::versionString));
            DBG("Latest: " << latestVersion.toString());

#if PRODUCTION_BUILD
            // return String(ProjectInfo::versionString).removeCharacters(".") < latestVersion.toString().removeCharacters(".");
            if (onUpdateCheck)
                onUpdateCheck(String(ProjectInfo::versionString).removeCharacters(".") < latestVersion.toString().removeCharacters("."));
#else
            DBG("Update result: " << int(String(ProjectInfo::versionString).removeCharacters(".") < latestVersion.toString().removeCharacters(".")));
            if (onUpdateCheck)
                onUpdateCheck(true);
#endif
        }
        else
            if (onUpdateCheck)
                onUpdateCheck(false);
    }

    void downloadUpdate()
    {
        download.setDownloadBlockSize(128000);
        download.setProgressInterval(60);
        download.setRetryDelay(2);
        download.setRetryLimit(2);
#if JUCE_WINDOWS
        download.startAsyncDownload(URL(downloadURLWin), result, progress);
#elif JUCE_MAC
        download.startAsyncDownload(URL(downloadURLMac), result, progress);
#endif
        isDownloading.store(true);
        isInstalling.store(false);
    }

    void paint(Graphics &g) override
    {
        if (isVisible())
        {
            g.setColour(Colours::darkslategrey);
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 15.f);

#if !JUCE_MAC
            g.setFont(getCustomFont(FontStyle::Regular).withHeight(14.f));
#else
            g.setFont(getCustomFont(FontStyle::Regular).withHeight(13.f));
#endif
            g.setColour(Colour(0xa7a7a7a7));

            auto textbounds = Rectangle<int>{getLocalBounds().withTrimmedBottom(70)};

            if (!downloadFinished.load())
            {
                if (!isDownloading.load())
                    g.drawFittedText("A new update is available! Would you like to download?\n\nChanges:\n" + changes, textbounds, Justification::centredTop, 10, 0.f);
                else
                    g.drawFittedText("Downloading... " + String(downloadProgress.load(), 2) + "%",
                                     textbounds, Justification::centred,
                                     1, 1.f);
            }
            else
            {
                if (downloadStatus.load())
                {
                    g.drawFittedText("Download complete.\nThe installer is in your Downloads folder. You must close your DAW to run the installation.",
                                     textbounds, Justification::centred,
                                     7, 1.f);
                    yes.setVisible(false);
                    no.setButtonText("Close");
                }
                else
                {
                    g.drawFittedText("Download failed. Please try again.",
                                     textbounds, Justification::centred,
                                     7, 1.f);
                    retry.setVisible(true);
                    retry.onClick = [this]
                    { yes.onClick(); };
                }
            }
        }
        else
            return;
    }

    void resized() override
    {
        auto halfWidth = getWidth() / 2;
        auto halfHeight = getHeight() / 2;

        Rectangle<int> yesBounds{getLocalBounds().withTrimmedTop(halfHeight).withTrimmedRight(halfWidth).reduced(20, 30)};
        Rectangle<int> noBounds{getLocalBounds().withTrimmedTop(halfHeight).withTrimmedLeft(halfWidth).reduced(20, 30)};
        Rectangle<int> retryBounds{getLocalBounds().withTrimmedTop(halfHeight).reduced(20, 30)};

        yes.setBounds(yesBounds);
        no.setBounds(noBounds);
        retry.setBounds(retryBounds);
    }

    std::function<void(bool)> onUpdateChange;

private:
    std::function<void(bool)> onUpdateCheck;

    std::future<void> future;

    gin::DownloadManager download;

    TextButton yes{"Yes"}, no{"No"}, retry{"Retry"};

    TopButtonLNF lnf;

    bool updated = false;

    std::atomic<bool> downloadStatus = false;
    std::atomic<bool> isDownloading = false;
    std::atomic<bool> isInstalling = false;
    std::atomic<float> downloadProgress;
    std::atomic<bool> downloadFinished = false;

    String changes;

    std::function<void(gin::DownloadManager::DownloadResult)> result =
        [this](gin::DownloadManager::DownloadResult dlResult)
    {
        repaint();

        downloadStatus.store(dlResult.ok);

        if (!dlResult.ok)
            return;

#if JUCE_WINDOWS
        auto exe = File("C:/Users/" + SystemStats::getLogonName() + "/Downloads/PiMax-windows.exe");

        if (!dlResult.saveToFile(exe))
            downloadStatus.store(false);
        else
            downloadFinished.store(true);
#elif JUCE_MAC || JUCE_LINUX
        auto dmg = File("~/Downloads/PiMax-mac.dmg");

        if (!dlResult.saveToFile(dmg))
            downloadStatus.store(false);
        else
            downloadFinished.store(true);
#endif

        updated = true;
        if (onUpdateChange != nullptr)
            onUpdateChange(updated);
        repaint();
    };

    std::function<void(int64, int64, int64)> progress = [this](int64 downloaded, int64 total, int64)
    {
        downloadProgress = 100.f * ((float)downloaded / (float)total);
        repaint();
    };
};
