// WaveshaperComponent.cpp

#include "WaveshaperComponent.h"

WaveshaperComponent::WaveshaperComponent(MaximizerAudioProcessor& p)
    : audioProcessor(p)
{
    p.apvts.addParameterListener("gain", this);
    p.apvts.addParameterListener("output", this);
    p.apvts.addParameterListener("curve", this);
    p.apvts.addParameterListener("clipType", this);
    p.apvts.addParameterListener("distType", this);
    p.apvts.addParameterListener("bandSplit", this);
    startTimerHz(60);

     //setBufferedToImage(true);
}

WaveshaperComponent::~WaveshaperComponent()
{
    audioProcessor.apvts.removeParameterListener("gain", this);
    audioProcessor.apvts.removeParameterListener("output", this);
    audioProcessor.apvts.removeParameterListener("curve", this);
    audioProcessor.apvts.removeParameterListener("clipType", this);
    audioProcessor.apvts.removeParameterListener("distType", this);
    audioProcessor.apvts.removeParameterListener("bandSplit", this);
    stopTimer();
}

void WaveshaperComponent::paint(Graphics& g)
{
    if (*audioProcessor.bandSplit)
        return;

    Path p;

    auto w = getWidth();

    float gain = pow(10.f, (audioProcessor.gain_dB->get() / 20.f));
    float curve = audioProcessor.curve->get();

    ColourGradient gradient(Colour(0xa7a7a7a7).withAlpha(gain / 8.f),
        getLocalBounds().getCentreX(), getLocalBounds().getCentreY(),
        Colours::transparentWhite, getLocalBounds().getX(), getLocalBounds().getY(), true);

    if (curve > 1.0) {
        auto fac = jmap(curve, 1.f, 1.2f, 0.f, 1.f);
        gradient.setColour(1, Colours::transparentWhite.interpolatedWith(Colours::seagreen, fac / 3.f));
    }
    else if (curve < 1.0) {
        auto fac = jmap(curve, 0.83f, 1.f, 0.f, 1.f);
        gradient.setColour(1, Colours::transparentWhite.interpolatedWith(Colours::teal, (1.f - fac) / 3.f));
    }

    g.setGradientFill(gradient);
    g.fillAll();

    ClipType clip = (ClipType)audioProcessor.clip->load();
    auto distIndex = audioProcessor.distType->load();
    auto outGain = pow(10.f, audioProcessor.output_dB->get() * 0.05f);

    for (int n = 0; n < w; ++n)
        mag[n] = jmap(float(n) / float(w), -1.f, 1.f);

    FloatVectorOperations::multiply(mag.data(), gain, mag.size());
    waveshapers::inflator_v2(mag.data(), mag.size(), curve, clip, distIndex);
    FloatVectorOperations::multiply(mag.data(), outGain, mag.size());

    const float outputMin = getLocalBounds().getBottom();
    const float outputMax = getLocalBounds().getY();
    auto map = [outputMax, outputMin](float input)
    {
        return jmap(input, -1.f, 1.f, outputMin, outputMax);
    };
    p.startNewSubPath(getLocalBounds().getX(), map(mag.front()));
    for (size_t i = 1; i < mag.size(); ++i)
    {
        p.lineTo(getLocalBounds().getX() + i, map(mag[i]));
    }

    g.setColour(Colours::floralwhite);
    g.strokePath(p, PathStrokeType(2.f));

    g.drawRoundedRectangle(getLocalBounds().toFloat(), 4.f, 1.f);
}

void WaveshaperComponent::resized()
{
    auto w = getWidth();
    if (w != mag.size())
        mag.resize(w);
}

void WaveshaperComponent::parameterChanged(const String& parameterID, float newValue)
{
    paramChanged = true;
}

void WaveshaperComponent::timerCallback()
{
    if (getWidth() != mag.size())
        mag.resize(getWidth());
    if (paramChanged) {
        paramChanged = false;
        repaint();
    }
}