#pragma once

namespace strix
{
    struct Buffer
    {
        Buffer() = default;

        Buffer(AudioBuffer<float>&& bufferIn) : buffer(std::move(bufferIn))
        {}

        AudioBuffer<float> buffer;
    };

    struct BufferTransfer
    {
        void set(Buffer&& p)
        {
            const SpinLock::ScopedLockType lock(mutex);
            buffer = std::move(p);
            newBuffer = true;
        }

        template <typename Fn>
        void get(Fn&& fn)
        {
            const SpinLock::ScopedTryLockType lock(mutex);

            if (lock.isLocked() && newBuffer)
            {
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
        FIR(bool twoFilters, size_t firOrder, dsp::ConvolutionMessageQueue& queue) : doubleFilter(twoFilters),
            size(firOrder),
            convLow(dsp::Convolution::Latency{2 * size - 1}, queue),
            convHigh(dsp::Convolution::Latency{2 * size - 1}, queue)
        {
            if (doubleFilter)
                firCoeffLow.resize(firOrder, 0.0f);
            //lastCoeffLow.resize(firOrder, 0.f);
            firCoeffHigh.resize(firOrder, 0.0f);
            //lastCoeffHigh.resize(firOrder, 0.f);
            //lowinc.resize(firOrder);
            //highinc.resize(firOrder);
        }

        void prepare(const dsp::ProcessSpec& spec) noexcept
        {
            sampleRate = spec.sampleRate;
            numSamples = spec.maximumBlockSize;
            //bps = sampleRate / numSamples;
            auto monoSpec = spec;
            monoSpec.numChannels = 1;

            if (doubleFilter) {
                for (auto& i : iirLow)
                    for (auto& j : i)
                        j->prepare(monoSpec);
                for (auto& i : iirLowPulse)
                    i->prepare(monoSpec);
            }
            for (auto& i : iirHigh)
                for (auto& j : i)
                    j->prepare(monoSpec);
            for (auto& i : iirHighPulse)
                i->prepare(monoSpec);

        #if USE_CONVOLUTION
            if (doubleFilter)
                convLow.prepare(spec);
            convHigh.prepare(spec);
        #else

            for (auto& f : firLow)
                f.prepare(spec);
            for (auto& f : lastFIRLow)
                f.prepare(spec);
            for (auto& f : firHigh)
                f.prepare(spec);
            for (auto& f : lastFIRHigh)
                f.prepare(spec);
        #endif
        
        }

        void reset() noexcept
        {
            if (doubleFilter) {
                for (auto& i : iirLow)
                    for (auto& j : i)
                        j->reset();
                for (auto& i : iirLowPulse)
                    i->reset();
            }
            for (auto& i : iirHigh)
                for (auto& j : i)
                    j->reset();
            for (auto& i : iirHighPulse)
                i->reset();
        #if USE_CONVOLUTION
            if (doubleFilter)
                convLow.reset();
            convHigh.reset();
        #else
            for (auto& f : firLow)
                f.reset();
            for (auto& f : lastFIRLow)
                f.reset();
            for (auto& f : firHigh)
                f.reset();
            for (auto& f : lastFIRHigh)
                f.reset();
        #endif
        }

        int getLatency() { return convHigh.getLatency(); }

        void setOversampleFactor(int newOSFactor)
        {
            oversampleFactor = newOSFactor;

            /*if (oversampleFactor > 1) {
                size = doubleSize;
                firCoeffLow.resize(size);
                firCoeffHigh.resize(size);
            }
            else {
                size = 512;
                firCoeffLow.resize(size);
                firCoeffHigh.resize(size);
            }*/
        }

        inline void createIIRCoeffs(double freq, double sampleRate) noexcept
        {
            if (doubleFilter) {
                iirLow[0].clear();
                iirLow[1].clear();
                iirLowPulse.clear();

                iirLowCoeffs = dsp::FilterDesign<float>::
                    designIIRLowpassHighOrderButterworthMethod(freq, sampleRate, 2);
                for (auto& c : iirLowCoeffs) {
                    iirLow[0].add(new dsp::IIR::Filter<float>(c));
                    iirLow[1].add(new dsp::IIR::Filter<float>(c));
                    iirLowPulse.add(new dsp::IIR::Filter<float>(c));
                }
            }

            iirHigh[0].clear();
            iirHigh[1].clear();
            iirHighPulse.clear();

            iirHighCoeffs = dsp::FilterDesign<float>::
                designIIRHighpassHighOrderButterworthMethod(freq, sampleRate, 2);
            for (auto& c : iirHighCoeffs) {
                iirHigh[0].add(new dsp::IIR::Filter<float>(c));
                iirHigh[1].add(new dsp::IIR::Filter<float>(c));
                iirHighPulse.add(new dsp::IIR::Filter<float>(c));
            }

        }

        inline void changeIIRCoeffs(double freq, double sampleRate) noexcept
        {
            if (doubleFilter)
                for (auto& iir : iirLow)
                    iir[0]->coefficients = *dsp::FilterDesign<float>::
                        designIIRLowpassHighOrderButterworthMethod(freq, sampleRate, 2).getFirst();

            for (auto& iir : iirHigh)
                iir[0]->coefficients = *dsp::FilterDesign<float>::
                designIIRHighpassHighOrderButterworthMethod(freq, sampleRate, 2).getFirst();

            if (doubleFilter)
                iirLowPulse[0]->coefficients = *dsp::FilterDesign<float>::
                    designIIRLowpassHighOrderButterworthMethod(freq, sampleRate, 2).getFirst();

            iirHighPulse[0]->coefficients = *dsp::FilterDesign<float>::
                designIIRHighpassHighOrderButterworthMethod(freq, sampleRate, 2).getFirst();
        }

        inline void recordImpulseResponse() noexcept
        {
            if (doubleFilter) {
                for (int i = 0; i < size; i++)
                {
                    auto input = i == 0 ? 1.0f : 0.0f;
                    for (auto& iir : iirLowPulse)
                        input = iir->processSample(input);

                    firCoeffLow[i] = input;
                }
            }

            for (int i = 0; i < size; i++)
            {
                auto input = i == 0 ? 1.0f : 0.0f;
                for (auto& iir : iirHighPulse)
                    input = iir->processSample(input);

                firCoeffHigh[i] = input;
            }

            if (doubleFilter)
                std::reverse(firCoeffLow.begin(), firCoeffLow.end());

            std::reverse(firCoeffHigh.begin(), firCoeffHigh.end());

        #if USE_CONVOLUTION
            AudioBuffer<float> highImpulse{ 2, size };

            auto highbuf = highImpulse.getArrayOfWritePointers();
            for (int ch = 0; ch < highImpulse.getNumChannels(); ++ch)
            {
                for (int i = 0; i < size; ++i) {
                    highbuf[ch][i] = firCoeffHigh[i];
                }
            }

            highTransfer.set(Buffer{ std::move(highImpulse) });

            //lastCoeffHigh = firCoeffHigh;

            if (doubleFilter) {
                AudioBuffer<float> lowImpulse{ 2, size };
                auto lowbuf = lowImpulse.getArrayOfWritePointers();
                for (int ch = 0; ch < lowImpulse.getNumChannels(); ++ch)
                {
                    for (int i = 0; i < size; ++i) {
                        lowbuf[ch][i] = firCoeffLow[i];
                    }
                }
                lowTransfer.set(Buffer{ std::move(lowImpulse) });
                //lastCoeffLow = firCoeffLow;
            }

        #endif
        }

        inline void recordNewImpulseResponse() noexcept
        {
            lastCoeffLow = firCoeffLow;
            lastCoeffHigh = firCoeffHigh;
            for (int i = 0; i < size; i++)
            {
                auto input = i == 0 ? 1.0f : 0.0f;
                for (auto& iir : iirLowPulse)
                    input = iir->processSample(input);

                firCoeffLow[i] = input;
            }

            for (int i = 0; i < size; i++)
            {
                auto input = i == 0 ? 1.0f : 0.0f;
                for (auto& iir : iirHighPulse)
                    input = iir->processSample(input);

                firCoeffHigh[i] = input;
            }

            std::reverse(firCoeffLow.begin(), firCoeffLow.end());
            std::reverse(firCoeffHigh.begin(), firCoeffHigh.end());

            for (size_t i = 0; i < size; ++i)
            {
                lowinc[i] = (firCoeffLow[i] - lastCoeffLow[i]) / 10.0;
                highinc[i] = (firCoeffHigh[i] - lastCoeffHigh[i]) / 10.0;
            }
        }

        void interpolateImpulseResponse() noexcept
        {
#if USE_CONVOLUTION

            AudioBuffer<float> lowImpulse{ 2, size };
            AudioBuffer<float> highImpulse{ 2, size };
            auto lowbuf = lowImpulse.getArrayOfWritePointers();
            auto highbuf = highImpulse.getArrayOfWritePointers();

            for (size_t i = 0; i < size; ++i) {
                for (int ch = 0; ch < lowImpulse.getNumChannels(); ++ch)
                {
                    lowbuf[ch][i] = lastCoeffLow[i];
                    highbuf[ch][i] = lastCoeffHigh[i];
                }
                lastCoeffLow[i] += lowinc[i];
                lastCoeffHigh[i] += highinc[i];
            }

            lowTransfer.set(Buffer{ std::move(lowImpulse) });
            highTransfer.set(Buffer{ std::move(highImpulse) });

#endif
        }

        void loadBuffers() noexcept
        {
            if (doubleFilter) {
                lowTransfer.get([this](Buffer& buf)
                    {
                        convLow.loadImpulseResponse(std::move(buf.buffer), sampleRate, dsp::Convolution::Stereo::yes,
                            dsp::Convolution::Trim::no, dsp::Convolution::Normalise::no);
                    });
            }

            highTransfer.get([this](Buffer& buf)
            {
                convHigh.loadImpulseResponse(std::move(buf.buffer), sampleRate, dsp::Convolution::Stereo::yes,
                    dsp::Convolution::Trim::no, dsp::Convolution::Normalise::no);
            });

        }

        inline void initFilter(double freq, double sampleRate) noexcept
        {
            createIIRCoeffs(freq, sampleRate);

            recordImpulseResponse();
        }

        inline void updateFilter(double freq, double sampleRate) noexcept
        {
            changeIIRCoeffs(freq, sampleRate);

            recordImpulseResponse();
        }

        template <typename T>
        inline void convolveLow(const dsp::ProcessContextReplacing<T>& context) noexcept
        {
            const auto& input = context.getInputBlock();
            auto& output = context.getOutputBlock();

            for (int ch = 0; ch < input.getNumChannels(); ++ch)
            {
                auto* in = input.getChannelPointer(ch);
                for (int i = 0; i < input.getNumSamples(); ++i) {
                    float out = 0.0;
                    for (auto& iir : iirLow[ch])
                        out = iir->processSample(in[i]);
                    output.setSample(ch, i, out);
                }
            }
            
            convLow.process(context);
        }

        template <typename T>
        inline void convolveHigh(const dsp::ProcessContextReplacing<T>& context) noexcept
        {
            const auto& input = context.getInputBlock();
            auto& output = context.getOutputBlock();

            for (int ch = 0; ch < input.getNumChannels(); ++ch)
            {
                auto* in = input.getChannelPointer(ch);
                for (int i = 0; i < input.getNumSamples(); ++i) {
                    float out = 0.0;
                    for (auto& iir : iirHigh[ch])
                        out = iir->processSample(in[i]);
                    output.setSample(ch, i, out);
                }
            }

            convHigh.process(context);
        }

        int size = 0;
        double sampleRate = 44100.0;
        int numSamples = 0;
        int bps = 0;

        std::vector<double> firCoeffLow;
        std::vector<double> lastCoeffLow;
        std::vector<double> firCoeffHigh;
        std::vector<double> lastCoeffHigh;

    private:
        using vec = dsp::SIMDRegister<float>;

        int oversampleFactor = 0;
        int set = 0;

        bool doubleFilter = false;

        std::vector<float> lowinc;
        std::vector<float> highinc;

#if USE_SIMD_SAT
        OwnedArray<dsp::IIR::Filter<vec>> iirLow[2];
        OwnedArray<dsp::IIR::Filter<float>> iirLowPulse;
        OwnedArray<dsp::IIR::Filter<vec>> iirHigh[2];
        OwnedArray<dsp::IIR::Filter<float>> iirHighPulse;
#else
        OwnedArray<dsp::IIR::Filter<float>> iirLow[2];
        OwnedArray<dsp::IIR::Filter<float>> iirHigh[2];
        OwnedArray<dsp::IIR::Filter<float>> iirLowPulse;
        OwnedArray<dsp::IIR::Filter<float>> iirHighPulse;
#endif
        
        ReferenceCountedArray<dsp::IIR::Coefficients<float>> iirLowCoeffs;
        ReferenceCountedArray<dsp::IIR::Coefficients<float>> iirHighCoeffs;


        dsp::Convolution convLow, convHigh;

        BufferTransfer lowTransfer;
        BufferTransfer highTransfer;
    };

    /*not actually a thread lmao*/
    struct FIRThread : FIR
    {
        FIRThread(bool twoFilters, size_t order, dsp::ConvolutionMessageQueue& q) : FIR(twoFilters, order, q)
        {}

        ~FIRThread() { }

        void prepare(const dsp::ProcessSpec& spec)
        {
            FIR::prepare(spec);
        }

        /*need this to init filters on the main thread or whatever to avoid trying to access null memory spaces*/
        void initFilters(double newFreq, double newSampleRate, int newIIROrder) noexcept
        {
            freq = newFreq;
            sampleRate = newSampleRate;
            iirOrder = newIIROrder;

            initFilter(freq, sampleRate);
#if! USE_CONVOLUTION
            setFIRCoeffs();
#endif
        }

        void setParams(double newFreq, double newSampleRate, int newIIROrder) noexcept
        {
            /*copy coeffs to "state" filters which can be our backup if thread does not
            finish in time...
            but also this will copy the coeffs from the updated filters once this is called again in
            the loop. So we actually need to make sure that we're ready to receive the new coefficients
            in our state filter*/
            //if (!isThreadRunning()) /*check if Background Thread is running. If not, we can perform the copy*/
            //    copyLastCoeffs(); 
            //else if (waitForThreadToExit(-1))
            //    copyLastCoeffs();

            freq = newFreq;
            sampleRate = newSampleRate;
            iirOrder = newIIROrder;

            updateFilter(freq, sampleRate);
        }

    private:

        double freq = 0.0, sampleRate = 0.0;
        int iirOrder = 0;
        int interpolationLength = 0;
    };

} //namespace strix
