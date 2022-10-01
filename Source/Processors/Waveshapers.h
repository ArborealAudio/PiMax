#pragma once
#include <JuceHeader.h>

#ifndef NDEBUG
#define X(textToWrite)                                                         \
  JUCE_BLOCK_WITH_FORCED_SEMICOLON(juce::String tempDbgBuf;                    \
                                   tempDbgBuf << textToWrite;                  \
                                   juce::Logger::writeToLog(tempDbgBuf);)

#define X_EVERY(n, textToWrite)                                                \
  {                                                                            \
    static int j = 0;                                                          \
    if ((j % static_cast<int>(n)) == 0) {                                      \
      X(textToWrite);                                                          \
    }                                                                          \
    ++j;                                                                       \
  }
#endif

static constexpr auto pi = MathConstants<double>::pi;
static constexpr auto e = MathConstants<double>::euler;
static constexpr auto halfPi = MathConstants<double>::halfPi;
//float lastInputGain = 0.0;
/*refactor use of param smoothing where input gain has already been smoothed.
Perhaps include in this header file an inline-able function for param smoothing*/

namespace waveshapers
{
	/*double str_x_PreAmp(double xn, float gain)
	{
		double yn = 0.0;

		return yn;
	}*/

	inline double gamma_PreAmp_v1(double xn, float gain)
	{
		double yn = 0.0;
		/*float currentInputGain = gain;
		if (currentInputGain != lastInputGain)
			lastInputGain = 0.001f * currentInputGain + (1.0 - 0.001f) * lastInputGain;
		else
			lastInputGain = gain;*/

		float k = gain;
		float nk = 0.4;

		xn += 0.5;

		if (xn > 4.0) {
			yn = 4.0;
		}
		else if (xn > 0.0) {
			xn /= 4.0;
			yn = std::atan(k * xn) / std::atan(k);
			yn *= 4.0;
		}
		else {
			if (xn < -1.0)
				yn = -1.0;
			else
				yn = nk * (std::atan((k / nk) * xn) / std::atan(k / nk));
		}

		return yn;
	}

	inline double gamma_PreAmp_v2(double xn, float inputGain)
	{
		double yn = 0.0;

		float k = inputGain * 0.6;
		float nk = 0.5;

		xn += 0.05;

		if (xn > 0.0) {
			yn = 2 * (1 / (1 + exp(-k * xn))) - 1;
		}
		else {
			yn = nk * (2 * (1 / (1 + exp(-(k / nk) * xn))) - 1);
		}

		return yn;
	}

	inline double poletti_ClassB(double xn, double g, double Ln, double Lp)
	{
		// run twice for positive and negative, switching Ln and Lp, run again with symmetric limits
		double yn = 0.0;
		if (xn <= 0)
			yn = (g * xn) / (1.0 - ((g * xn) / Ln));
		else
			yn = (g * xn) / (1.0 + ((g * xn) / Lp));
		return yn;
	}

	inline double piShaper(double xn, float inputGain)
	{
		double yn = 0.0;

		yn = (std::atan(pow(pi, (e * inputGain * xn))) - 0.5 / std::atan(pow(pi, (e * inputGain)))) / 0.5;

		return yn;
	}

	inline double logShaper(double xn, float inputGain)
	{
		double yn = 0.0;
		double yn_pos = 0.0;
		double yn_neg = 0.0;

		if (xn > 0.0)
			yn_pos = (log10(inputGain * (xn / 0.6)) + 1.6) / inputGain;
		else
			yn_neg = (-log10(inputGain * (-xn / 0.6)) - 1.6) / inputGain;

		yn = yn_pos + yn_neg;

		yn /= 2;

		return yn;
	}

	inline double arraya (double xn)
	{
		double yn = 0.0;

		yn = (3 * xn / 2) * (1 - (xn * xn) / 3);

		return yn;
	}

	inline double inflator(double xn, double k, float clip) //includes phase inversion at high gain
	{
		double yn = 0.0;

		if (xn >= 0.0) {
			if (xn < 1.0)
				yn = sin(halfPi * pow(k, log10(xn)) * xn);
			else if (clip)
				yn = 1.0;
			else
				yn = sin(halfPi * xn);
		}

		else if (xn < 0.0) {
			if (xn > -1.0)
				yn = sin(halfPi * pow(k, log10(-xn)) * xn);
			else if (clip)
				yn = -1.0;
			else
				yn = sin(halfPi * xn);
		}

		return yn;
	}

	template <typename T>
	void inflator_v2(T* xn, const size_t numSamples, T k, const ClipType clip, const bool type) noexcept
	{
		if (type)
			FloatVectorOperations::add(xn, 0.2, numSamples);

		switch (clip)
		{
		case ClipType::Deep: {
			const auto curve = k * k;

			FloatVectorOperations::multiply(xn, pi, numSamples);

			for (int i = 0; i < numSamples; ++i)
				xn[i] = xn[i] / std::pow((1.0 + std::pow(std::abs(xn[i]), curve)), 1.0 / curve);

			if (curve < 1.0)
				FloatVectorOperations::multiply(xn, 1.0 / curve, numSamples);
			break;
			}
		case ClipType::Warm:
			FloatVectorOperations::multiply(xn, halfPi, numSamples);

			for (int i = 0; i < numSamples; ++i)
				xn[i] = (-1.0 / (1.0 + std::exp(xn[i])) + 0.5);

			FloatVectorOperations::multiply(xn, e, numSamples);
			break;
		default:
			for (size_t i = 0; i < numSamples; ++i)
			{
				T yn = 0.0;
				const T x = xn[i];

				if (x > -1.0 && x < 1.0) {
					if (k > 1.0) {
						yn = dsp::FastMathApproximations::sin(halfPi * pow(k, acos(x * x)) * x);
					}
					else {
						auto nk = 1.0 / k;
						yn = dsp::FastMathApproximations::sin(halfPi * pow(nk, log10(x * x)) * x);
					}
				}

				else if (clip == ClipType::Clip) /*clip inputs over digital max*/
					yn = jlimit((T)-1.0, (T)1.0, x);

				else if (clip != ClipType::Infinite) {
					if (x >= 0.0)
						yn = 1.0 / (1.0 + (1.0 - x) * (1.0 - x) * halfPi);
					else
						yn = -1.0 / (1.0 + (1.0 + x) * (1.0 + x) * halfPi);
				}

				else
					yn = dsp::FastMathApproximations::sin(halfPi * x);

				xn[i] = yn;
			}
			break;
		}
	}

	inline double crossover_ClassB(double xn)
	{
		double yn = 0.0;

		if (xn > 1.0)
			yn = 1.0;
		else if (xn < -1.0)
			yn = -1.0;
		else
			yn = pow((sin(halfPi * xn)), 3);

		return yn;
	}
}
