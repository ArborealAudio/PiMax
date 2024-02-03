#pragma once
#include <JuceHeader.h>

#include "LRFilter.h"
#include "LinearFilter.hpp"
#include "MaximPizer.h"
#include "StereoWidener.h"

struct MultibandProcessor
{
    static constexpr int TOTAL_BANDS = 4;
    static constexpr int TOTAL_XOVER = 3;

    MultibandProcessor(AudioProcessorValueTreeState &vts) : apvts(vts)
    {
        for (size_t i = 0; i < TOTAL_XOVER; ++i) {
            bands[i] = (LRFilter<float>());
            crossovers[i] = dynamic_cast<strix::FloatParameter *>(
                apvts.getParameter("crossover" + std::to_string(i)));
        }

        for (int i = 0; i < TOTAL_BANDS; ++i) {
            bandInGain[i] = dynamic_cast<strix::FloatParameter *>(
                apvts.getParameter("bandInGain" + std::to_string(i)));
            bandOutGain[i] = dynamic_cast<strix::FloatParameter *>(
                apvts.getParameter("bandOutGain" + std::to_string(i)));
            bandWidth[i] = dynamic_cast<strix::FloatParameter *>(
                apvts.getParameter("bandWidth" + std::to_string(i)));
            soloBand[i] =
                (apvts.getRawParameterValue("soloBand" + std::to_string(i)));
            muteBand[i] =
                (apvts.getRawParameterValue("muteBand" + std::to_string(i)));
            bypassBand[i] =
                (apvts.getRawParameterValue("bypassBand" + std::to_string(i)));
        }

        // init band buffers to their logical maximum (4096 * 4x oversample)
        for (auto &b : bandBuffer)
            b.setSize(2, 4096 * 4);

        linearPhase = apvts.getRawParameterValue("linearPhase");
        curve = apvts.getRawParameterValue("curve");
        clip = apvts.getRawParameterValue("clipType");
        distIndex = apvts.getRawParameterValue("distType");
        monoWidth = apvts.getRawParameterValue("monoWidth");
        autoGain = apvts.getRawParameterValue("autoGain");
    }

    void prepare(const dsp::ProcessSpec &spec)
    {
        lastSampleRate = spec.sampleRate;

        for (auto &buffer : bandBuffer) {
            buffer.setSize(spec.numChannels, spec.maximumBlockSize, false, true,
                           true);
        }

        for (auto &band : bands)
            band.prepare(spec);

        for (auto &lBand : linBand)
            lBand.prepare(spec);

        initCrossovers();

        for (auto &w : bandWidener)
            w.prepare(spec);

        for (int i = 0; i < TOTAL_BANDS; ++i) {
            lastInGain[i] = 1.0;
            lastOutGain[i] = 1.0;
            lastBandWidth[i] = 1.0;
        }

        for (auto &m : mPi)
            m.prepare(spec);
    }

    void reset()
    {
        for (auto &band : bands)
            band.reset();
        for (auto &lb : linBand)
            lb.reset();

        xm1[0] = 0.0, xm1[1] = 0.0, ym1[0] = 0.0, ym1[1] = 0.0;
    }

    // like prepare(), but does not resize the audio buffers
    void updateSpecs(const dsp::ProcessSpec &newSpec)
    {
        lastSampleRate = newSpec.sampleRate;

        for (auto &band : bands)
            band.prepare(newSpec);

        for (auto &band : linBand)
            band.prepare(newSpec);

        initCrossovers();

        for (auto &w : bandWidener)
            w.prepare(newSpec);

        for (auto &m : mPi)
            m.prepare(newSpec);
    }

    /* init filters from current param values */
    void initCrossovers()
    {
        for (size_t i = 0; i < TOTAL_XOVER; ++i) {
            auto crossover = *crossovers[i] > (lastSampleRate * 0.5)
                                 ? lastSampleRate * 0.5 - 1.0
                                 : *crossovers[i];
            bands[i].updateFilter(crossover);
            linBand[i].updateFilters(crossover, lastSampleRate);
        }
    }

    // update filters & update num bands
    void updateAllCrossovers(int newNumBands)
    {
        numBands = newNumBands;

        for (size_t i = 0; i < TOTAL_XOVER; ++i) {
            if (!*linearPhase) {
                bands[i].updateFilter(*crossovers[i]);
                bands[i].reset();
            }

            else {
                linBand[i].updateFilters(*crossovers[i], lastSampleRate);
            }
        }
    }

    void updateCrossoverNonLin(int crossoverID)
    {
        bands[crossoverID].updateFilter(*crossovers[crossoverID]);
    }

    void updateCrossoverLin(int crossoverID)
    {
        linBand[crossoverID].updateFilters(*crossovers[crossoverID],
                                         lastSampleRate);
    }

    inline void processFilters(int n, size_t numSamples)
    {
        auto lowBand =
            dsp::AudioBlock<float>(bandBuffer[n]).getSubBlock(0, numSamples);
        dsp::AudioBlock<float> highBand;
        if (n != numBands)
            highBand = dsp::AudioBlock<float>(bandBuffer[n + 1])
                           .getSubBlock(0, numSamples);

        if (*linearPhase) {
            if (n != numBands) {
                linBand[n].loadBuffersIntoConvolution();

                if (n == 0) {
                    linBand[n].convolveLow(lowBand);
                    linBand[n].convolveHigh(highBand);
                } else {
                    linBand[n].convolveHigh(highBand);
                }
            }

            if (n > 0 && numBands > n) {
                for (size_t ch = 0; ch < lowBand.getNumChannels(); ++ch) {
                    FloatVectorOperations::subtract(
                        lowBand.getChannelPointer(ch),
                        highBand.getChannelPointer(ch), numSamples);
                }
            }
        } else {
            if (n != numBands) {
                bands[n].process<false>(lowBand, highBand);
            }
        }
    }

