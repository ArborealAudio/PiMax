#pragma once

/*custom Linkwitz-Riley filter object which contains two independent filter objects for the purpose
of properly cascading band-passed outputs*/
class LRFilter
{
public:

	void prepare(const dsp::ProcessSpec& spec)
	{
		for (auto& band : bands) {
			band.prepare(spec);
		}
		bands[0].setType(dsp::LinkwitzRileyFilterType::highpass);
		bands[1].setType(dsp::LinkwitzRileyFilterType::lowpass);
		bands[2].setType(dsp::LinkwitzRileyFilterType::allpass);
		bands[3].setType(dsp::LinkwitzRileyFilterType::allpass);
	}

	void updateFilter(float crossoverFreq)
	{
		for (auto& band : bands)
			band.setCutoffFrequency(crossoverFreq);
		/*bands[0].setCutoffFrequency(crossoverFreq);
		bands[1].setCutoffFrequency(crossoverFreq);
		bands[2].setCutoffFrequency(crossoverFreq / 2.0);
		bands[3].setCutoffFrequency(crossoverFreq * 2.0);*/
	}

	double getCutoffFreq()
	{
		return bands[0].getCutoffFrequency();
	}

	void reset()
	{
		for (auto& band : bands)
			band.reset();

	}

	/*processes an allpass filter*/
	template <typename T>
	void processAllpass(int channel, T& lowInput, T& highInput)
	{
		lowInput = bands[2].processSample(channel, lowInput);
		highInput = bands[3].processSample(channel, highInput);
	}

	/*processes the first filter object and returns the high-pass output*/
	template <typename T>
	void processSample(int channel, T inputSample, T& outputHigh)
	{
		outputHigh = bands[0].processSample(channel, inputSample);
	}

	/*processes the first filter object and returns the low- and high-pass outputs*/
	template <typename T>
	void processSample(int channel, T inputSample, T& outputLow, T& outputHigh)
	{
		bands[0].processSample(channel, inputSample, outputLow, outputHigh);
	}

	/*processes two inputs and returns two outputs, where the low output overwrites the first input, high output the second input*/
	template <typename T>
	void processTwoSamples(int channel, T input1, T input2, T& outputLow, T& outputHigh)
	{
		outputHigh = bands[0].processSample(channel, input2);
		outputLow = bands[1].processSample(channel, input1);
	}

private:

#if USE_SIMD_SAT
	std::array<dsp::LinkwitzRileyFilter<dsp::SIMDRegister<float>>, 4> bands;
#else
	std::array<dsp::LinkwitzRileyFilter<float>, 4> bands;
#endif
	double lastSampleRate = 0.0;

};