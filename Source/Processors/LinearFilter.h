#pragma once

namespace strix
{
    namespace LinearFilter
    {
        struct Buffer
        {
            Buffer() = default;

            Buffer(AudioBuffer<float> &&bufferIn) : buffer(std::move(bufferIn))
            {
            }

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
            FIR(bool twoFilters, size_t firOrder, dsp::ConvolutionMessageQueue &queue) : doubleFilter(twoFilters),
                                                                                         size(firOrder),
                                                                                         convLow(queue),
                                                                                         convHigh(queue)
            {
                if (doubleFilter)
                    firCoeffLow.resize(firOrder, 0.0f);
                firCoeffHigh.resize(firOrder, 0.0f);
            }

            void prepare(const dsp::ProcessSpec &spec) noexcept
            {
                sampleRate = spec.sampleRate;
                numSamples = spec.maximumBlockSize;
                auto monoSpec = spec;
                monoSpec.numChannels = 1;

                if (doubleFilter)
                {
                    for (auto &i : iirLow)
                        for (auto &j : i)
                            j.prepare(monoSpec);
                    for (auto &i : iirLowPulse)
                        i.prepare(monoSpec);
                }
                for (auto &i : iirHigh)
                    for (auto &j : i)
                        j.prepare(monoSpec);
                for (auto &i : iirHighPulse)
                    i.prepare(monoSpec);

#if USE_CONVOLUTION
                if (doubleFilter)
                    convLow.prepare(spec);
                convHigh.prepare(spec);
#else

                for (auto &f : firLow)
                    f.prepare(spec);
                for (auto &f : lastFIRLow)
                    f.prepare(spec);
                for (auto &f : firHigh)
                    f.prepare(spec);
                for (auto &f : lastFIRHigh)
                    f.prepare(spec);
#endif
            }

            void reset() noexcept
            {
                if (doubleFilter)
                {
                    for (auto &i : iirLow)
                        for (auto &j : i)
                            j.reset();
                    for (auto &i : iirLowPulse)
                        i.reset();
                }
                for (auto &i : iirHigh)
                    for (auto &j : i)
                        j.reset();
                for (auto &i : iirHighPulse)
                    i.reset();
#if USE_CONVOLUTION
                if (doubleFilter)
                    convLow.reset();
                convHigh.reset();
#else
                for (auto &f : firLow)
                    f.reset();
                for (auto &f : lastFIRLow)
                    f.reset();
                for (auto &f : firHigh)
                    f.reset();
                for (auto &f : lastFIRHigh)
                    f.reset();
#endif
            }

            int getLatency()
            {
                return 2 * convHigh.getCurrentIRSize();
            }

            void setOversampleFactor(int newOSFactor)
            {
                oversampleFactor = newOSFactor;
            }

            inline void createIIRCoeffs(double freq, double sampleRate) noexcept
            {
                if (doubleFilter)
                {
                    iirLow[0].clear();
                    iirLow[1].clear();
                    iirLowPulse.clear();

                    farbot::RealtimeObject<ReferenceCountedObjectPtr<dsp::IIR::Coefficients<float>>,
                                           farbot::RealtimeObjectOptions::nonRealtimeMutatable>::ScopedAccess<farbot::ThreadType::nonRealtime>
                        lowCoeffs(iirLowCoeffs);

                    *lowCoeffs = dsp::FilterDesign<float>::
                                     designIIRLowpassHighOrderButterworthMethod(freq, sampleRate, 2)
                                         .getFirst();
                    /*it'll always be one set of coeffs*/

                    iirLow[0].emplace_back(dsp::IIR::Filter<float>(*lowCoeffs));
                    iirLow[1].emplace_back(dsp::IIR::Filter<float>(*lowCoeffs));
                    iirLowPulse.emplace_back(dsp::IIR::Filter<float>(*lowCoeffs));
                }

                iirHigh[0].clear();
                iirHigh[1].clear();
                iirHighPulse.clear();

                farbot::RealtimeObject<ReferenceCountedObjectPtr<dsp::IIR::Coefficients<float>>,
                                       farbot::RealtimeObjectOptions::nonRealtimeMutatable>::ScopedAccess<farbot::ThreadType::nonRealtime>
                    hiCoeffs(iirHighCoeffs);

                *hiCoeffs = dsp::FilterDesign<float>::
                                designIIRHighpassHighOrderButterworthMethod(freq, sampleRate, 2)
                                    .getFirst();

                iirHigh[0].emplace_back(dsp::IIR::Filter<float>(*hiCoeffs));
                iirHigh[1].emplace_back(dsp::IIR::Filter<float>(*hiCoeffs));
                iirHighPulse.emplace_back(dsp::IIR::Filter<float>(*hiCoeffs));
            }