    inline void processBandWidth(size_t n, size_t numSamples)
    {
        auto block =
            dsp::AudioBlock<float>(bandBuffer[n]).getSubBlock(0, numSamples);
        // auto block = bandBlock[n].getSubBlock(0, numSamples);

        if (*bandWidth[n] != 1.0 && bandBuffer[n].getNumChannels() > 1) {
            if (lastBandWidth[n] != *bandWidth[n]) {
                bandWidener[n].widenBlockWithRamp(block, lastBandWidth[n],
                                                  *bandWidth[n], *monoWidth);
                lastBandWidth[n] = *bandWidth[n];
            } else
                bandWidener[n].widenBlock(block, *bandWidth[n], *monoWidth);
        }
    }

    template <typename T>
    inline void sumBands(dsp::AudioBlock<T> &block)
    {
        block.fill(0.0);

        if (*soloBand[0] || *soloBand[1] || *soloBand[2] || *soloBand[3]) {
            for (size_t b = 0; b <= numBands; ++b) {
                if (*soloBand[b]) {
                    for (size_t ch = 0; ch < block.getNumChannels(); ++ch) {
                        FloatVectorOperations::add(
                            block.getChannelPointer(ch),
                            bandBuffer[b].getReadPointer(ch),
                            block.getNumSamples());
                    }
                }
            }
        } else {
            for (size_t b = 0; b <= numBands; ++b) {
                for (size_t ch = 0; ch < block.getNumChannels(); ++ch) {
                    FloatVectorOperations::add(block.getChannelPointer(ch),
                                               bandBuffer[b].getReadPointer(ch),
                                               block.getNumSamples());
                }
            }
        }
    }

    template <typename T>
    void processBands(dsp::AudioBlock<T> &block)
    {
        const auto numSamples = block.getNumSamples();

        // copy into multi-buffers
        for (auto &b : bandBuffer) {
            for (int ch = 0; ch < block.getNumChannels(); ++ch)
                b.copyFrom(ch, 0, block.getChannelPointer(ch), numSamples);
        }

        for (size_t n = 0; n <= numBands; ++n) {
            if (n < numBands)
                updateCrossoverNonLin(n);
            auto gain = pow(10.f, *bandInGain[n] * 0.05f);
            auto outGain = pow(10.f, *bandOutGain[n] * 0.05f);

            processFilters(n, numSamples);

            if (*muteBand[n]) {
                bandBuffer[n].clear();
                continue;
            }

            if (!*bypassBand[n]) {
                if (gain != lastInGain[n])
                    bandBuffer[n].applyGainRamp(0, numSamples, lastInGain[n],
                                                gain);
                else
                    bandBuffer[n].applyGain(gain);

                dsp::AudioBlock<T> b_block(bandBuffer[n]);
                b_block = b_block.getSubBlock(0, numSamples);

                mPi[n].process(b_block);

                if (*autoGain) {
                    if (gain != lastInGain[n])
                        bandBuffer[n].applyGainRamp(
                            0, numSamples, 1.0 / lastInGain[n], 1.0 / gain);
                    else
                        bandBuffer[n].applyGain(1.0 / gain);
                }
            }

            lastInGain[n] = gain;

            if (outGain != lastOutGain[n])
                bandBuffer[n].applyGainRamp(0, numSamples, lastOutGain[n],
                                            outGain);
            else
                bandBuffer[n].applyGain(0, numSamples, outGain);

            lastOutGain[n] = outGain;

            processBandWidth(n, numSamples);
        }

        sumBands(block);
    }

    std::array<std::atomic<float> *, TOTAL_BANDS> muteBand, soloBand,
        bypassBand;
    std::array<strix::FloatParameter *, TOTAL_BANDS> bandInGain, bandOutGain,
        bandWidth;
    std::array<float, TOTAL_BANDS> lastInGain, lastOutGain, lastBandWidth;
    std::array<strix::FloatParameter *, TOTAL_XOVER> crossovers;

    std::atomic<bool> updateCrossovers = false;
    int crossover_changed_ID;

    std::array<LRFilter<float>, TOTAL_XOVER> bands;

    // dsp::ConvolutionMessageQueue q{16000};

    static constexpr int FFT_SIZE = 2048;
    std::array<LinearFilter::Processor<FFT_SIZE>, TOTAL_XOVER> linBand{
        {{LinearFilter::Processor<FFT_SIZE>(true)},
         {LinearFilter::Processor<FFT_SIZE>(false)},
         {LinearFilter::Processor<FFT_SIZE>(false)}}};

    std::array<StereoWidener<float>, TOTAL_BANDS> bandWidener;

    // IDEA: Can we set a max size at init, and don't bother resizing on
    // oversample?
    std::array<AudioBuffer<float>, TOTAL_BANDS> bandBuffer;

  private:
    int numBands = 2;

    double lastSampleRate = 44100.0;

    double xm1[2]{0.0, 0.0};
    double ym1[2]{0.0, 0.0};

    std::atomic<float> *linearPhase, *curve, *clip, *distIndex, *monoWidth,
        *autoGain;

    AudioProcessorValueTreeState &apvts;

    MaximPizer mPi[4] = {MaximPizer(apvts), MaximPizer(apvts),
                         MaximPizer(apvts), MaximPizer(apvts)};
};
