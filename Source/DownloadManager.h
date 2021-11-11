/*
  ==============================================================================

    DownloadManager.h
    Created: 3 Nov 2021 4:43:12pm
    Author:  alexb

  ==============================================================================
*/

#pragma once

const String versionURL
{ "https://arborealaudio.com/wp-json/twentytwentyone-child/v1/pimax-version" };

const String downloadURLWin
{ "https://arborealaudio.com/wp-content/downloads/PiMax-windows.exe" };

const String downloadURLMac
{ "https://arborealaudio.com/wp-content/downloads/PiMax-mac.dmg" };

struct DownloadManager : Component
{
    DownloadManager(bool hasUpdated) : updated(hasUpdated)
    {
        lnf.setType(TopButtonLNF::Type::Regular);
        addChildComponent(yes);
        yes.setLookAndFeel(&lnf);
        addChildComponent(no);
        no.setLookAndFeel(&lnf);
        addChildComponent(retry);
        retry.setLookAndFeel(&lnf);

        if (checkForUpdate() && !updated) {
            DBG("need update");
            //displayDownloadPrompt();
            setVisible(true);
        }
        else {
            setVisible(false);
        }

        no.onClick = [this] { setVisible(false); updated = true; onUpdateChange(updated); };
        yes.onClick = [this] { downloadFinished.store(false); downloadUpdate(); };
        
    }

    ~DownloadManager()
    {
        yes.setLookAndFeel(nullptr);
        no.setLookAndFeel(nullptr);
        retry.setLookAndFeel(nullptr);
    }

    bool checkForUpdate()
    {
        if (auto stream = URL(versionURL).createInputStream(URL::InputStreamOptions(URL::ParameterHandling::inAddress)))
        {
            auto latestVersion = stream->readEntireStreamAsString();
            
            DBG("Current: " << String(ProjectInfo::versionString).removeCharacters("."));
            DBG("Latest: " << latestVersion);
            
            return String(ProjectInfo::versionString).removeCharacters(".") != latestVersion;
        }
        else
            return false;
    }

    void displayDownloadPrompt()
    {
        yes.setVisible(true);
        no.setVisible(true);
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

    void paint(Graphics& g) override
    {
        if (isVisible()) {
            g.setColour(Colours::darkslategrey);
            g.fillRoundedRectangle(getLocalBounds().toFloat(), 15.f);
            
            g.setFont(getCustomFont(FontStyle::Regular).withHeight(17.f));
            g.setColour(Colour(0xa7a7a7a7));

            auto textbounds = Rectangle<int>{ getLocalBounds().withTrimmedBottom(getHeight() / 2).reduced(10) };

            if (!downloadFinished.load()) {
                if (!isDownloading.load())
                    g.drawFittedText("A new update is available! Would you like to download?",
                        textbounds, Justification::centred,
                        7, 1.f);
                else
                    g.drawFittedText("Downloading... " + String(downloadProgress.load(), 2) + "%",
                        textbounds, Justification::centred,
                        1, 1.f);
            }
            else {
                if (downloadStatus.load()) {
                    g.drawFittedText("Download complete.\nThe installer is in your Downloads folder. You must close your DAW to run the installation.",
                        textbounds, Justification::centred,
                        7, 1.f);
                    yes.setVisible(false);
                    no.setButtonText("Close");
                }
                else {
                    g.drawFittedText("Download failed. Please try again.",
                        textbounds, Justification::centred,
                        7, 1.f);
                    retry.setVisible(true);
                    retry.onClick = [this] {yes.onClick(); };
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

        Rectangle<int> yesBounds{ getLocalBounds().withTrimmedTop(halfHeight).withTrimmedRight(halfWidth)
                                .reduced(20, 30)};
        Rectangle<int> noBounds{ getLocalBounds().withTrimmedTop(halfHeight).withTrimmedLeft(halfWidth)
                                .reduced(20, 30) };
        Rectangle<int> retryBounds{ getLocalBounds().withTrimmedTop(halfHeight).reduced(20, 30) };

        yes.setBounds(yesBounds);
        no.setBounds(noBounds);
        retry.setBounds(retryBounds);
    }

    std::function<void(bool)> onUpdateChange;

private:
    gin::DownloadManager download;

    TextButton yes{ "Yes" }, no{ "No" }, retry{ "Retry" };

    TopButtonLNF lnf;

    bool updated = false;

    std::atomic<bool> downloadStatus = false;
    std::atomic<bool> isDownloading = false;
    std::atomic<bool> isInstalling = false;
    std::atomic<float> downloadProgress;
    std::atomic<bool> downloadFinished = false;

    std::function<void(gin::DownloadManager::DownloadResult)> result =
        [this](gin::DownloadManager::DownloadResult download)
    {
        repaint();

        downloadStatus.store(download.ok);

        if (!download.ok)
            return;

#if JUCE_WINDOWS
        auto exe = File("C:/Users/" + SystemStats::getLogonName() + "/Downloads/PiMax-windows.exe");
        
        if (!download.saveToFile(exe))
            downloadStatus.store(false);
        else
            downloadFinished.store(true);
#elif JUCE_MAC
        auto dmg = File("~/Downloads/PiMax-mac.dmg");

        if (!download.saveToFile(dmg))
            downloadStatus.store(false);
        else
            downloadFinished.store(true);
#endif
        
        //if (!stream.write(download.data.getData(), download.data.getSize()))
        //    downloadStatus.store(false);

//        auto unzip = File(File::getSpecialLocation(File::SpecialLocationType::tempDirectory)
//                    .getFullPathName() + "/tmp");
//
//        if (ZipFile(temp).uncompressTo(unzip)) {
//
//            gin::ElevatedFileCopy copy;
//
//            copy.deleteFile(temp);
//
//            auto vst3 = File(unzip.getFullPathName() + "/Maximizer.vst3");
//            auto vst = File(unzip.getFullPathName() + "/PiMax.dll");
//#if JUCE_WINDOWS
//            copy.moveFile(vst3, File("C:/Program Files/Common Files/VST3/Maximizer.vst3"));/*change filename back!!*/
//            copy.moveFile(vst3, File("C:/Program Files/Steinberg/VSTPlugins/PiMax.dll"));
//#endif
//            auto result = copy.execute(false);
//            downloadStatus.store(true);
//        }
//        else {
//            DBG("uncompress failed");
//            downloadStatus.store(false);
//        }

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
