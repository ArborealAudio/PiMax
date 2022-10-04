#pragma once

class MaximPizer
{
	static constexpr auto halfPi = MathConstants<double>::halfPi;
    static constexpr auto pi = MathConstants<float>::pi;

    AudioProcessorValueTreeState& apvts;

    strix::SVTFilter<float> hiShelf, lowShelf;

	std::atomic<float>* curve, *clip, *type, *boost, *gain;
    float curve_m = 0, type_m = 0;
    ClipType clip_m;
    bool boost_m = false;

	double x_n1[2]{0.0}, x_n2[2]{0.0}, y_n1[2]{0.0};

public:

	MaximPizer(AudioProcessorValueTreeState& a) : apvts(a)
	{
        gain = apvts.getRawParameterValue("gain");
		curve = apvts.getRawParameterValue("curve");
		clip = apvts.getRawParameterValue("clipType");
		type = apvts.getRawParameterValue("distType");
		boost = apvts.getRawParameterValue("boost");
	}

	void prepare(const dsp::ProcessSpec& spec)
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
	}

    void reset()
    {
        hiShelf.reset();
        x_n1[0] = x_n1[1] = x_n2[0] = x_n2[1] = y_n1[0] = y_n1[1] = 0.0;
    }

    void loadAtomics()
    {
        curve_m = curve->load();
        clip_m = (ClipType)clip->load();
        type_m = type->load();
        boost_m = boost->load();

        hiShelf.setCutoffFreq(5000.0 * (1.f / (curve_m * curve_m * curve_m)));
        lowShelf.setCutoffFreq(150.f * (curve_m * curve_m));
    }

	template <typename T>
	void process(dsp::AudioBlock<T>& block)
	{
        loadAtomics();
        
        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
        {
            auto in = block.getChannelPointer(ch);

            if (type_m)
                FloatVectorOperations::add(in, 0.1, block.getNumSamples());
                        
            switch (clip_m)
            {
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
	void processRegular(size_t channel, size_t numSamples, T* in) noexcept
	{
		T yn, x1, x2;
		double k = curve_m;
		if (boost_m)
			k *= k;

        for (int i = 0; i < numSamples; ++i)
        {
            /*if we're in regular bounds, apply curve algo*/
            const auto xn = in[i];

            const auto a = jlimit(0.0, 1.0, (std::abs(xn) + std::abs(x_n2[channel])) / 2.0);

            if (xn > -1.0 && xn < 1.0) {
                if (k > 1.0) {
                    const auto j = pow(k, acos(xn * xn));
                    x1 = dsp::FastMathApproximations::sin(halfPi * j * xn);
                    if (xn >= 0.0)
                        x2 = dsp::FastMathApproximations::sin(halfPi * pow(xn, k));
                    else
                        x2 = -dsp::FastMathApproximations::sin(halfPi * pow(-xn, k));
                    yn = x1 * (1.0 - a) + x2 * a;
                }
                else if (k < 1.0) {
                    const auto k_r = (1.0 / k);
                    const auto j = pow(k_r, acos(xn * xn));
                    x1 = dsp::FastMathApproximations::sin(halfPi * j * xn);
                    if (xn >= 0.0)
                        x2 = dsp::FastMathApproximations::sin(halfPi * pow(xn, k_r));
                    else
                        x2 = -dsp::FastMathApproximations::sin(halfPi * pow(-xn, k_r));
                    yn = x1 * a + x2 * (1.0 - a);
                }
                else
                    yn = sin(halfPi * xn);
            }
            /*check for clip*/
            else
            {
                switch (clip_m)
                {
                case ClipType::Finite:
                    if (xn >= 0.0)
                        //yn = pow(1.0 / cosh(xn - 1.0), pi);
                        yn = 1.0 / (1.0 + (1.0 - xn) * (1.0 - xn) * halfPi);
                    else
                        //yn = -pow(1.0 / cosh(xn + 1.0), pi);
                        yn = -1.0 / (1.0 + (1.0 + xn) * (1.0 + xn) * halfPi);
                    break;
                case ClipType::Clip:
                    yn = jlimit((T) -1.0, (T) 1.0, xn);
                    break;
                case ClipType::Infinite:
                    /*full sine for infinite type*/
                    yn = sin(halfPi * xn);
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

    template <typename T>
    void processDeep(size_t ch, size_t numSamples, T* xn)
    {
        const double k = curve_m * curve_m;

        FloatVectorOperations::multiply(xn, halfPi, numSamples);

        for (int i = 0; i < numSamples; ++i)
        {
            xn[i] = xn[i] / std::pow((1.0 + std::pow(std::abs(xn[i]), k)), 1.0 / k);
            xn[i] += 0.2f * lowShelf.processSample(ch, xn[i]);
            xn[i] += 0.2f * hiShelf.processSample(ch, xn[i]);
        }

        if (k < 1.0)
            FloatVectorOperations::multiply(xn, 1.0 / k, numSamples);
    }

    template <typename T>
    void processWarm(size_t ch, size_t numSamples, T* xn)
    {
        FloatVectorOperations::multiply(xn, halfPi, numSamples);

        auto lfGain = curve_m >= 1.f ? curve_m * curve_m - 1.f : 0.f;

        for (int i = 0; i < numSamples; ++i)
        {
            auto hf = 0.3f * hiShelf.processSample(ch, xn[i]);
            auto lf = lfGain * lowShelf.processSample(ch, xn[i]);
            xn[i] -= hf;
            xn[i] += lf;
            xn[i] = (-1.0 / (1.0 + std::exp(xn[i])) + 0.5);
        }

        FloatVectorOperations::multiply(xn, e, numSamples);
    }

	inline double acos(double x)
	{
		return (-0.69813170079773212 * x * x - 0.87266462599716477) * x + 1.5707963267948966;
	}

	////powf(10.f,x) is exactly exp(log(10.0f)*x)
	inline double pow(double a, double b) { return exp(log(a) * b); }

	//// This is a fast approximation to log2()
	//// Y = C[0]*F*F*F + C[1]*F*F + C[2]*F + C[3] + E;
	template <typename T>
	T log2_approx(T X) {
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
		return(Y);
	}

	////log10f is exactly log2(x)/log2(10.0f)
	template <typename T>
	T log10_fast(T x) { return (log2_approx(x) * 0.3010299956639812f); }

};