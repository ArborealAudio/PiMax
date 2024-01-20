#pragma once
#include "../PluginProcessor.h"
#include <JuceHeader.h>

static constexpr auto halfPi = MathConstants<double>::halfPi;
static constexpr double sqrtHalfPi = 1.25331413731550025;
static constexpr auto pi = MathConstants<float>::pi;
static constexpr auto e = MathConstants<float>::euler;

class MaximPizer
{

    AudioProcessorValueTreeState &apvts;

    strix::SVTFilter<float> hiShelf, lowShelf;

    std::atomic<float> *clip, *boost, *gain;
    strix::FloatParameter *curve;
    SmoothedValue<float> smoothCurve[2];
    float lastCurve = 1.f;
    ClipType clip_m;
    bool boost_m = false;

    double x_n1[2]{0.0}, x_n2[2]{0.0}, y_n1[2]{0.0};

  public:
    MaximPizer(AudioProcessorValueTreeState &a) : apvts(a)
    {
        gain = apvts.getRawParameterValue("gain");
        curve =
            dynamic_cast<strix::FloatParameter *>(apvts.getParameter("curve"));
        clip = apvts.getRawParameterValue("clipType");
        boost = apvts.getRawParameterValue("boost");
    }

    void prepare(const dsp::ProcessSpec &spec)
    {
        x_n1[0] = x_n1[1] = x_n2[0] = x_n2[1] = y_n1[0] = y_n1[1] = 0.0;

        hiShelf.prepare(spec);
        hiShelf.setType(strix::FilterType::firstOrderHighpass);
        hiShelf.setCutoffFreq(5000.0 * (1.f / (*curve * *curve * *curve)));
        hiShelf.setResonance(0.5f);

        lowShelf.prepare(spec);
        lowShelf.setType(strix::FilterType::firstOrderLowpass);
        lowShelf.setCutoffFreq(150.f * (*curve * *curve));
        lowShelf.setResonance(0.5f);

        smoothCurve[0].reset(spec.sampleRate, 0.01);
        smoothCurve[1].reset(spec.sampleRate, 0.01);
    }

    void reset()
    {
        hiShelf.reset();
        x_n1[0] = x_n1[1] = x_n2[0] = x_n2[1] = y_n1[0] = y_n1[1] = 0.0;
    }

    void loadAtomics()
    {
        clip_m = (ClipType)clip->load();
        boost_m = (bool)boost->load();

        if (lastCurve != *curve) {
            hiShelf.setCutoffFreq(5000.0 * (1.f / (*curve * *curve * *curve)));
            lowShelf.setCutoffFreq(150.f * (*curve * *curve));
            lastCurve = *curve;
        }
    }

    template <typename T> void process(dsp::AudioBlock<T> &block)
    {
        loadAtomics();

        for (size_t ch = 0; ch < block.getNumChannels(); ++ch) {
            auto in = block.getChannelPointer(ch);

            switch (clip_m) {
            case ClipType::Deep:
                processDeep(ch, block.getNumSamples(), in);
                break;
            case ClipType::Warm:
                processWarm(ch, block.getNumSamples(), in);
                break;
            default:
                processRegular(ch, block.getNumSamples(), in);
                break;
            };
        }
    }

