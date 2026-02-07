/* Simple resampler based on bsnes's ruby audio library */

#pragma once

#include <snes9x/ring_bufferSNSF.h>

inline bool fEqual(float a, float b) { return fabs(a - b) < 1e-6; }

class ResamplerSNSF : public ring_bufferSNSF
{
public:
	virtual void clear() = 0;
	virtual void time_ratio(double) = 0;
	virtual void read(short *, int) = 0;
	virtual int avail() = 0;

	ResamplerSNSF(int num_samples) : ring_bufferSNSF(num_samples << 1)
	{
	}

	virtual ~ResamplerSNSF()
	{
	}

	bool push(short *src, int num_samples)
	{
		if (this->max_write() < num_samples)
			return false;

		!num_samples || ring_bufferSNSF::push(reinterpret_cast<unsigned char *>(src), num_samples << 1);

		return true;
	}

	int max_write()
	{
		return this->space_empty () >> 1;
	}

	void resize(int num_samples)
	{
		ring_bufferSNSF::resize(num_samples << 1);
	}
};
