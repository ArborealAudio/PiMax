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

    void setOwner(const String& nameToDisplay)
    {
        name = nameToDisplay;
    }

    void paint(Graphics& g) override
    {
        Rectangle<float> bounds{ 12.f, 12.f, 66.f, 65.f };
        logo->drawWithin(g, bounds, RectanglePlacement::centred, 1.f);

        blur(g);
    }

    bool hitTest(int x, int y) override
    {
        if (logo->getBounds().contains((float)x, (float)y) && onLogoClick != nullptr && !splashOn &&
            ModifierKeys::currentModifiers.isLeftButtonDown())
        {
            onLogoClick();
            return true;
        }
        else if (splashOn && clipArea.contains((float)x, (float)y))
        {
            if (ModifierKeys::currentModifiers.isLeftButtonDown()) {
                if (clipArea.reduced(50.f).withTrimmedBottom(70.f).contains(x, y))
                    return true;
                else {
                    splashOn = false;
                    repaint();
                }
            }
            return true;
        }
        else if (splashOn && !clipArea.contains((float)x, (float)y) && !logo->getBounds().contains((float)x, (float)y))
            return true;
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

    inline void blur(Graphics& g) noexcept
    {
        if (needBlur) {
            gin::applyStackBlur(img, 35);
            needBlur = false;
        }

        if (splashOn) {
            g.fillAll(Colours::black.withAlpha(0.33f));
            g.drawImage(img, clipArea, RectanglePlacement::centred);
            g.setColour(Colours::darkslategrey.withAlpha(0.25f));
            g.fillRect(clipArea);
            g.setColour(Colour(0xa7a7a7a7));
            g.drawRoundedRectangle(clipArea, 5.f, 1.5f);

            logo->drawWithin(g, clipArea.reduced(50.f), RectanglePlacement::yTop, 1.f);

            g.setFont(getCustomFont(FontStyle::Regular).withHeight(16.f));
            Time time(Time::getCurrentTime());
            g.drawFittedText("Licensed to: " + name +
                "\nv" + String(ProjectInfo::versionString) + 
                "\n(c) Arboreal Audio " + String(time.getYear()),
                clipArea.withTrimmedTop(150.f).toNearestInt(),
                Justification::centred, 3, 1.f);

            if (clipArea.reduced(50.f).withTrimmedBottom(70.f).contains(getMouseXYRelative().toFloat())) {
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

    std::function<void()> onLogoClick;

private:
        
    bool needBlur = false;
    bool splashOn = false;

    Rectangle<float> clipArea{ 250.f, 90.f, 225.f, 300.f };

    Image img;

    std::unique_ptr<Drawable> logo;

    String name;
};