    template <typename T>
    void processRegular(size_t channel, size_t numSamples, T *in) noexcept
    {
        T yn = 0.0, x1 = 0.0, x2 = 0.0, k = 0.0;

        if (boost_m)
            smoothCurve[channel].setTargetValue(*curve * *curve);
        else
            smoothCurve[channel].setTargetValue(*curve);
        for (size_t i = 0; i < numSamples; ++i) {
            k = smoothCurve[channel].getNextValue();
            /*if we're in regular bounds, apply curve algo*/
            const auto xn = in[i];

            const auto a =
                jmin(1.0, (std::abs(xn) + std::abs(x_n2[channel])) / 2.0);

            if (xn > -1.0 && xn < 1.0) {
                if (k > 1.0) {
                    const auto j = this->pow(k, this->acos(xn * xn));
                    x1 = dsp::FastMathApproximations::sin(halfPi * j * xn);
                    if (xn >= 0.0)
                        x2 = dsp::FastMathApproximations::sin(halfPi *
                                                              this->pow(xn, k));
                    else
                        x2 = -dsp::FastMathApproximations::sin(
                            halfPi * this->pow(-xn, k));
                    yn = x1 * (1.0 - a) + x2 * a;
                } else if (k < 1.0) {
                    const auto k_r = (1.0 / k);
                    const auto j = this->pow(k_r, this->acos(xn * xn));
                    x1 = dsp::FastMathApproximations::sin(halfPi * j * xn);
                    if (xn >= 0.0)
                        x2 = dsp::FastMathApproximations::sin(
                            halfPi * this->pow(xn, k_r));
                    else
                        x2 = -dsp::FastMathApproximations::sin(
                            halfPi * this->pow(-xn, k_r));
                    yn = x1 * a + x2 * (1.0 - a);
                } else
                    yn = std::sin(halfPi * xn);
            }
            /*check for clip*/
            else {
                switch (clip_m) {
                case ClipType::Finite:
                    if (xn >= 0.0)
                        // yn = pow(1.0 / cosh(xn - 1.0), pi);
                        yn = 1.0 / (1.0 + (1.0 - xn) * (1.0 - xn) * halfPi);
                    else
                        // yn = -pow(1.0 / cosh(xn + 1.0), pi);
                        yn = -1.0 / (1.0 + (1.0 + xn) * (1.0 + xn) * halfPi);
                    break;
                case ClipType::Clip:
                    yn = jlimit((T)-1.0, (T)1.0, xn);
                    break;
                case ClipType::Infinite:
                    /*full sine for infinite type*/
                    yn = std::sin(halfPi * xn);
                    break;
                }
            }

            auto avg = yn - (yn + y_n1[channel]) / 2.0;
            if (k > 1.0)
                yn -= avg * (1.0 - 1.0 / k);
            else if (k < 1.0)
                yn += avg * (1.0 - k);

            x_n2[channel] = x_n1[channel];
            x_n1[channel] = xn;
            y_n1[channel] = yn;
            in[i] = yn;
        }
    }

    template <typename T> void processDeep(size_t ch, size_t numSamples, T *xn)
    {
        FloatVectorOperations::multiply(xn, halfPi, numSamples);
        T k = 0.0;
        if (smoothCurve[ch].isSmoothing()) {
            smoothCurve[ch].setTargetValue(*curve * *curve);
            for (int i = 0; i < numSamples; ++i) {
                k = smoothCurve[ch].getNextValue();
                xn[i] = xn[i] / this->pow((1.0 + this->pow(std::abs(xn[i]), k)),
                                          1.0 / k);
                xn[i] += 0.2f * lowShelf.processSample(ch, xn[i]);
                xn[i] += 0.2f * hiShelf.processSample(ch, xn[i]);
                if (k < 1.0)
                    xn[i] *= 1.0 / k;
            }
        } else {
            k = *curve * *curve;
            for (int i = 0; i < numSamples; ++i) {
                xn[i] = xn[i] / this->pow((1.0 + this->pow(std::abs(xn[i]), k)),
                                          1.0 / k);
                xn[i] += 0.2f * lowShelf.processSample(ch, xn[i]);
                xn[i] += 0.2f * hiShelf.processSample(ch, xn[i]);
            }
            if (k < 1.0)
                FloatVectorOperations::multiply(xn, 1.0 / k, numSamples);
        }
    }

    template <typename T> void processWarm(size_t ch, size_t numSamples, T *xn)
    {
        FloatVectorOperations::multiply(xn, halfPi, numSamples);

        smoothCurve[ch].setTargetValue(*curve >= 1.f ? *curve * *curve - 1.f
                                                     : 0.f);

        for (int i = 0; i < numSamples; ++i) {
            auto hf = 0.3f * hiShelf.processSample(ch, xn[i]);
            auto lf = smoothCurve[ch].getNextValue() *
                      lowShelf.processSample(ch, xn[i]);
            xn[i] -= hf;
            xn[i] += lf;
            xn[i] = (-1.0 / (1.0 + std::exp(xn[i])) + 0.5);
        }

        FloatVectorOperations::multiply(xn, e, numSamples);
    }

    inline double acos(double x)
    {
        return (-0.69813170079773212 * x * x - 0.87266462599716477) * x +
               1.5707963267948966;
    }

    ////powf(10.f,x) is exactly exp(log(10.0f)*x)
    inline double pow(double a, double b) { return exp(log(a) * b); }

    //// This is a fast approximation to log2()
    //// Y = C[0]*F*F*F + C[1]*F*F + C[2]*F + C[3] + E;
    template <typename T> T log2_approx(T X)
    {
        T Y, F;
        int E;
        F = frexpf(fabsf(X), &E);
        Y = 1.23149591368684f;
        Y *= F;
        Y += -4.11852516267426f;
        Y *= F;
        Y += 6.02197014179219f;
        Y *= F;
        Y += -3.13396450166353f;
        Y += E;
        return (Y);
    }

    ////log10f is exactly log2(x)/log2(10.0f)
    template <typename T> T log10_fast(T x)
    {
        return (log2_approx(x) * 0.3010299956639812f);
    }
};