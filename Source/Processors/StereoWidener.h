#pragma once
//#include "JuceHeader.h"

class StereoWidener
{
public:
	StereoWidener() {}
	~StereoWidener() {}

	void prepare(const dsp::ProcessSpec& spec)
	{
		for (auto& d : delay)
			d.prepare(spec);

		delay[0].setDelay(0.008 * spec.sampleRate);
		delay[1].setDelay(0.010 * spec.sampleRate);

		for (auto& d : delay)
			d.reset();
	}

	template <typename T>
	inline void widenBlock(dsp::AudioBlock<T>& block, float width, bool isMono)
	{
		auto& input = block;

		if (isMono && width > 1.0)
			input = widenMonoSource(input, width);
		else
			input = widenStereoSourceBlock(input, width);
	}

	template <typename T>
	inline void widenBuffer(AudioBuffer<T>& buffer, float width, bool isMono)
	{
		auto channelData = buffer.getArrayOfWritePointers();
		dsp::AudioBlock<T> block(channelData, 2, buffer.getNumSamples());

		if (isMono && width > 1.0)
			block = widenMonoSource(block, width);
		else
			block = widenStereoSourceBlock(block, width);
	}

	template <typename T>
	void widenStereoSource(dsp::ProcessContextReplacing<T>& context, float width)
	{
		const auto& input = context.getInputBlock();
		auto& output = context.getOutputBlock();

		auto xnL = input.getChannelPointer(0);
		auto xnR = input.getChannelPointer(1);

		T side = 0.0;
		T mid = 0.0;
		T yn_L = 0.0;
		T yn_R = 0.0;

		for (int i = 0; i < input.getNumSamples(); ++i)
		{
			side = xnL[i] - xnR[i];
			mid = (xnL[i] + xnR[i]) / 2;

			side *= (width / 2);

			yn_L = mid + side;
			yn_R = mid - side;

			output.setSample(0, i, yn_L);
			output.setSample(1, i, yn_R);
		}

	}

	template <typename T>
	dsp::AudioBlock<T> widenStereoSourceBlock(const dsp::AudioBlock<T>& block, float width)
	{
		const auto& input = block;
		auto& output = input;

		auto xnL = input.getChannelPointer(0);
		auto xnR = input.getChannelPointer(1);

		T side = 0.0;
		T mid = 0.0;
		T yn_L = 0.0;
		T yn_R = 0.0;

		for (int i = 0; i < input.getNumSamples(); ++i)
		{
			side = xnL[i] - xnR[i];
			mid = (xnL[i] + xnR[i]) / 2;

			side *= (width / 2);

			yn_L = mid + side;
			yn_R = mid - side;

			output.setSample(0, i, yn_L);
			output.setSample(1, i, yn_R);
		}

		return output;
	}

	template <typename T>
	dsp::AudioBlock<T> widenMonoSource(const dsp::AudioBlock<T>& block, float width)
	{
		auto& input = block;
		auto output = input;
		auto mix = 0.5 * (width - 1.0);

		/*for (int channel = 0; channel < input.getNumChannels(); ++channel) {*/
			for (int i = 0; i < input.getNumSamples(); ++i)
			{
				auto xn = input.getSample(0, i);

				delay[0].pushSample(0, xn);
				delay[1].pushSample(1, xn);

				T xn_DL = delay[0].popSample(0, -1, true);
				T xn_DR = delay[1].popSample(1, -1, true);

				T s_D = xn_DL - xn_DR;

				T yn_L = (s_D * mix) + xn;
				T yn_R = xn - (s_D * mix);

				output.setSample(0, i, yn_L);
				output.setSample(1, i, yn_R);
			}
		//}

		return output;
	}

#if USE_SIMD_SAT
	std::array<dsp::DelayLine<dsp::SIMDRegister<float>, dsp::DelayLineInterpolationTypes::None>, 2> delay
	{ {
		{dsp::DelayLine < dsp::SIMDRegister<float>, dsp::DelayLineInterpolationTypes::None>(44100.0)},
		{dsp::DelayLine < dsp::SIMDRegister<float>, dsp::DelayLineInterpolationTypes::None>(44100.0)}
	} };
#else
	std::array<dsp::DelayLine<double, dsp::DelayLineInterpolationTypes::None>, 2> delay
	{ {
		{dsp::DelayLine < double, dsp::DelayLineInterpolationTypes::None>(44100.0)},
		{dsp::DelayLine < double, dsp::DelayLineInterpolationTypes::None>(44100.0)}
	} };
#endif

};