#pragma once

#include "LinearFilter.h"
#include "LRFilter.h"

//using xs = xsimd::batch<float>;
using vec = dsp::SIMDRegister<float>;

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

    ~MultibandProcessor()
    {
    }

    void prepare(const dsp::ProcessSpec& spec)
    {
        lastSampleRate = spec.sampleRate;

        for (auto& band : bands)
            band.prepare(spec);

        auto linSpec = spec;
#if! USE_CONVOLUTION
        linSpec.numChannels = 1;
#endif
        for (int i = 0; i < 3; ++i)
            linBand[i].prepare(linSpec);

        for (auto& w : bandWidener)
            w.prepare(spec);

        for (int i = 0; i < 4; ++i) {
            lastInGain[i] = *bandInGain[i];
            lastOutGain[i] = *bandOutGain[i];
            lastBandWidth[i] = *bandWidth[i];
        }

        for (auto& m : mPi)
            m.prepare();
    }

    void updateSpecs(const dsp::ProcessSpec& newSpec)
    {
        lastSampleRate = newSpec.sampleRate;

        for (auto& band : bands)
            band.prepare(newSpec);

        coeffsUpdated[0] = false;
        coeffsUpdated[1] = false;
        coeffsUpdated[2] = false;
        for (auto& band : linBand)
            band.prepare(newSpec);

        initCrossovers();

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
            bands[i].reset();

            if (*crossovers[i] > (lastSampleRate * 0.5))
                linBand[i].initFilters(lastSampleRate * 0.5 - 1.0, lastSampleRate, 2);
            else
                linBand[i].initFilters(*crossovers[i], lastSampleRate, 2);
            //lastCrossover[i] = *crossovers[i];
            linBand[i].reset();
        }
        //lastNumBands = numBands;
    }

    inline void updateAllCrossovers(int newNumBands) noexcept
    {
        numBands = newNumBands;

        //coeffsUpdated[0] = false;
        //coeffsUpdated[1] = false;
        //coeffsUpdated[2] = false;
        for (int i = 0; i < 3; ++i) {
            if (!*linearPhase) {
                bands[i].updateFilter(*crossovers[i]);
                bands[i].reset();
            }

            else {
                linBand[i].setParams(*crossovers[i], lastSampleRate, 2);
                //linBand[i].reset();
            }
        }
        //lastNumBands = numBands;
    }

    inline void updateCrossoverNonLin(int crossover) noexcept
    {
        bands[crossover].updateFilter(*crossovers[crossover]);
    }

    inline void updateCrossoverLin(int crossover) noexcept
    {
        coeffsUpdated[crossover] = false;
        linBand[crossover].setParams(*crossovers[crossover], lastSampleRate, 2);
    }

    void setOversamplingFactor(int newFactor){
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

#if USE_SIMD_SAT
        T out[4];
        T yn;
#else
        T yn = 0.0;
#endif
        float gain[4] = {};
        float outGain[4] = {};
        float out[4] = {};
        float inc[4] = {};
        float outinc[4] = {};
        for (int i = 0; i <= numBands; ++i) {
            gain[i] = pow(10.f, *bandInGain[i] / 20.f);
            outGain[i] = pow(10.f, *bandOutGain[i] / 20.f);
            inc[i] = (gain[i] - lastInGain[i]) / (float)numSamples;
            outinc[i] = (outGain[i] - lastOutGain[i]) / (float)numSamples;
        }

        for (int ch = 0; ch < inputBlock.getNumChannels(); ++ch) {
            
            auto bandPtr = inputBlock.getChannelPointer(ch);
            for (int i = 0; i < numSamples; ++i) {

                for (int n = 0; n <= numBands; ++n) {

                    if (n != numBands) {
                        if (n == 0)
                            bands[n].processSample(ch, bandPtr[i], out[0], out[1]);
                        else
                            bands[n].processTwoSamples(ch, out[n], bandPtr[i], out[n], out[n + 1]);
                    }

                    if (!*bypassBand[n]) {
                        if (gain[n] != lastInGain[n]) {
                            out[n] *= lastInGain[n];
                            out[n] = mPi[n].processSample(ch, out[n]);
                            if (*autoGain)
                                out[n] *= 1.0 / lastInGain[n];
                            if (ch > 0)
                                lastInGain[n] += inc[n];
                        }
                        else {
                            out[n] *= gain[n];
                            out[n] = mPi[n].processSample(ch, out[n]);
                            if (*autoGain)
                                out[n] *= 1.0 / gain[n];
                        }
                    }
                    if (*muteBand[n])
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
        for (int i = 0; i <= numBands; ++i) {
            lastInGain[i] = gain[i];
            lastOutGain[i] = outGain[i];
        }

//        for (int channel = 0; channel < inputBlock.getNumChannels(); ++channel)
//        {
//            for (int i = 0; i < inputBlock.getNumSamples(); ++i)
//            {
//                T in = inputBlock.getSample(channel, i);
//
//                if (!*linearPhase)
//                    bands[0].processSample(channel, in, out[0], out[1]);
//            #if! USE_CONVOLUTION
//                else {
//                    if (!linBand[0].isThreadRunning() && coeffsUpdated[0]) {
//                        out[0] = linBand[0].processLowPass(channel, in);
//                        out[1] = linBand[0].processHighPass(channel, in);
//                    }
//                    else {
//                        out[0] = linBand[0].processLastLowPass(channel, in);
//                        out[1] = linBand[0].processLastHighPass(channel, in);
//                    }
//                }
//            #endif
//                if (!*bypassBand[0]) {
//                    auto gain = pow(10.f, (*bandInGain[0] / 20.f));
//                    auto invgain = 1.0 / gain;
//                    out[0] *= gain;
//                    out[0] = mPi[0].processSample(channel, out[0]);
//                    if (*autoGain)
//                        out[0] *= invgain;
//                }
//                out[0] *= pow(10.f, (*bandOutGain[0] / 20.f));
//                bandBuffer[0].setSample(channel, i, out[0]);
//
//                /*if (*soloBand[0])
//                    yn += out[0];*/
//                auto gain1 = pow(10.f, (*bandInGain[1] / 20.f));
//                auto invgain1 = 1.0 / gain1;
//                if (numBands < 2) {
//                    if (!*bypassBand[1]) {
//                        
//                        out[1] *= gain1;
//                        out[1] = mPi[1].processSample(channel, out[1]);
//                        if (*autoGain)
//                            out[1] *= invgain1;
//                    }
//                    out[1] *= pow(10.f, (*bandOutGain[1] / 20.f));
//                    bandBuffer[1].setSample(channel, i, out[1]);
//                    /*if (*soloBand[1])
//                         yn += out[1];*/
//                }
//
//                if (numBands > 1) {
//                    if (!*linearPhase)
//                        bands[1].processTwoSamples(channel, out[1], in, out[1], out[2]);
//#if! USE_CONVOLUTION
//                    else {
//                        if (!linBand[1].isThreadRunning() && coeffsUpdated[1]) {
//                            out[2] = linBand[1].processHighPass(channel, in);
//                            out[1] = out[1] - out[2];
//                        }
//                        else {
//                            out[2] = linBand[1].processLastHighPass(channel, in);
//                            out[1] = out[1] - out[2];
//                        }
//                    }
//#endif
//                    if (!*bypassBand[1]) {
//                        out[1] *= gain1;
//                        out[1] = mPi[1].processSample(channel, out[1]);
//                        if (*autoGain)
//                            out[1] *= invgain1;
//                    }
//                    out[1] *= pow(10.f, (*bandOutGain[1] / 20.f));
//                    bandBuffer[1].setSample(channel, i, out[1]);
//                    /*if (*soloBand[1])
//                        yn += out[1];*/
//
//                    if (numBands < 3) {
//                        if (!*bypassBand[2]) {
//                            auto gain = pow(10.f, (*bandInGain[2] / 20.f));
//                            auto invgain = 1.0 / gain;
//                            out[2] *= gain;
//                            out[2] = mPi[2].processSample(channel, out[2]);
//                            if (*autoGain)
//                                out[2] *= invgain;
//                        }
//                        out[2] *= pow(10.f, (*bandOutGain[2] / 20.f));
//                        bandBuffer[2].setSample(channel, i, out[2]);
//                        /*if (*soloBand[2])
//                            yn += out[2];*/
//                    }
//                }
//
//                if (numBands > 2) {
//                    if (!*linearPhase)
//                        bands[2].processTwoSamples(channel, out[2], in, out[2], out[3]);
//#if! USE_CONVOLUTION
//                    else {
//                        if (!linBand[2].isThreadRunning() && coeffsUpdated[2]) {
//                            out[3] = linBand[2].processHighPass(channel, in);
//                            out[2] = out[2] - out[3];
//                        }
//                        else {
//                            out[3] = linBand[2].processLastHighPass(channel, in);
//                            out[2] = out[2] - out[3];
//                        }
//                    }
//#endif
//
//                    if (!*bypassBand[2]) {
//                        auto gain = pow(10.f, (*bandInGain[2] / 20.f));
//                        auto invgain = 1.0 / gain;
//                        out[2] *= gain;
//                        out[2] = mPi[2].processSample(channel, out[2]);
//                        if (*autoGain)
//                            out[2] *= invgain;
//                    }
//                    out[2] *= pow(10.f, (*bandOutGain[2] / 20.f));
//                    bandBuffer[2].setSample(channel, i, out[2]);
//                    /*if (*soloBand[2])
//                        yn += out[2];*/
//
//                    if (!*bypassBand[3]) {
//                        auto gain = pow(10.f, (*bandInGain[3] / 20.f));
//                        auto invgain = 1.0 / gain;
//                        out[3] *= gain;
//                        out[3] = mPi[3].processSample(channel, out[3]);
//                        if (*autoGain)
//                            out[3] *= invgain;
//                    }
//                    out[3] *= pow(10.f, (*bandOutGain[3] / 20.f));
//                    bandBuffer[3].setSample(channel, i, out[3]);
//                    /*if (*soloBand[3])
//                        yn += out[3];*/
//                }
//            }
//        }

        /*widen any bands which need to be*/
        for (int i = 0; i <= numBands ; ++i)
        {
            if (*bandWidth[i] != 1.0)
                bandWidener[i].widenBuffer(bandBuffer[i], *bandWidth[i], *monoWidth);
        }

        const double r = 1.0 - (1.0 / (float)(oversampleFactor * 1000.0));

        if (*soloBand[0] || *soloBand[1] || *soloBand[2] || *soloBand[3]) {
            for (int channel = 0; channel < inputBlock.getNumChannels(); ++channel)
            {
                for (int i = 0; i < inputBlock.getNumSamples(); ++i)
                {
                    yn = 0.0;
#if USE_SIMD_SAT
                    vec out = 0.0;
#else
                    T out = 0.0;
#endif
                    for (int b = 0; b <= numBands; ++b)
                        if (*soloBand[b])
                            out += bandBuffer[b].getSample(channel, i);

                    if (*distIndex == 1) {
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
        if (!*soloBand[0] && !*soloBand[1] && !*soloBand[2] && !*soloBand[3]) {
            for (int channel = 0; channel < inputBlock.getNumChannels(); ++channel)
            {
                for (int i = 0; i < inputBlock.getNumSamples(); ++i)
                {
                    yn = 0.0;
#if USE_SIMD_SAT
                    vec out = 0.0;
#else
                    T out = 0.0;
#endif
                    for (int b = 0; b <= numBands; ++b)
                        out += bandBuffer[b].getSample(channel, i);

                    if (*distIndex == 1) {
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
        std::array<dsp::AudioBlock<T>, 4> band
        { {
            {bandBuffer[0]},
            {bandBuffer[1]},
            {bandBuffer[2]},
            {bandBuffer[3]},
        } };

        /*convolving bands and adding them to their respective buffers, then applying gains & saturating*/
        for (size_t n = 0; n <= numBands; ++n) {

            if (n != numBands) {
                linBand[n].loadBuffers();

                if (n == 0) {
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
            //float inc = 0;
            if (*bandWidth[i] != 1.0) {
                //if (lastBandWidth[i] != *bandWidth[i]) {
                //    auto inc = (*bandWidth[i] - lastBandWidth[i]) / (float)(lastSampleRate / numSamples);
                //    bandWidener[i].widenBlock(band[i], lastBandWidth[i], *monoWidth);
                //    lastBandWidth[i] += inc;
                //}
                //else
                bandWidener[i].widenBlock(band[i], *bandWidth[i], *monoWidth);
            }
            //band[i] = bandWidener[i].widenBlock(band[i], *bandWidth[i], *monoWidth);
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

    std::array<strix::FIRThread, 3> linBand
    { {
        {strix::FIRThread(true, 512, q)},
        {strix::FIRThread(false, 512, q)},
        {strix::FIRThread(false, 512, q)}
    } };

    std::array<StereoWidener, 4> bandWidener;

#if USE_SIMD_SAT
    std::array<AudioBuffer<vec>, 4> bandBuffer;
#else
    std::array<AudioBuffer<float>, 4> bandBuffer;
#endif
private:

    int numBands = 2;
    int oversampleFactor = 0;

    double lastSampleRate = 0.0;
#if USE_SIMD_SAT
    vec xm1[2], ym1[2];
#else
    double xm1[2]{ 0.0, 0.0 };
    double ym1[2]{ 0.0, 0.0 };
#endif
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
