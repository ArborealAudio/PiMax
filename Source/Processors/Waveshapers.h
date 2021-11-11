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

namespace strix
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

	inline double inflator_v2(double xn, double k, float clip, float type) noexcept
	{
		double yn = 0.0;

		if (type == 1)
			xn += 0.2;

		if (xn > -1.0 && xn < 1.0) {
			if (k > 1.0) {
				yn = dsp::FastMathApproximations::sin(halfPi * pow(k, acos(xn * xn)) * xn);
			}
			else {
				k = 1.0 / k;
				yn = dsp::FastMathApproximations::sin(halfPi * pow(k, log10(xn * xn)) * xn);
			}
		}

		else if (clip == 1) /*clip inputs over digital max*/
			yn = jlimit(-1.0, 1.0, xn);

		else if (clip != 2) {
			if (xn >= 0.0)
				//yn = pow(1 / dsp::FastMathApproximations::cosh(xn - 1.0), pi);
				yn = 1.0 / (1.0 + (1.0 - xn) * (1.0 - xn) * halfPi);
			else
				//yn = -pow(1 / dsp::FastMathApproximations::cosh(xn + 1.0), pi);
				yn = -1.0 / (1.0 + (1.0 + xn) * (1.0 + xn) * halfPi);
		}

		else
			yn = dsp::FastMathApproximations::sin(halfPi * xn);

		return yn;
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

	inline double airwindowsTube(double input)
	{
		//double* in1 = inputs[0];
		//double* in2 = inputs[1];
		//double* out1 = outputs[0];
		//double* out2 = outputs[1];

		//double overallscale = 1.0;
		//overallscale /= 44100.0;
		//overallscale *= getSampleRate();

		double gain = 1.0 + 0.2246161992650486;
		//this maxes out at +1.76dB, which is the exact difference between what a triangle/saw wave
		//would be, and a sine (the fullest possible wave at the same peak amplitude). Why do this?
		//Because the nature of this plugin is the 'more FAT TUUUUBE fatness!' knob, and because
		//sticking to this amount maximizes that effect on a 'normal' sound that is itself unclipped
		//while confining the resulting 'clipped' area to what is already 'fattened' into a flat
		//and distorted region. You can always put a gain trim in front of it for more distortion,
		//or cascade them in the DAW for more distortion.
		double iterations = 1.0;
		int powerfactor = (5.0 * iterations) + 1;
		double gainscaling = 1.0 / (double)(powerfactor + 1);
		double outputscaling = 1.0 + (1.0 / (double)(powerfactor));

		//while (--sampleFrames >= 0)
		//{
			//long double inputSampleL = *in1;
			//long double inputSampleR = *in2;
			//if (fabs(inputSampleL) < 1.18e-43) inputSampleL = fpdL * 1.18e-43;
			//if (fabs(inputSampleR) < 1.18e-43) inputSampleR = fpdR * 1.18e-43;

			//if (overallscale > 1.9) {
			//	long double stored = inputSampleL;
			//	inputSampleL += previousSampleA; previousSampleA = stored; inputSampleL *= 0.5;
			//	stored = inputSampleR;
			//	inputSampleR += previousSampleB; previousSampleB = stored; inputSampleR *= 0.5;
			//} //for high sample rates on this plugin we are going to do a simple average		

			input *= gain;
			//inputSampleR *= gain;

			if (input > 1.0) input = 1.0;
			if (input < -1.0) input = -1.0;
			//if (inputSampleR > 1.0) inputSampleR = 1.0;
			//if (inputSampleR < -1.0) inputSampleR = -1.0;

			double factor = input; //Left channel

			for (int x = 0; x < powerfactor; x++) factor *= input;
			//this applies more and more of a 'curve' to the transfer function

			if (powerfactor % 2 == 1) factor = (factor / input) * fabs(input);
			//if we would've got an asymmetrical effect this undoes the last step, and then
			//redoes it using an absolute value to make the effect symmetrical again

			factor *= gainscaling;
			input -= factor;
			input *= outputscaling;

			return input;

			//factor = input; //Right channel

			//for (int x = 0; x < powerfactor; x++) factor *= inputSampleR;
			////this applies more and more of a 'curve' to the transfer function

			//if (powerfactor % 2 == 1) factor = (factor / inputSampleR) * fabs(inputSampleR);
			////if we would've got an asymmetrical effect this undoes the last step, and then
			////redoes it using an absolute value to make the effect symmetrical again

			//factor *= gainscaling;
			//inputSampleR -= factor;
			//inputSampleR *= outputscaling;

			///*  This is the simplest raw form of the 'fattest' TUBE boost between -1.0 and 1.0

			// if (inputSample > 1.0) inputSample = 1.0; if (inputSample < -1.0) inputSample = -1.0;
			// inputSample = (inputSample-(inputSample*fabs(inputSample)*0.5))*2.0;

			// */

			//if (overallscale > 1.9) {
			//	long double stored = inputSampleL;
			//	inputSampleL += previousSampleC; previousSampleC = stored; inputSampleL *= 0.5;
			//	stored = inputSampleR;
			//	inputSampleR += previousSampleD; previousSampleD = stored; inputSampleR *= 0.5;
			//} //for high sample rates on this plugin we are going to do a simple average

			//begin 64 bit stereo floating point dither
			//int expon; frexp((double)inputSampleL, &expon);
			//fpdL ^= fpdL << 13; fpdL ^= fpdL >> 17; fpdL ^= fpdL << 5;
			//inputSampleL += ((double(fpdL) - uint32_t(0x7fffffff)) * 1.1e-44l * pow(2, expon + 62));
			//frexp((double)inputSampleR, &expon);
			//fpdR ^= fpdR << 13; fpdR ^= fpdR >> 17; fpdR ^= fpdR << 5;
			//inputSampleR += ((double(fpdR) - uint32_t(0x7fffffff)) * 1.1e-44l * pow(2, expon + 62));
			//end 64 bit stereo floating point dither

			/**out1 = inputSampleL;
			*out2 = inputSampleR;

			in1++;
			in2++;
			out1++;
			out2++;*/
		//}
	}

	//inline double processHysteresis(double xn, double y, double xn_d, double k) noexcept
	//{
	//	double j = k > 1.0 ? pow(k, acos(xn * xn)) : pow(1.0 / k, log10(xn * xn));

	//	auto f = [j](double xn)
	//	{
	//		//return sin(halfPi * j * xn);
	//		return 1.0 / tanh(xn) - 1.0 / xn;
	//	};

	//	auto df = [j, k](double xn)
	//	{
	//		/*if (k < 1.0)
	//			return cos(halfPi * j * xn) * halfPi * (2.0 * log(j) + 1) * pow(j, log(xn * xn));*/
	//		return 1.0 - (1.0 / (tanh(xn) * tanh(xn))) + (1.0 / (xn * xn));
	//	};

	//	double Q = (xn + alpha * y) / a;

	//	auto FQ = f(Q);
	//	auto F_primeQ = df(Q);

	//	delta_x = (double)((xn_d >= 0.0) - (xn_d < 0.0));
	//	delta_y = (double)(delta_x > 0.0 - delta_x < 0.0) == ((FQ - y > 0.0) - (FQ - y < 0.0));

	//	double kap1 = (1 - c) * delta_y;
	//	double f1Denom = (1 - c) * delta_x * 0.47875 - alpha * (S * FQ - y);
	//	double f1 = kap1 * (S * FQ - y) / f1Denom;
	//	double f2 = (c * (S / a) * xn_d) * F_primeQ;
	//	double f3 = 1.0 - ((c * alpha * (S / a)) * F_primeQ);

	//	return xn_d * (f1 + f2) / f3;
	//}

	//inline double rungeKutta(double x, double xn_d, double k) noexcept
	//{
	//	double k1 = (1.0 / sR) * processHysteresis(x_n1, y_n1, xn_d1, k);
	//	double k2 = (1.0 / sR) * processHysteresis((x + x_n1) / 2.0, y_n1 + (k1 / 2.0), (xn_d + xn_d1) / 2.0, k);

	//	return y_n1 + k2;
	//}
}
