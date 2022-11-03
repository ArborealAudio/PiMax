#pragma once

template <typename T>
class StereoWidener
{
public:
	StereoWidener() {}
	~StereoWidener() {}

	void prepare(const dsp::ProcessSpec& spec)
	{
        auto forceSpec = spec;
        forceSpec.numChannels = 2;
		for (auto& d : delay)
			d.prepare(forceSpec);

		delay[0].setDelay(0.008 * spec.sampleRate);
		delay[1].setDelay(0.010 * spec.sampleRate);

		for (auto& d : delay)
			d.reset();
	}

	inline void widenBlock(dsp::AudioBlock<T>& block, float width, bool isMono)
	{
        if (block.getNumChannels() < 2)
            return;
        
        auto& input = block;

		if ((isMono && width > 1.0))
			input = widenMonoSource(input, width);
		else if (!isMono)
			input = widenStereoSourceBlock(input, width);
        else return;
    }

    inline void widenBlockWithRamp(dsp::AudioBlock<T>& block, float beginWidth, float endWidth, bool isMono)
    {
        if (block.getNumChannels() < 2)
            return;
        
        if (isMono && beginWidth > 1.0 && endWidth > 1.0)
            block = widenMonoSourceWithRamp(block, beginWidth, endWidth);
        else if (!isMono)
            block = widenStereoSourceBlockWithRamp(block, beginWidth, endWidth);
        else return;
    }

	inline void widenBuffer(AudioBuffer<T>& buffer, float width, bool isMono)
	{
        if (buffer.getNumChannels() < 2)
            return;
        
		dsp::AudioBlock<T> block(buffer);

		if (isMono && width > 1.0)
			block = widenMonoSource(block, width);
		else if (!isMono)
			block = widenStereoSourceBlock(block, width);
        else return;
	}
    
    inline void widenBufferWithRamp(AudioBuffer<T>& buffer, float beginWidth, float endWidth, bool isMono)
    {
        if (buffer.getNumChannels() < 2)
            return;
        
        dsp::AudioBlock<T> block(buffer);

        if (isMono && beginWidth > 1.0 && endWidth > 1.0)
            block = widenMonoSourceWithRamp(block, beginWidth, endWidth);
        else if (!isMono)
            block = widenStereoSourceBlockWithRamp(block, beginWidth, endWidth);
        else return;
    }

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
    
    dsp::AudioBlock<T> widenStereoSourceBlockWithRamp(const dsp::AudioBlock<T>& block, float beginWidth,
                                                      float endWidth)
    {
        const auto& input = block;
        auto& output = input;

        auto xnL = input.getChannelPointer(0);
        auto xnR = input.getChannelPointer(1);

        T side = 0.0;
        T mid = 0.0;
        T yn_L = 0.0;
        T yn_R = 0.0;
        
        auto inc = (endWidth - beginWidth) / (float)block.getNumSamples();

        for (int i = 0; i < input.getNumSamples(); ++i)
        {
            side = xnL[i] - xnR[i];
            mid = (xnL[i] + xnR[i]) / 2;

            side *= (beginWidth / 2);
            beginWidth += inc;

            yn_L = mid + side;
            yn_R = mid - side;

            output.setSample(0, i, yn_L);
            output.setSample(1, i, yn_R);
        }

        return output;
    }

	dsp::AudioBlock<T> widenMonoSource(const dsp::AudioBlock<T>& block, float width)
	{
		auto output = block;
		auto mix = 0.5 * (width - 1.0);

			for (int i = 0; i < block.getNumSamples(); ++i)
			{
				auto xn = block.getSample(0, i);

				delay[0].pushSample(0, xn);
				delay[1].pushSample(1, xn);

				T xn_DL = delay[0].popSample(0);
				T xn_DR = delay[1].popSample(1);

				T s_D = mix * (xn_DL - xn_DR);

				T yn_L = s_D + xn;
				T yn_R = xn - s_D;
                
				output.setSample(0, i, yn_L);
                output.setSample(1, i, yn_R);
			}

		return output;
	}
    
    dsp::AudioBlock<T> widenMonoSourceWithRamp(const dsp::AudioBlock<T>& block, float beginWidth,
                                               float endWidth)
    {
        auto output = block;
        
        auto inc = (endWidth - beginWidth) / (float)block.getNumSamples();

            for (int i = 0; i < block.getNumSamples(); ++i)
            {
                auto xn = block.getSample(0, i);
                
                delay[0].pushSample(0, xn);
                delay[1].pushSample(1, xn);

                T xn_DL = delay[0].popSample(0, -1, true);
                T xn_DR = delay[1].popSample(1, -1, true);

                auto mix = 0.5 * (beginWidth - 1.0);
                beginWidth += inc;
                
                T s_D = mix * (xn_DL - xn_DR);

                T yn_L = s_D + xn;
                T yn_R = xn - s_D;

                output.setSample(0, i, yn_L);
                output.setSample(1, i, yn_R);
            }

        return output;
    }

	std::array<dsp::DelayLine<T, dsp::DelayLineInterpolationTypes::None>, 2> delay
	{ {
		{dsp::DelayLine <T, dsp::DelayLineInterpolationTypes::None>(44100.0)},
		{dsp::DelayLine <T, dsp::DelayLineInterpolationTypes::None>(44100.0)}
	} };

};
