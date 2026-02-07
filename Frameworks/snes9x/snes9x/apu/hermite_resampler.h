/* Simple resampler based on bsnes's ruby audio library */

#pragma once

#include <cmath>
#include "resamplerSNSF.h"

class HermiteResampler : public ResamplerSNSF
{
protected:
	double r_step;
	double r_frac;
	int r_left[4], r_right[4];

	template<typename T1, typename T2> static T1 CLAMP(T1 x, T2 low, T2 high) { return x > high ? high : (x < low ? low : x); }
	template<typename T> static short SHORT_CLAMP(T n) { return static_cast<short>(CLAMP(n, -32768, 32767)); }

	double hermite(double mu1, double a, double b, double c, double d)
	{
		static const double tension = 0.0; //-1 = low, 0 = normal, 1 = high
		static const double bias = 0.0; //-1 = left, 0 = even, 1 = right

		double mu2, mu3, m0, m1, a0, a1, a2, a3;

		mu2 = mu1 * mu1;
		mu3 = mu2 * mu1;

		m0  = (b - a) * (1 + bias) * (1 - tension) / 2;
		m0 += (c - b) * (1 - bias) * (1 - tension) / 2;
		m1  = (c - b) * (1 + bias) * (1 - tension) / 2;
		m1 += (d - c) * (1 - bias) * (1 - tension) / 2;

		a0 = 2 * mu3 - 3 * mu2 + 1;
		a1 = mu3 - 2 * mu2 + mu1;
		a2 = mu3 - mu2;
		a3 = -2 * mu3 + 3 * mu2;

		return (a0 * b) + (a1 * m0) + (a2 * m1) + (a3 * c);
	}

public:
	HermiteResampler(int num_samples) : ResamplerSNSF(num_samples)
	{
		this->clear();
	}

	void time_ratio(double ratio)
	{
		this->r_step = ratio;
		this->clear();
	}

	void clear()
	{
		ring_bufferSNSF::clear();
		this->r_frac = 1.0;
		this->r_left[0] = this->r_left[1] = this->r_left[2] = this->r_left[3] = 0;
		this->r_right[0] = this->r_right[1] = this->r_right[2] = this->r_right[3] = 0;
	}

	void read(short *data, int num_samples)
	{
		int i_position = this->start >> 1;
		short *internal_buffer = reinterpret_cast<short *>(&this->buffer[0]);
		int o_position = 0;
		int consumed = 0;

		while (o_position < num_samples && consumed < this->buffer_size)
		{
			int s_left = internal_buffer[i_position];
			int s_right = internal_buffer[i_position + 1];
			int max_samples = this->buffer_size >> 1;
			static const double margin_of_error = 1.0e-10;

			if (std::abs(this->r_step - 1.0) < margin_of_error)
			{
				data[o_position] = static_cast<short>(s_left);
				data[o_position + 1] = static_cast<short>(s_right);

				o_position += 2;
				i_position += 2;
				if (i_position >= max_samples)
					i_position -= max_samples;
				consumed += 2;

				continue;
			}

			while (this->r_frac <= 1.0 && o_position < num_samples)
			{
				data[o_position] = SHORT_CLAMP(hermite(this->r_frac, this->r_left[0], this->r_left[1], this->r_left[2], this->r_left[3]));
				data[o_position + 1] = SHORT_CLAMP(hermite(this->r_frac, this->r_right[0], this->r_right[1], this->r_right[2], this->r_right[3]));

				o_position += 2;

				this->r_frac += this->r_step;
			}

			if (this->r_frac > 1.0)
			{
				this->r_left[0] = this->r_left[1];
				this->r_left[1] = this->r_left[2];
				this->r_left[2] = this->r_left[3];
				this->r_left[3] = s_left;

				this->r_right[0] = this->r_right[1];
				this->r_right[1] = this->r_right[2];
				this->r_right[2] = this->r_right[3];
				this->r_right[3] = s_right;

				this->r_frac -= 1.0;

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
		return static_cast<int>(std::floor(((this->size >> 2) - this->r_frac) / this->r_step) * 2);
	}
};
