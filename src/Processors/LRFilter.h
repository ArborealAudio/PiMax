#pragma once

#include <JuceHeader.h>

/*custom Linkwitz-Riley filter object which contains two independent filter
objects for the purpose of properly cascading band-passed outputs*/
template <typename T> class LRFilter
{
  public:
    void prepare(const dsp::ProcessSpec &spec)
    {
        for (auto &band : bands) {
            band.prepare(spec);
        }
        bands[0].setType(dsp::LinkwitzRileyFilterType::highpass);
        bands[1].setType(dsp::LinkwitzRileyFilterType::lowpass);

        crossover.reset(spec.maximumBlockSize);
    }

    void updateFilter(float crossoverFreq)
    {
        if (crossover.isSmoothing())
            crossover.setTargetValue(crossoverFreq);
        else
            for (auto &band : bands)
                band.setCutoffFrequency(crossoverFreq);
    }

    double getCutoffFreq() { return bands[0].getCutoffFrequency(); }

    void reset()
    {
        for (auto &band : bands)
            band.reset();
    }

    /**
     * process one channel and return the high output
     * @tparam twoOutputs whether to generate two outputs from one input, or to
     * process each input in place */
    template <bool twoOutputs>
    void process(dsp::AudioBlock<T> &low, dsp::AudioBlock<T> &hi)
    {
        if (crossover.isSmoothing()) {
            for (size_t i = 0; i < low.getNumSamples(); ++i) {
                for (auto &band : bands)
                    band.setCutoffFrequency(crossover.getNextValue());
                if (twoOutputs) {
                    for (size_t ch = 0; ch < low.getNumChannels(); ++ch) {
                        auto &inL = low.getChannelPointer(ch)[i];
                        bands[0].processSample(ch, inL, inL,
                                               hi.getChannelPointer(ch)[i]);
                    }
                } else {
                    for (size_t ch = 0; ch < low.getNumChannels(); ++ch) {
                        auto &inL = low.getChannelPointer(ch)[i];
                        auto &inH = hi.getChannelPointer(ch)[i];
                        inH = bands[0].processSample(ch, inH);
                        inL = bands[1].processSample(ch, inL);
                    }
                }
            }
            return;
        }

        for (size_t ch = 0; ch < low.getNumChannels(); ++ch) {
            if (twoOutputs) {
                auto inL = low.getChannelPointer(ch);
                auto outH = hi.getChannelPointer(ch);
                for (size_t i = 0; i < low.getNumSamples(); ++i)
                    bands[0].processSample(ch, inL[i], inL[i], outH[i]);
                return;
            }

            auto inL = low.getChannelPointer(ch);
            auto inH = hi.getChannelPointer(ch);
            for (size_t i = 0; i < low.getNumSamples(); ++i) {
                inH[i] = bands[0].processSample(ch, inH[i]);
                inL[i] = bands[1].processSample(ch, inL[i]);
            }
        }
    }

    /* process one channel and return low and high outputs */
    void process(const T *input, T *outputLow, T *outputHigh, size_t numSamples,
                 int ch)
    {
        if (crossover.isSmoothing()) {
            for (size_t i = 0; i < numSamples; ++i) {
                for (auto &band : bands)
                    band.setCutoffFrequency(crossover.getNextValue());
                bands[0].processSample(ch, input[i], outputLow[i],
                                       outputHigh[i]);
            }
            return;
        }
        for (size_t i = 0; i < numSamples; ++i) {
            bands[0].processSample(ch, input[i], outputLow[i], outputHigh[i]);
        }
    }

    /* process low and high channels independently */
    void process(T *low, T *high, size_t numSamples, int ch)
    {
        if (crossover.isSmoothing()) {
            for (size_t i = 0; i < numSamples; ++i) {
                for (auto &band : bands)
                    band.setCutoffFrequency(crossover.getNextValue());
                high[i] = bands[0].processSample(ch, high[i]);
                low[i] = bands[1].processSample(ch, low[i]);
            }
            return;
        }
        for (size_t i = 0; i < numSamples; ++i) {
            high[i] = bands[0].processSample(ch, high[i]);
            low[i] = bands[1].processSample(ch, low[i]);
        }
    }

  private:
    std::array<dsp::LinkwitzRileyFilter<T>, 2> bands;

    SmoothedValue<T> crossover{40.f};

    double lastSampleRate = 0.0;
};
