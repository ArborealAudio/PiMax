#pragma once

/*custom Linkwitz-Riley filter object which contains two independent filter objects for the purpose
of properly cascading band-passed outputs*/
template <typename T>
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

    /*process one channel and return the high output*/
    void process(const T* input, T* outputHigh, size_t numSamples, int ch)
    {
        for (size_t i = 0; i < numSamples; ++i)
        {
            outputHigh[i] = bands[0].processSample(ch, input[i]);
        }
    }

    /*process one channel and return low and high outputs*/
    void process(const T* input, T* outputLow, T* outputHigh, size_t numSamples, int ch)
    {
        for (size_t i = 0; i < numSamples; ++i)
        {
            bands[0].processSample(ch, input[i], outputLow[i], outputHigh[i]);
        }
    }

    /*process low and high channels independently*/
    void process(T* low, T* high, size_t numSamples, int ch)
    {
        for (size_t i = 0; i < numSamples; ++i)
        {
            high[i] = bands[0].processSample(ch, high[i]);
            low[i] = bands[1].processSample(ch, low[i]);
        }
    }

private:

#if USE_SIMD_SAT
	std::array<dsp::LinkwitzRileyFilter<dsp::SIMDRegister<float>>, 4> bands;
#else
	std::array<dsp::LinkwitzRileyFilter<T>, 2> bands;
#endif
	double lastSampleRate = 0.0;

};