#pragma once

#include "LinearFilter.h"
#include "LRFilter.h"

struct MultibandProcessor
{
	MultibandProcessor(AudioProcessorValueTreeState& vts) : apvts(vts)
    {
        for (int i = 0; i < 3; ++i)
        {
            bands[i] = (LRFilter());
            crossovers[i] = apvts.getRawParameterValue("crossover" + std::to_string(i));
        }

        for (int i = 0; i < 4; ++i)
        {
            bandInGain[i] = (apvts.getRawParameterValue("bandInGain" + std::to_string(i)));
            bandOutGain[i] = (apvts.getRawParameterValue("bandOutGain" + std::to_string(i)));
            bandWidth[i] = (apvts.getRawParameterValue("bandWidth" + std::to_string(i)));
            soloBand[i] = (apvts.getRawParameterValue("soloBand" + std::to_string(i)));
            muteBand[i] = (apvts.getRawParameterValue("muteBand" + std::to_string(i)));
            bypassBand[i] = (apvts.getRawParameterValue("bypassBand" + std::to_string(i)));
        }

        linearPhase = apvts.getRawParameterValue("linearPhase");
        curve = apvts.getRawParameterValue("curve");
        clip = apvts.getRawParameterValue("clipType");
        distIndex = apvts.getRawParameterValue("distType");
        monoWidth = apvts.getRawParameterValue("monoWidth");
        autoGain = apvts.getRawParameterValue("autoGain");
    }

    void prepare(const dsp::ProcessSpec& spec)
    {
        lastSampleRate = spec.sampleRate;

        for (auto& band : bands)
            band.prepare(spec);

        for (int i = 0; i < 3; ++i)
            linBand[i].prepare(spec);

        for (auto& w : bandWidener)
            w.prepare(spec);

        for (int i = 0; i < 4; ++i) {
            lastInGain[i] = 1.0;
            lastOutGain[i] = 1.0;
            lastBandWidth[i] = 1.0;
        }

        for (auto& m : mPi)
            m.prepare();
    }

    void reset()
    {
        for (auto& band : bands)
            band.reset();
        for (auto& lb : linBand)
            lb.reset();

        xm1[0] = 0.0, xm1[1] = 0.0, ym1[0] = 0.0, ym1[1] = 0.0;
    }

    void updateSpecs(const dsp::ProcessSpec& newSpec)
    {
        lastSampleRate = newSpec.sampleRate;

        for (auto& band : bands)
            band.prepare(newSpec);

        for (auto& band : linBand)
            band.prepare(newSpec);

        initCrossovers();
        
        for (auto& w: bandWidener)
            w.prepare(newSpec);

        for (auto& m : mPi)
            m.prepare();
    }

    /*passes crossover points to filter objects*/
    inline void initCrossovers() noexcept
    {
        for (int i = 0; i < 3; ++i) {
            if (*crossovers[i] > (lastSampleRate * 0.5))
                bands[i].updateFilter(lastSampleRate * 0.5 - 1.0);
            else
                bands[i].updateFilter(*crossovers[i]);

            if (*crossovers[i] > (lastSampleRate * 0.5))
                linBand[i].initFilters(lastSampleRate * 0.5 - 1.0, lastSampleRate, 2);
            else
                linBand[i].initFilters(*crossovers[i], lastSampleRate, 2);
        }
    }

    inline void updateAllCrossovers(int newNumBands) noexcept
    {
        numBands = newNumBands;

        for (int i = 0; i < 3; ++i) {
            if (!*linearPhase) {
                bands[i].updateFilter(*crossovers[i]);
                bands[i].reset();
            }

            else {
                linBand[i].setParams(*crossovers[i], lastSampleRate, 2);
            }
        }
    }

    inline void updateCrossoverNonLin(int crossover) noexcept
    {
        bands[crossover].updateFilter(*crossovers[crossover]);
    }

    inline void updateCrossoverLin(int crossover) noexcept
    {
        linBand[crossover].setParams(*crossovers[crossover], lastSampleRate, 2);
    }

    void setOversamplingFactor(int newFactor)
    {
        oversampleFactor = newFactor;
        for (auto& b : linBand)
            b.setOversampleFactor(oversampleFactor);
    }

