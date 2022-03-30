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
	}

	void updateFilter(float crossoverFreq)
	{
		for (auto& band : bands)
			band.setCutoffFrequency(crossoverFreq);
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
	std::array<dsp::LinkwitzRileyFilter<float>, 2> bands;
#endif
	double lastSampleRate = 0.0;

};