/*
  ==============================================================================

    Splash.h
    Created: 17 Oct 2021 10:44:59pm
    Author:  alexb

  ==============================================================================
*/

#pragma once

struct Splash : Component
{
    Splash()
    {
        logo = Drawable::createFromImageData(BinaryData::arboreal_tree_svg, BinaryData::arboreal_tree_svgSize);
        Rectangle<float> bounds{ 12.f, 12.f, 66.f, 65.f };
        logo->setBounds(bounds.toNearestInt());
    }

    void paint(Graphics& g) override
    {
        Rectangle<float> bounds{ 12.f, 12.f, 66.f, 65.f };
        logo->drawWithin(g, bounds, RectanglePlacement::centred, 1.f);

        if (needBlur) {
            //gin::applyStackBlur(img, 35);
            Blur::blurImage<4, true>(img);
            needBlur = false;
        }

        if (splashOn) {
            g.fillAll(Colours::black.withAlpha(0.33f));
            g.drawImage(img, clipArea, RectanglePlacement::centred);
            g.setColour(Colours::darkslategrey.withAlpha(0.25f));
            g.fillRect(clipArea);
            g.setColour(Colour(0xa7a7a7a7));
            g.drawRoundedRectangle(clipArea, 5.f, 1.5f);

            if (clipArea.reduced(50.f).withTrimmedBottom(70.f).contains(getMouseXYRelative().toFloat()))
            {
                g.setColour(Colours::darkslategrey);
                g.fillRoundedRectangle(clipArea.reduced(50.f).withTrimmedBottom(70.f), 5.f);
                repaint();
                setMouseCursor(MouseCursor::PointingHandCursor);
                if (isMouseButtonDown()) {
                    URL("https://arborealaudio.com").launchInDefaultBrowser();
                    splashOn = false;
                    repaint();
                    return;
                }
            }
            else
                setMouseCursor(MouseCursor::NormalCursor);

            g.setColour(Colour(0xa7a7a7a7));

            logo->drawWithin(g, clipArea.reduced(50.f), RectanglePlacement::yTop, 1.f);

            auto wrapperStr = AudioProcessor::getWrapperTypeDescription(currentWrapper);
            if (currentWrapper == AudioProcessor::wrapperType_Undefined)
                wrapperStr = "CLAP";

            g.setFont(getCustomFont(FontStyle::Regular).withHeight(16.f));
            Time time(Time::getCurrentTime());
            g.drawFittedText("v" + String(ProjectInfo::versionString) + 
                "\n" + wrapperStr +
                "\n(c) Arboreal Audio " + String(time.getYear()),
                clipArea.withTrimmedTop(150.f).toNearestInt(),
                Justification::centred, 3, 1.f);
        }
    }

    bool hitTest(int x, int y) override
    {
        /* left click on logo */
        if (logo->getBounds().contains((float)x, (float)y) && onLogoClick && !splashOn &&
            ModifierKeys::currentModifiers.isLeftButtonDown())
        {
            onLogoClick();
            return true;
        }
        /* mouse inside splash */
        else if (splashOn && clipArea.contains((float)x, (float)y))
        {
            if (ModifierKeys::currentModifiers.isLeftButtonDown()) {
                if (clipArea.reduced(50.f).withTrimmedBottom(70.f).contains(x, y))
                    return true;
            }
            return true;
        }
        /* mouse outside splash and logo bounds */
        else if (splashOn && !clipArea.contains((float)x, (float)y) && !logo->getBounds().contains((float)x, (float)y))
        {
            if (ModifierKeys::currentModifiers.isLeftButtonDown()) {
                splashOn = false;
                repaint();
                return true;
            }
            return true;
        }
        else
            return false;
    }

    void setImage(const Image& newImage) noexcept
    {
        img = newImage;
        
        auto bounds = getParentComponent()->getLocalArea(this, clipArea);

        img = img.getClippedImage(bounds.toNearestInt());

        needBlur = true;
        splashOn = true;
        repaint();
    }

    void setPluginWrapperType(AudioProcessor::WrapperType type)
    {
        currentWrapper = type;
    }

    std::function<void()> onLogoClick;

private:
        
    bool needBlur = false;
    bool splashOn = false;

    Rectangle<float> clipArea{ 250.f, 90.f, 225.f, 300.f };

    Image img;

    AudioProcessor::WrapperType currentWrapper;

    std::unique_ptr<Drawable> logo;
};