    template <typename T>
	inline void addBands(const dsp::ProcessContextReplacing<T>& context) noexcept
	{
        const auto& inputBlock = context.getInputBlock();
        const auto numSamples = inputBlock.getNumSamples();
        auto& outputBlock = context.getOutputBlock();

        for (auto& b : bandBuffer)
            b.setSize(b.getNumChannels(), numSamples, true, false, true);

        T yn = 0.0;

        float gain[4] = {0.0};
        float outGain[4] = {0.0};
        bool autoGain_m = *autoGain;
        float out[4] = {0.0};
        float inc[4] = {0.0};
        float outinc[4] = {0.0};
        bool solo[4] = {false}, mute[4]{false}, bypass[4]{false};
        float width[4] = {0.0};
        bool monoWidth_m = *monoWidth;

        for (int i = 0; i <= numBands; ++i)
        {
            mPi[i].loadAtomics();
            gain[i] = pow(10.f, *bandInGain[i] * 0.05f);
            outGain[i] = pow(10.f, *bandOutGain[i] * 0.05f);
            inc[i] = (gain[i] - lastInGain[i]) / (float)numSamples;
            outinc[i] = (outGain[i] - lastOutGain[i]) / (float)numSamples;
            solo[i] = *soloBand[i];
            mute[i] = *muteBand[i];
            bypass[i] = *bypassBand[i];
            width[i] = *bandWidth[i];
        }

        for (int ch = 0; ch < inputBlock.getNumChannels(); ++ch)
        {
            auto bandPtr = inputBlock.getChannelPointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                for (int n = 0; n <= numBands; ++n)
                {
                    if (n != numBands) {
                        if (n == 0)
                            bands[n].processSample(ch, bandPtr[i], out[0], out[1]);
                        else
                            bands[n].processTwoSamples(ch, out[n], bandPtr[i], out[n], out[n + 1]);
                    }

                    if (!bypass[n]) {
                        if (gain[n] != lastInGain[n]) {
                            out[n] *= lastInGain[n];
                            out[n] = mPi[n].processSample(ch, out[n]);
                            if (autoGain_m)
                                out[n] *= 1.0 / lastInGain[n];
                            if (ch > 0)
                                lastInGain[n] += inc[n];
                        }
                        else {
                            out[n] *= gain[n];
                            out[n] = mPi[n].processSample(ch, out[n]);
                            if (autoGain_m)
                                out[n] *= 1.0 / gain[n];
                        }
                        //out[n] /= halfPi; pulled out halfPi scaledown as it was subtracting too much
                    }
                    if (mute[n])
                        out[n] = 0.0;

                    if (outGain[n] != lastOutGain[n]) {
                        out[n] *= lastOutGain[n];
                        if (ch > 0)
                            lastOutGain[n] += outinc[n];
                    }
                    else out[n] *= outGain[n];

                    bandBuffer[n].setSample(ch, i, out[n]);
                }
            }
        }
        
        /*ensure last gains are set to cooked value of corresponding atomics*/
        for (int i = 0; i <= numBands; ++i)
        {
            lastInGain[i] = gain[i];
            lastOutGain[i] = outGain[i];
        }

        /*widen any bands which need to be*/
        for (int i = 0; i <= numBands ; ++i)
        {
            if (width[i] != 1.0) {
                if (inputBlock.getNumChannels() > 1) {
                    if (width[i] != lastBandWidth[i]) {
                        bandWidener[i].widenBufferWithRamp(bandBuffer[i], lastBandWidth[i], width[i], monoWidth_m);
                        lastBandWidth[i] = width[i];
                    }
                    else
                        bandWidener[i].widenBuffer(bandBuffer[i], width[i], monoWidth_m);
                }
                else {
                    if (width[i] != lastBandWidth[i]) {
                        bandWidener[i].widenBufferWithRamp(bandBuffer[i], lastBandWidth[i], width[i], true);
                        lastBandWidth[i] = width[i];
                    }
                    else
                        bandWidener[i].widenBuffer(bandBuffer[i], width[i], true);
                }
            }
        }

        const double r = 1.0 - (1.0 / (float)(oversampleFactor * 1000));

