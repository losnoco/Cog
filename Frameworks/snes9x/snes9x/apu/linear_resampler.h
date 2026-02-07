/* Simple fixed-point linear resampler by BearOso*/

#pragma once

#include "resamplerSNSF.h"

class LinearResampler : public ResamplerSNSF
{
	static const int f_prec = 15;
	static const uint32_t f__one = 1 << f_prec;

protected:
	uint32_t f__r_step;
	uint32_t f__inv_r_step;
	uint32_t f__r_frac;
	int r_left, r_right;

	static short lerp(uint32_t t, int a, short b) { return (((b - a) * t) >> f_prec) + a; }

public:
	LinearResampler(int num_samples) : ResamplerSNSF(num_samples)
	{
		this->f__r_frac = 0;
	}

	void time_ratio(double ratio)
	{
		if (fEqual(ratio, 0.0))
			ratio = 1.0;
		this->f__r_step = static_cast<uint32_t>(ratio * f__one);
		this->f__inv_r_step = static_cast<uint32_t>(f__one / ratio);
		this->clear();
	}

	void clear()
	{
		ring_bufferSNSF::clear();
		this->f__r_frac = 0;
		this->r_left = 0;
		this->r_right = 0;
	}

	void read(short *data, int num_samples)
	{
		int i_position = this->start >> 1;
		short *internal_buffer = reinterpret_cast<short *>(&this->buffer[0]);
		int o_position = 0;
		int consumed = 0;
		int max_samples = this->buffer_size >> 1;

		while (o_position < num_samples && consumed < this->buffer_size)
		{
			if (this->f__r_step == f__one)
			{
				data[o_position] = internal_buffer[i_position];
				data[o_position + 1] = internal_buffer[i_position + 1];

				o_position += 2;
				i_position += 2;
				if (i_position >= max_samples)
					i_position -= max_samples;
				consumed += 2;

				continue;
			}

			while (this->f__r_frac <= f__one && o_position < num_samples)
			{
				data[o_position] = lerp(this->f__r_frac, this->r_left, internal_buffer[i_position]);
				data[o_position + 1] = lerp(this->f__r_frac, this->r_right, internal_buffer[i_position + 1]);

				o_position += 2;

				this->f__r_frac += this->f__r_step;
			}

			if (this->f__r_frac > f__one)
			{
				this->f__r_frac -= f__one;
				this->r_left = internal_buffer[i_position];
				this->r_right = internal_buffer[i_position + 1];
				i_position += 2;
				if (i_position >= max_samples)
					i_position -= max_samples;
				consumed += 2;
			}
		}

		this->size -= consumed << 1;
		this->start += consumed << 1;
		if (this->start >= this->buffer_size)
			this->start -= this->buffer_size;
	}

	int avail()
	{
		return (((this->size >> 2) * this->f__inv_r_step) - ((this->f__r_frac * this->f__inv_r_step) >> f_prec)) >> (f_prec - 1);
	}
};