            inline void changeIIRCoeffs(double freq, double sampleRate) noexcept
            {
                if (doubleFilter)
                {
                    farbot::RealtimeObject<ReferenceCountedObjectPtr<dsp::IIR::Coefficients<float>>,
                                           farbot::RealtimeObjectOptions::nonRealtimeMutatable>::ScopedAccess<farbot::ThreadType::nonRealtime>
                        lowCoeffs(iirLowCoeffs);

                    *lowCoeffs = dsp::FilterDesign<float>::
                                     designIIRLowpassHighOrderButterworthMethod(freq, sampleRate, 2)
                                         .getFirst();
                }

                farbot::RealtimeObject<ReferenceCountedObjectPtr<dsp::IIR::Coefficients<float>>,
                                       farbot::RealtimeObjectOptions::nonRealtimeMutatable>::ScopedAccess<farbot::ThreadType::nonRealtime>
                    hiCoeffs(iirHighCoeffs);

                *hiCoeffs = dsp::FilterDesign<float>::
                                designIIRHighpassHighOrderButterworthMethod(freq, sampleRate, 2)
                                    .getFirst();
            }

            inline void recordImpulseResponse() noexcept
            {
                farbot::RealtimeObject<ReferenceCountedObjectPtr<dsp::IIR::Coefficients<float>>,
                                       farbot::RealtimeObjectOptions::nonRealtimeMutatable>::ScopedAccess<farbot::ThreadType::nonRealtime>
                    irCoeffsLow(iirLowCoeffs);

                farbot::RealtimeObject<ReferenceCountedObjectPtr<dsp::IIR::Coefficients<float>>,
                                       farbot::RealtimeObjectOptions::nonRealtimeMutatable>::ScopedAccess<farbot::ThreadType::nonRealtime>
                    irCoeffsHigh(iirHighCoeffs);

                if (doubleFilter)
                {
                    for (int i = 0; i < size; i++)
                    {
                        auto input = i == 0 ? 1.0f : 0.0f;
                        for (auto &iir : iirLowPulse)
                        {
                            iir.coefficients = *irCoeffsLow;
                            input = iir.processSample(input);
                        }

                        firCoeffLow[i] = input;
                    }
                }

                for (int i = 0; i < size; i++)
                {
                    auto input = i == 0 ? 1.0f : 0.0f;
                    for (auto &iir : iirHighPulse)
                    {
                        iir.coefficients = *irCoeffsHigh;
                        input = iir.processSample(input);
                    }

                    firCoeffHigh[i] = input;
                }

                if (doubleFilter)
                    std::reverse(firCoeffLow.begin(), firCoeffLow.end());

                std::reverse(firCoeffHigh.begin(), firCoeffHigh.end());

#if USE_CONVOLUTION
                AudioBuffer<float> highImpulse{2, size};

                auto highbuf = highImpulse.getArrayOfWritePointers();
                for (int ch = 0; ch < highImpulse.getNumChannels(); ++ch)
                {
                    for (int i = 0; i < size; ++i)
                    {
                        highbuf[ch][i] = firCoeffHigh[i];
                    }
                }

                highTransfer.set(Buffer{std::move(highImpulse)});

                if (doubleFilter)
                {
                    AudioBuffer<float> lowImpulse{2, size};
                    auto lowbuf = lowImpulse.getArrayOfWritePointers();
                    for (int ch = 0; ch < lowImpulse.getNumChannels(); ++ch)
                    {
                        for (int i = 0; i < size; ++i)
                        {
                            lowbuf[ch][i] = firCoeffLow[i];
                        }
                    }
                    lowTransfer.set(Buffer{std::move(lowImpulse)});
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
                    for (auto &iir : iirLowPulse)
                        input = iir.processSample(input);

                    firCoeffLow[i] = input;
                }

                for (int i = 0; i < size; i++)
                {
                    auto input = i == 0 ? 1.0f : 0.0f;
                    for (auto &iir : iirHighPulse)
                        input = iir.processSample(input);

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

            void loadBuffers() noexcept
            {
                if (doubleFilter)
                {
                    lowTransfer.get([this](Buffer &buf)
                                    { convLow.loadImpulseResponse(std::move(buf.buffer), sampleRate, dsp::Convolution::Stereo::yes,
                                                                  dsp::Convolution::Trim::no, dsp::Convolution::Normalise::no); });
                }

                highTransfer.get([this](Buffer &buf)
                                 { convHigh.loadImpulseResponse(std::move(buf.buffer), sampleRate, dsp::Convolution::Stereo::yes,
                                                                dsp::Convolution::Trim::no, dsp::Convolution::Normalise::no); });
            }

            void initFilter(double freq, double _sampleRate)
            {
                createIIRCoeffs(freq, _sampleRate);

                recordImpulseResponse();
            }

            void updateFilter(double freq, double _sampleRate)
            {
                changeIIRCoeffs(freq, _sampleRate);

                recordImpulseResponse();
            }

            template <typename T>
            inline void convolveLow(const dsp::ProcessContextReplacing<T> &context) noexcept
            {
                const auto &input = context.getInputBlock();
                auto &output = context.getOutputBlock();

                farbot::RealtimeObject<ReferenceCountedObjectPtr<dsp::IIR::Coefficients<float>>, farbot::RealtimeObjectOptions::nonRealtimeMutatable>::ScopedAccess<farbot::ThreadType::realtime> rtLowCoeffs(iirLowCoeffs);

                for (int ch = 0; ch < input.getNumChannels(); ++ch)
                {
                    auto *in = output.getChannelPointer(ch);
                    for (int i = 0; i < input.getNumSamples(); ++i)
                    {
                        for (auto &iir : iirLow[ch])
                        {
                            iir.coefficients = *rtLowCoeffs;
                            in[i] = iir.processSample(in[i]);
                        }
                    }
                }

                convLow.process(context);
            }

            template <typename T>
            inline void convolveHigh(const dsp::ProcessContextReplacing<T> &context) noexcept
            {
                const auto &input = context.getInputBlock();
                auto &output = context.getOutputBlock();

                farbot::RealtimeObject<ReferenceCountedObjectPtr<dsp::IIR::Coefficients<float>>, farbot::RealtimeObjectOptions::nonRealtimeMutatable>::ScopedAccess<farbot::ThreadType::realtime> rtHighCoeffs(iirHighCoeffs);

                for (int ch = 0; ch < input.getNumChannels(); ++ch)
                {
                    auto *in = output.getChannelPointer(ch);
                    for (int i = 0; i < input.getNumSamples(); ++i)
                    {
                        for (auto &iir : iirHigh[ch])
                        {
                            iir.coefficients = *rtHighCoeffs;
                            in[i] = iir.processSample(in[i]);
                        }
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
            int oversampleFactor = 0;
            int set = 0;

            bool doubleFilter = false;

            std::vector<float> lowinc;
            std::vector<float> highinc;

            std::vector<dsp::IIR::Filter<float>> iirLow[2];
            std::vector<dsp::IIR::Filter<float>> iirHigh[2];
            std::vector<dsp::IIR::Filter<float>> iirLowPulse;
            std::vector<dsp::IIR::Filter<float>> iirHighPulse;

            farbot::RealtimeObject<ReferenceCountedObjectPtr<dsp::IIR::Coefficients<float>>, farbot::RealtimeObjectOptions::nonRealtimeMutatable> iirLowCoeffs;
            farbot::RealtimeObject<ReferenceCountedObjectPtr<dsp::IIR::Coefficients<float>>, farbot::RealtimeObjectOptions::nonRealtimeMutatable> iirHighCoeffs;

            dsp::Convolution convLow, convHigh;

            BufferTransfer lowTransfer;
            BufferTransfer highTransfer;
        };

        /*not actually a thread (or FIR) lmao*/
        struct FIRThread : FIR
        {
            FIRThread(bool twoFilters, size_t order, dsp::ConvolutionMessageQueue &q) : FIR(twoFilters, order, q)
            {
            }

            void prepare(const dsp::ProcessSpec &spec)
            {
                FIR::prepare(spec);
            }

            /*creates new IIRs & IR*/
            void initFilters(double newFreq, double newSampleRate, int newIIROrder) noexcept
            {
                freq = newFreq;
                sampleRate = newSampleRate;
                iirOrder = newIIROrder;

                initFilter(freq, sampleRate);
            }

            /*only changes coeffs of existing IIRs & new IR*/
            void setParams(double newFreq, double newSampleRate, int newIIROrder) noexcept
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