        if (solo[0] || solo[1] || solo[2] || solo[3])
        {
            bool distIndex_m = distIndex->load();

            for (int channel = 0; channel < inputBlock.getNumChannels(); ++channel)
            {
                for (int i = 0; i < numSamples; ++i)
                {
                    yn = 0.0;
                    T out = 0.0;

                    for (int b = 0; b <= numBands; ++b)
                        if (solo[b])
                            out += bandBuffer[b].getSample(channel, i);

                    if (distIndex_m) {
                        /*DC Blocker*/
                        yn = out;
                        out = yn - xm1[channel] + r * ym1[channel];
                        xm1[channel] = yn;
                        ym1[channel] = out;
                    }

                    outputBlock.setSample(channel, i, out);
                }
            }
        }

        /*add all relevent bands to output, make sure solo is not on for any band*/
        if (!solo[0] && !solo[1] && !solo[2] && !solo[3])
        {
            bool distIndex_m = distIndex->load();

            for (int channel = 0; channel < inputBlock.getNumChannels(); ++channel)
            {
                for (int i = 0; i < numSamples; ++i)
                {
                    yn = 0.0;
                    T out = 0.0;

                    for (int b = 0; b <= numBands; ++b)
                        out += bandBuffer[b].getSample(channel, i);

                    if (distIndex_m) {
                        /*DC Blocker*/
                        yn = out;
                        out = yn - xm1[channel] + r * ym1[channel];
                        xm1[channel] = yn;
                        ym1[channel] = out;
                    }

                    outputBlock.setSample(channel, i, out);
                }
            }
        }
	}

    template <typename T>
    inline void addBandsConvolution(const dsp::ProcessContextReplacing<T>& context) noexcept
    {
        const auto& inputBlock = context.getInputBlock();
        int numSamples = inputBlock.getNumSamples();
        auto& outputBlock = context.getOutputBlock();

        for (auto& b : bandBuffer)
            if (b.getNumSamples() != numSamples)
                b.setSize(b.getNumChannels(), numSamples, true, false, true);
        
        std::array<dsp::AudioBlock<T>, 4> band
        { {
            {bandBuffer[0]},
            {bandBuffer[1]},
            {bandBuffer[2]},
            {bandBuffer[3]},
        } };

        /*convolving bands and adding them to their respective buffers, then applying gains & saturating*/
        for (size_t n = 0; n <= numBands; ++n) {

            mPi[n].loadAtomics();

            if (n != numBands) {
                linBand[n].loadBuffers();

                if (n == 0)
                {
                    linBand[n].convolveLow(dsp::ProcessContextReplacing<T>(band[n]));
                    linBand[n].convolveHigh(dsp::ProcessContextReplacing<T>(band[n + 1]));
                }
                else
                    linBand[n].convolveHigh(dsp::ProcessContextReplacing<T>(band[n + 1]));
            }

            auto gain = pow(10.f, *bandInGain[n] / 20.f);
            auto outGain = pow(10.f, *bandOutGain[n] / 20.f);
            for (int ch = 0; ch < inputBlock.getNumChannels(); ++ch)
            {
                auto bandPtr = band[n].getChannelPointer(ch);
                auto m_lastGain = lastInGain[n];
                auto m_lastOut = lastOutGain[n];
                auto inc = (gain - m_lastGain) / (float)numSamples;
                auto outinc = (outGain - m_lastOut) / (float)numSamples;
                for (int i = 0; i < numSamples; ++i)
                {
                    if (n > 0 && numBands > n)
                        bandPtr[i] = bandPtr[i] - band[n + 1].getSample(ch, i);

                    if (!*bypassBand[n]) {
                        if (gain != m_lastGain) {
                            bandPtr[i] *= m_lastGain;
                            bandPtr[i] = mPi[n].processSample(ch, bandPtr[i]);
                            if (*autoGain)
                                bandPtr[i] *= 1.0 / m_lastGain;
                            m_lastGain += inc;
                        }
                        else {
                            bandPtr[i] *= gain;
                            bandPtr[i] = mPi[n].processSample(ch, bandPtr[i]);
                            if (*autoGain)
                                bandPtr[i] *= 1.0 / gain;
                        }
                        //bandPtr[i] /= halfPi - (1.f / (float)(numBands + 1));
                    }
                    if (*muteBand[n])
                        bandPtr[i] = 0.0;
                        
                    if (outGain != m_lastOut) {
                        bandPtr[i] *= m_lastOut;
                        m_lastOut += outinc;
                    }
                    else bandPtr[i] *= outGain;
                }
            }
            lastInGain[n] = gain;
            lastOutGain[n] = outGain;
        }

        /*widen bands*/
        for (int i = 0; i <= numBands; ++i)
        {
            if (*bandWidth[i] != 1.0 && inputBlock.getNumChannels() > 1) {
                if (lastBandWidth[i] != *bandWidth[i]) {
                    bandWidener[i].widenBufferWithRamp(bandBuffer[i], lastBandWidth[i], *bandWidth[i], *monoWidth);
                    lastBandWidth[i] = *bandWidth[i];
                }
                else
                    bandWidener[i].widenBlock(band[i], *bandWidth[i], *monoWidth);
            }
        }

        const double r = 1.0 - (1.0 / (float)(oversampleFactor * 1000.0));

        /*if any solo conditions present, add relevant bands*/
        if (*soloBand[0] || *soloBand[1] || *soloBand[2] || *soloBand[3]) {
            for (int channel = 0; channel < inputBlock.getNumChannels(); ++channel)
            {
                for (int i = 0; i < numSamples; ++i)
                {
#if USE_SIMD_SAT
                    vec out_m, out;
#else
                    T out_m = 0.0;
                    T out = 0.0;
#endif
                    for (int b = 0; b <= numBands; ++b)
                        if (*soloBand[b])
                            out += band[b].getSample(channel, i);

                    if (*distIndex == 1) {
                        /*DC Blocker*/
                        out_m = out;
                        out = out_m - xm1[channel] + r * ym1[channel];
                        xm1[channel] = out_m;
                        ym1[channel] = out;
                    }

                    outputBlock.setSample(channel, i, out);
                }
            }
        }

        /*if no solo conditions present, add all relevant bands*/
        if (!*soloBand[0] && !*soloBand[1] && !*soloBand[2] && !*soloBand[3]) {
            for (int channel = 0; channel < inputBlock.getNumChannels(); ++channel)
            {
                for (int i = 0; i < numSamples; ++i)
                {
#if USE_SIMD_SAT
                    vec out_m, out;
#else
                    T out_m = 0.0;
                    T out = 0.0;
#endif
                    for (int b = 0; b <= numBands; ++b)
                        out += band[b].getSample(channel, i);

                    if (*distIndex == 1) {
                        /*DC Blocker*/
                        out_m = out;
                        out = out_m - xm1[channel] + r * ym1[channel];
                        xm1[channel] = out_m;
                        ym1[channel] = out;
                    }

                    outputBlock.setSample(channel, i, out);
                }
            }
        }
    }

    std::array<std::atomic<float>*, 4> muteBand, soloBand, bypassBand, bandInGain, bandOutGain, bandWidth;
    std::array<float, 4> lastInGain, lastOutGain, lastBandWidth;
    std::array<std::atomic<float>*, 3> crossovers;

    std::array<LRFilter, 3> bands;

    dsp::ConvolutionMessageQueue q{ 16000 };

    std::array<strix::LinearFilter::FIRThread, 3> linBand
    { {
        {strix::LinearFilter::FIRThread(true,  2048, q)},
        {strix::LinearFilter::FIRThread(false, 2048, q)},
        {strix::LinearFilter::FIRThread(false, 2048, q)}
    } };

    std::array<StereoWidener, 4> bandWidener;

    std::array<AudioBuffer<float>, 4> bandBuffer;

private:

    int numBands = 2;
    int oversampleFactor = 0;

    double lastSampleRate = 0.0;

    double xm1[2]{ 0.0, 0.0 };
    double ym1[2]{ 0.0, 0.0 };

    bool coeffsUpdated[3];

    std::atomic<float>* linearPhase, *curve, *clip, *distIndex, *monoWidth, *autoGain;

	AudioProcessorValueTreeState& apvts;

    std::array<MaximPizer, 4> mPi
    { {
        {MaximPizer(apvts)},
        {MaximPizer(apvts)},
        {MaximPizer(apvts)},
        {MaximPizer(apvts)}
    } };
};
