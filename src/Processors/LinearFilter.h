#pragma once
#include <JuceHeader.h>

namespace strix {
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

    template <typename Fn> void get(Fn &&fn)
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

class FIR
{
  public:
    FIR(bool twoFilters, size_t firOrder, dsp::ConvolutionMessageQueue &queue)
        : doubleFilter(twoFilters), size(firOrder), convLow(queue),
          convHigh(queue)
    {
        if (doubleFilter) {
            linCoeffLow.resize(size, 0);
        }
        linCoeffHigh.resize(size, 0);
    }

    void prepare(const dsp::ProcessSpec &spec) noexcept
    {
        sampleRate = spec.sampleRate;
        numSamples = spec.maximumBlockSize;
        auto monoSpec = spec;
        monoSpec.numChannels = 1;

        if (doubleFilter) {
            for (auto &i : iirLow)
                i.prepare(monoSpec);

            iirLowPulse.prepare(monoSpec);
        }
        for (auto &i : iirHigh)
            i.prepare(monoSpec);

        iirHighPulse.prepare(monoSpec);

        if (doubleFilter)
            convLow.prepare(spec);
        convHigh.prepare(spec);
    }

    void reset() noexcept
    {
        if (doubleFilter) {
            for (auto &i : iirLow)
                i.reset();
            iirLowPulse.reset();
        }
        for (auto &i : iirHigh)
            i.reset();
        iirHighPulse.reset();
        if (doubleFilter)
            convLow.reset();
        convHigh.reset();
    }

    int getLatency() { return 2 * convHigh.getCurrentIRSize(); }

    void setOversampleFactor(int newOSFactor)
    {
        oversampleFactor = newOSFactor;
    }

    inline void createIIRCoeffs(double freq, double sampleRate) noexcept
    {
        if (doubleFilter) {

            // farbot::RealtimeObject<
            //     ReferenceCountedObjectPtr<dsp::IIR::Coefficients<float>>,
            //     farbot::RealtimeObjectOptions::nonRealtimeMutatable>::
            //     ScopedAccess<farbot::ThreadType::nonRealtime>
            //         lowCoeffs(iirLowCoeffs);

            iirLowCoeffs = *dsp::FilterDesign<float>::
                                designIIRLowpassHighOrderButterworthMethod(
                                    freq, sampleRate, 2)
                                    .getFirst();
            /*it'll always be one set of coeffs*/

            iirLow[0].coefficients = iirLowCoeffs;
            iirLow[1].coefficients = iirLowCoeffs;
            iirLowPulse.coefficients = iirLowCoeffs;
        }

        // iirHigh[0].clear();
        // iirHigh[1].clear();

        // farbot::RealtimeObject<
        //     ReferenceCountedObjectPtr<dsp::IIR::Coefficients<float>>,
        //     farbot::RealtimeObjectOptions::nonRealtimeMutatable>::
        //     ScopedAccess<farbot::ThreadType::nonRealtime>
        //         hiCoeffs(iirHighCoeffs);

        iirHighCoeffs =
            *dsp::FilterDesign<
                 float>::designIIRHighpassHighOrderButterworthMethod(freq,
                                                                     sampleRate,
                                                                     2)
                 .getFirst();

        iirHigh[0].coefficients = iirHighCoeffs;
        iirHigh[1].coefficients = iirHighCoeffs;
        iirHighPulse.coefficients = iirHighCoeffs;
    }

    inline void changeIIRCoeffs(double freq, double sampleRate) noexcept
    {
        if (doubleFilter) {
            // farbot::RealtimeObject<
            //     ReferenceCountedObjectPtr<dsp::IIR::Coefficients<float>>,
            //     farbot::RealtimeObjectOptions::nonRealtimeMutatable>::
            //     ScopedAccess<farbot::ThreadType::nonRealtime>
            //         lowCoeffs(iirLowCoeffs);

            iirLowCoeffs = *dsp::FilterDesign<float>::
                                designIIRLowpassHighOrderButterworthMethod(
                                    freq, sampleRate, 2)
                                    .getFirst();
        }

        // farbot::RealtimeObject<
        //     ReferenceCountedObjectPtr<dsp::IIR::Coefficients<float>>,
        //     farbot::RealtimeObjectOptions::nonRealtimeMutatable>::
        //     ScopedAccess<farbot::ThreadType::nonRealtime>
        //         hiCoeffs(iirHighCoeffs);

        iirHighCoeffs =
            *dsp::FilterDesign<
                 float>::designIIRHighpassHighOrderButterworthMethod(freq,
                                                                     sampleRate,
                                                                     2)
                 .getFirst();
    }

    inline void recordImpulseResponse() noexcept
    {
        // farbot::RealtimeObject<
        //     ReferenceCountedObjectPtr<dsp::IIR::Coefficients<float>>,
        //     farbot::RealtimeObjectOptions::nonRealtimeMutatable>::
        //     ScopedAccess<farbot::ThreadType::nonRealtime>
        //         irCoeffsLow(iirLowCoeffs);

        // farbot::RealtimeObject<
        //     ReferenceCountedObjectPtr<dsp::IIR::Coefficients<float>>,
        //     farbot::RealtimeObjectOptions::nonRealtimeMutatable>::
        //     ScopedAccess<farbot::ThreadType::nonRealtime>
        //         irCoeffsHigh(iirHighCoeffs);

        if (doubleFilter) {
            for (int i = 0; i < size; i++) {
                auto input = i == 0 ? 1.0f : 0.0f;
                // iir.coefficients = iirLowCoeffs;
                linCoeffLow[i] = iirLowPulse.processSample(input);
            }
            std::reverse(linCoeffLow.begin(), linCoeffLow.end());
        }

        for (int i = 0; i < size; i++) {
            auto input = i == 0 ? 1.0f : 0.0f;
            // iir.coefficients = iirHighCoeffs;
            linCoeffHigh[i] = iirHighPulse.processSample(input);
            std::reverse(linCoeffHigh.begin(), linCoeffHigh.end());
        }

        AudioBuffer<float> highImpulse{2, size};
        for (int ch = 0; ch < highImpulse.getNumChannels(); ++ch) {
            highImpulse.copyFrom(ch, 0, linCoeffHigh.data(), size);           
        }
        highTransfer.set(Buffer{std::move(highImpulse)});

        if (doubleFilter) {
            AudioBuffer<float> lowImpulse{2, size};
            for (int ch = 0; ch < lowImpulse.getNumChannels(); ++ch) {
                lowImpulse.copyFrom(ch, 0, linCoeffLow.data(), size);           
            }
            lowTransfer.set(Buffer{std::move(lowImpulse)});
        }
    }

