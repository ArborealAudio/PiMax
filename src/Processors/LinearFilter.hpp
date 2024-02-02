// LinearFilter.hpp

#pragma once

#include <JuceHeader.h>

#include <stdint.h>

namespace LinearFilter {

struct Buffer
{
    Buffer() = default;

    Buffer(AudioBuffer<float> &&bufferIn) : buffer(std::move(bufferIn)) {}

    AudioBuffer<float> buffer;
};

struct BufferTransfer
{
    void set(Buffer &&p)
    {
        const SpinLock::ScopedLockType lock(mutex);
        buffer = std::move(p);
        newBuffer = true;
    }

    template <typename Fn>
    void get(Fn &&fn)
    {
        const SpinLock::ScopedTryLockType lock(mutex);

        if (lock.isLocked() && newBuffer) {
            fn(buffer);
            newBuffer = false;
        }
    }

  private:
    Buffer buffer;
    bool newBuffer = false;
    SpinLock mutex;
};

template <int size>
struct Processor
{
    Processor(bool twoFilters) : two_filters(twoFilters) {}

    void prepare(const dsp::ProcessSpec &spec)
    {
        sampleRate = spec.sampleRate;
        numSamples = spec.maximumBlockSize;

        auto monoSpec = spec;
        monoSpec.numChannels = 1;

        for (auto &f : iirHigh)
            f.prepare(monoSpec);
        highPulse.prepare(monoSpec);
        convHigh.prepare(spec);

        if (two_filters) {
            for (auto &f : iirLow)
                f.prepare(monoSpec);
            lowPulse.prepare(monoSpec);
            convLow.prepare(spec);
        }
    }

    void reset()
    {
        for (auto &i : iirHigh)
            i.reset();
        highPulse.reset();
        convHigh.reset();
        if (two_filters) {
            for (auto &i : iirLow)
                i.reset();
            lowPulse.reset();
            convLow.reset();
        }
    }

    int getLatency() { return 2 * convHigh.getCurrentIRSize(); }

    void createIIRCoeffs(double freq, double sampleRate)
    {
        if (two_filters) {
            newIIRCoeffLow = *dsp::FilterDesign<float>::
                              designIIRLowpassHighOrderButterworthMethod(
                                  freq, sampleRate, 2)
                                  .getFirst();
            // *iirLow[0].coefficients = coeff;
            // *iirLow[1].coefficients = coeff;
            *lowPulse.coefficients = newIIRCoeffLow;
        }

        newIIRCoeffHigh =
            *dsp::FilterDesign<
                 float>::designIIRHighpassHighOrderButterworthMethod(freq,
                                                                     sampleRate,
                                                                     2)
                 .getFirst();
        // *iirHigh[0].coefficients = coeff;
        // *iirHigh[1].coefficients = coeff;
        *highPulse.coefficients = newIIRCoeffHigh;
    }

    void createImpulseResponse()
    {
        for (size_t i = 0; i < size; ++i) {
            auto input = i == 0 ? 1 : 0;
            linCoeffHigh[i] = highPulse.processSample(input);
        }
        std::reverse(linCoeffHigh.begin(), linCoeffHigh.end());

        AudioBuffer<float> highIR{1, size};
        for (int ch = 0; ch < highIR.getNumChannels(); ++ch) {
            highIR.copyFrom(ch, 0, linCoeffHigh.data(), size);
        }
        highTransfer.set(Buffer{std::move(highIR)});

        if (two_filters) {
            for (size_t i = 0; i < size; ++i) {
                auto input = i == 0 ? 1 : 0;
                linCoeffLow[i] = lowPulse.processSample(input);
            }
            std::reverse(linCoeffLow.begin(), linCoeffLow.end());

            AudioBuffer<float> lowIR{1, size};
            for (int ch = 0; ch < lowIR.getNumChannels(); ++ch) {
                lowIR.copyFrom(ch, 0, linCoeffLow.data(), size);
            }
            lowTransfer.set(Buffer{std::move(lowIR)});
        }
    }

    // This should be called from the main thread to create new coeffs
    // Need to manage notifying audio thread when this begins and ends
    void updateFilters(double freq, double SR)
    {
        createIIRCoeffs(freq, SR);
        createImpulseResponse();
        // TODO: Copy new coeffs to current. Should we lock a mutex?
    }

    void loadBuffersIntoConvolution()
    {
        highTransfer.get([&](Buffer &buf) {
            convHigh.loadImpulseResponse(std::move(buf.buffer), sampleRate,
                                         dsp::Convolution::Stereo::yes,
                                         dsp::Convolution::Trim::no,
                                         dsp::Convolution::Normalise::no);
        });

        if (two_filters) {
            lowTransfer.get([&](Buffer &buf) {
                convLow.loadImpulseResponse(std::move(buf.buffer), sampleRate,
                                            dsp::Convolution::Stereo::yes,
                                            dsp::Convolution::Trim::no,
                                            dsp::Convolution::Normalise::no);
            });
        }
    }

    void convolveLow(dsp::AudioBlock<float> &block)
    {
        for (int ch = 0; ch < block.getNumChannels(); ++ch) {
            auto *in = block.getChannelPointer(ch);
            for (size_t i = 0; i < block.getNumSamples(); ++i) {
                *iirLow[ch].coefficients = newIIRCoeffLow;
                in[i] = iirLow[ch].processSample(in[i]);
            }
        }
        convLow.process(dsp::ProcessContextReplacing<float>(block));
    }

    void convolveHigh(dsp::AudioBlock<float> &block)
    {
        for (int ch = 0; ch < block.getNumChannels(); ++ch) {
            auto *in = block.getChannelPointer(ch);
            for (size_t i = 0; i < block.getNumSamples(); ++i) {
                *iirHigh[ch].coefficients = newIIRCoeffHigh;
                in[i] = iirHigh[ch].processSample(in[i]);
            }
        }
        convHigh.process(dsp::ProcessContextReplacing<float>(block));
    }

    const int mSize = size;

  private:
    const bool two_filters = false;
    std::array<float, size> linCoeffLow, linCoeffHigh, newLinCoeffLow,
        newLinCoeffHigh;
    dsp::IIR::Coefficients<float> iirCoeffLow, iirCoeffHigh, newIIRCoeffLow,
        newIIRCoeffHigh;
    dsp::IIR::Filter<float> iirLow[2], iirHigh[2],
        // filters for generating IR to create linear coeffs
        lowPulse, highPulse;

    dsp::Convolution convHigh, convLow;

    BufferTransfer highTransfer, lowTransfer;

    double sampleRate;
    uint32 numSamples;
};
} // namespace LinearFilter
