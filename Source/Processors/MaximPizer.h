#pragma once

class MaximPizer
{
	static constexpr auto halfPi = MathConstants<double>::halfPi;

	AudioProcessorValueTreeState& apvts;

	std::atomic<float>* curve, *clip, *type;

#if USE_SIMD_SAT
	using Vec2 = dsp::SIMDRegister<float>;
	Vec2 x_n1[2]{ 0.0, 0.0 }, x_n2[2]{ 0.0, 0.0 }, y_n1[2]{ 0.0, 0.0 };
#else
	double x_n1[2]{ 0.0, 0.0}, x_n2[2]{ 0.0, 0.0 }, y_n1[2]{ 0.0, 0.0 };
#endif

public:

	MaximPizer(AudioProcessorValueTreeState& a) : apvts(a)
	{
		curve = apvts.getRawParameterValue("curve");
		clip = apvts.getRawParameterValue("clipType");
		type = apvts.getRawParameterValue("distType");
	}

	void prepare()
	{
		x_n1[0] = 0.0;
		x_n1[1] = 0.0;
		x_n2[0] = 0.0;
		x_n2[1] = 0.0;
		y_n1[0] = 0.0;
		y_n1[1] = 0.0;
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

	//template <typename T>
	//inline dsp::SIMDRegister<T> processSample(int channel, dsp::SIMDRegister<T> xn) noexcept
	//{
	//	dsp::SIMDRegister<T> yn, x1, x2;
	//	double k = *curve;
	//	auto xPi = xn * halfPi;

	//	/*DC offset for asym (need to ramp offset incase of switch)*/
	//	xn += *type == 1 ? 0.2 : 0.0;

	//	using xs = xsimd::batch<T>;

	//	auto sum = xsimd::add(xsimd::abs((xs)xn.value), xsimd::abs((xs)x_n2[channel].value));
	//	sum = xsimd::div(sum, (xs)2.0);
	//	const auto a = (Vec2)jlimit((xs)0.0, (xs)1.0, sum);

	//	auto g_n1 = dsp::SIMDRegister<T>::greaterThan(xn.value, (T)-1.0);
	//	auto l_1 = dsp::SIMDRegister<T>::lessThan(xn.value, (T)1.0);
	//	auto g_0 = dsp::SIMDRegister<T>::greaterThanOrEqual(xn.value, (T)0.0);

	//	if (k > 1.0) {
	//		const auto j = (Vec2)xsimd::pow((xs)k, xsimd::acos((xs)xn.value * (xs)xn.value)) & g_n1 & l_1;
	//		auto xk = xsimd::pow((xs)xn.value, (xs)k);
	//		x1 = (Vec2)xsimd::sin((xs)xPi.value * (xs)j) & g_n1 & l_1;
	//		x2 = ((Vec2)xsimd::sin((xs)halfPi * xk) & g_n1 & l_1 & g_0) +
	//			((Vec2)-xsimd::sin((xs)halfPi * -xk) & g_n1 & l_1 & ~g_0);
	//		yn = (Vec2)x1 * ((Vec2)1.0 - a) + x2 * a & g_n1 & l_1;
	//	}
	//	else if (k < 1.0) {
	//		const auto k_r = (1.0 / k);
	//		const auto j = (Vec2)xsimd::pow((xs)k_r, xsimd::acos((xs)xn.value * (xs)xn.value)) & g_n1 & l_1;
	//		auto xk = xsimd::pow((xs)xn.value, (xs)k_r);
	//		x1 = (Vec2)xsimd::sin((xs)xPi.value * (xs)j) & g_n1 & l_1;
	//		x2 = ((Vec2)xsimd::sin((xs)halfPi * xk) & l_1 & g_0) +
	//			((Vec2)-xsimd::sin((xs)halfPi * -xk) & g_n1 & ~g_0);
	//		yn = (Vec2)x1 * a + x2 * ((Vec2)1.0 - a) & g_n1 & l_1;
	//	}
	//	else
	//		yn = (Vec2)xsimd::sin((xs)xPi.value) & g_n1 & l_1;

	//	if (*clip == 1) {
	//		yn = ((Vec2)1.0 & ~l_1) +
	//			((Vec2)-1.0 & ~g_n1);
	//	}

	//	else if (*clip != 2) {
	//		auto one_min_x = (Vec2)xsimd::sub((xs)1.0, xn.value);
	//		auto one_plus_x = (Vec2)xsimd::add((xs)1.0, xn.value);
	//		auto pos_denom = (Vec2)1.0 + one_min_x * one_min_x * halfPi;
	//		auto neg_denom = (Vec2)1.0 + one_plus_x * one_plus_x * halfPi;
	//		auto y_pos = (Vec2)xsimd::div((xs) 1.0, (xs)pos_denom.value);
	//		auto y_neg = (Vec2)xsimd::div((xs) -1.0, (xs)neg_denom.value);
	//		yn = (y_pos & g_0 & ~l_1) + (y_neg & ~g_0 & ~g_n1);
	//	}

	//	else
	//		yn = (Vec2)xsimd::sin((xs)xPi.value);

	//	auto avgsum = yn + y_n1[channel];
	//	auto avg = yn - xsimd::div((xs)avgsum.value, (xs)2.0);
	//	if (k > 1.0)
	//		yn -= avg * (Vec2)(1.0 - 1.0 / k);
	//	else if (k < 1.0)
	//		yn += avg * (Vec2)(1.0 - k);

	//	x_n2[channel] = x_n1[channel];
	//	x_n1[channel] = xn;
	//	y_n1[channel] = yn;

	//	return yn;
	//}

	template <typename T>
	inline T processSample(int channel, T xn) noexcept
	{
		T yn, x1, x2;
		double k = *curve;

		/*DC offset for asym (need to ramp offset incase of switch)*/
		xn += *type == 1 ? 0.1 : 0.0;

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
		else if (*clip == 1) {
			yn = jlimit((T) -1.0, (T) 1.0, xn);
		}
		/*single-period for finite type*/
		else if (*clip != 2) {
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