    void loadBuffers() noexcept
    {
        if (doubleFilter) {
            lowTransfer.get([this](Buffer &buf) {
                convLow.loadImpulseResponse(std::move(buf.buffer), sampleRate,
                                            dsp::Convolution::Stereo::yes,
                                            dsp::Convolution::Trim::no,
                                            dsp::Convolution::Normalise::no);
            });
        }

        highTransfer.get([this](Buffer &buf) {
            convHigh.loadImpulseResponse(std::move(buf.buffer), sampleRate,
                                         dsp::Convolution::Stereo::yes,
                                         dsp::Convolution::Trim::no,
                                         dsp::Convolution::Normalise::no);
        });
    }

    void initFilter(double freq, double _sampleRate)
    {
        createIIRCoeffs(freq, _sampleRate);

        recordImpulseResponse();
    }

    void updateFilter(double freq, double _sampleRate)
    {
        // changeIIRCoeffs(freq, _sampleRate);
        createIIRCoeffs(freq, _sampleRate);

        recordImpulseResponse();
    }

    template <typename T>
    inline void
    convolveLow(const dsp::ProcessContextReplacing<T> &context) noexcept
    {
        const auto &input = context.getInputBlock();
        auto &output = context.getOutputBlock();

        // farbot::RealtimeObject<
        //     ReferenceCountedObjectPtr<dsp::IIR::Coefficients<float>>,
        //     farbot::RealtimeObjectOptions::nonRealtimeMutatable>::
        //     ScopedAccess<farbot::ThreadType::realtime>
        //         rtLowCoeffs(iirLowCoeffs);

        for (int ch = 0; ch < input.getNumChannels(); ++ch) {
            auto *in = output.getChannelPointer(ch);
            for (int i = 0; i < input.getNumSamples(); ++i) {
                in[i] = iirLow[ch].processSample(in[i]);
            }
        }

        convLow.process(context);
    }

    template <typename T>
    inline void
    convolveHigh(const dsp::ProcessContextReplacing<T> &context) noexcept
    {
        const auto &input = context.getInputBlock();
        auto &output = context.getOutputBlock();

        // farbot::RealtimeObject<
        //     ReferenceCountedObjectPtr<dsp::IIR::Coefficients<float>>,
        //     farbot::RealtimeObjectOptions::nonRealtimeMutatable>::
        //     ScopedAccess<farbot::ThreadType::realtime>
        //         rtHighCoeffs(iirHighCoeffs);

        for (int ch = 0; ch < input.getNumChannels(); ++ch) {
            auto *in = output.getChannelPointer(ch);
            for (int i = 0; i < input.getNumSamples(); ++i) {
                in[i] = iirHigh[ch].processSample(in[i]);
            }
        }

        convHigh.process(context);
    }

    int size = 0;
    double sampleRate = 44100.0;
    int numSamples = 0;

    std::vector<float> linCoeffLow;
    std::vector<float> linCoeffHigh;

  private:
    int oversampleFactor = 0;

    const bool doubleFilter = false;

    std::vector<float> lowinc;
    std::vector<float> highinc;

    dsp::IIR::Filter<float> iirLow[2];
    dsp::IIR::Filter<float> iirHigh[2];
    dsp::IIR::Filter<float> iirLowPulse;
    dsp::IIR::Filter<float> iirHighPulse;

    dsp::IIR::Coefficients<float> iirLowCoeffs;
    dsp::IIR::Coefficients<float> iirHighCoeffs;

    dsp::Convolution convLow, convHigh;

    BufferTransfer lowTransfer;
    BufferTransfer highTransfer;
};

/*not actually a thread (or FIR) lmao*/
struct FIRThread : FIR
{
    FIRThread(bool twoFilters, size_t order, dsp::ConvolutionMessageQueue &q)
        : FIR(twoFilters, order, q)
    {
    }

    void prepare(const dsp::ProcessSpec &spec) { FIR::prepare(spec); }

    /*creates new IIRs & IR*/
    void initFilters(double newFreq, double newSampleRate,
                     int newIIROrder) noexcept
    {
        freq = newFreq;
        sampleRate = newSampleRate;
        iirOrder = newIIROrder;

        initFilter(freq, sampleRate);
    }

    /*only changes coeffs of existing IIRs & new IR*/
    void setParams(double newFreq, double newSampleRate,
                   int newIIROrder) noexcept
    {
        freq = newFreq;
        sampleRate = newSampleRate;
        iirOrder = newIIROrder;

        updateFilter(freq, sampleRate);
    }

  private:
    double freq = 0.0, sampleRate = 44100.0;
    int iirOrder = 0;
};

} // namespace LinearFilter
} // namespace strix
