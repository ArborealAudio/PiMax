#pragma once

class MaximPizer
{
	static constexpr auto halfPi = MathConstants<double>::halfPi;

	AudioProcessorValueTreeState& apvts;

	std::atomic<float>* curve, *clip, *type, *boost;
    float curve_m = 0.f;
    int clip_m = 0, type_m = 0;
    bool boost_m = false;

	double x_n1[2]{0.0}, x_n2[2]{0.0}, y_n1[2]{0.0};

public:

	MaximPizer(AudioProcessorValueTreeState& a) : apvts(a)
	{
		curve = apvts.getRawParameterValue("curve");
		clip = apvts.getRawParameterValue("clipType");
		type = apvts.getRawParameterValue("distType");
		boost = apvts.getRawParameterValue("boost");
	}

	void prepare()
	{
		x_n1[0] = x_n1[1] = x_n2[0] = x_n2[1] = y_n1[0] = y_n1[1] = 0.0;
	}

    void loadAtomics()
    {
        curve_m = curve->load();
        clip_m = clip->load();
        type_m = type->load();
        boost_m = boost->load();
    }

	template <typename T>
	void process(const dsp::ProcessContextReplacing<T>& context)
	{
		const auto& input = context.getInputBlock();
		auto& output = context.getOutputBlock();

		for (size_t i = 0; i < input.getNumSamples(); ++i)
		{
			auto x = input.getSample(0, i);
			x = processSample(0, x);
			output.setSample(0, i, x);
		}
	}

	template <typename T>
	inline T processSample(int channel, T xn) noexcept
	{
		T yn, x1, x2;
		double k = curve_m;
		if (boost_m)
			k *= k;

		/*DC offset for asym (need to ramp offset incase of switch)*/
		xn += type_m == 1 ? 0.1 : 0.0;

		/*if we're in regular bounds, apply curve algo*/

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
		else if (clip_m == 1) {
			yn = jlimit((T) -1.0, (T) 1.0, xn);
		}
		/*single-period for finite type*/
		else if (clip_m != 2) {
			if (xn >= 0.0)
				//yn = pow(1.0 / cosh(xn - 1.0), pi);
				yn = 1.0 / (1.0 + (1.0 - xn) * (1.0 - xn) * halfPi);
			else
				//yn = -pow(1.0 / cosh(xn + 1.0), pi);
				yn = -1.0 / (1.0 + (1.0 + xn) * (1.0 + xn) * halfPi);
		}
		/*full sine for infinite type*/
		else {
			yn = sin(halfPi * xn);
		}

		auto avg = yn - (yn + y_n1[channel]) / 2.0;
		if (k > 1.0)
			yn -= avg * (1.0 - 1.0 / k);
		else if (k < 1.0)
			yn += avg * (1.0 - k);

		x_n2[channel] = x_n1[channel];
		x_n1[channel] = xn;
		y_n1[channel] = yn;
		
		return yn;
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