/* Simple resampler based on bsnes's ruby audio library */

#pragma once

#include <cmath>
#include "resamplerSNSF.h"

class OsculatingResampler : public ResamplerSNSF
{
protected:
	double r_step;
	double r_frac;
	int r_left[6], r_right[6];

	template<typename T1, typename T2> static T1 CLAMP(T1 x, T2 low, T2 high) { return x > high ? high : (x < low ? low : x); }
	template<typename T> static short SHORT_CLAMP(T n) { return static_cast<short>(CLAMP(n, -32768, 32767)); }

	double osculating(double x, double a, double b, double c, double d, double e, double f)
	{
		double z = x - 0.5;
		double even1 = a + f, odd1 = a - f;
		double even2 = b + e, odd2 = b - e;
		double even3 = c + d, odd3 = c - d;
		double c0 = 0.01171875 * even1 - 0.09765625 * even2 + 0.5859375 * even3;
		double c1 = 0.2109375 * odd2 - 281 / 192.0 * odd3 - 13 / 384.0 * odd1;
		double c2 = 0.40625 * even2 - 17 / 48.0 * even3 - 5 / 96.0 * even1;
		double c3 = 0.1875 * odd1 - 53 / 48.0 * odd2 + 2.375 * odd3;
		double c4 = 1 / 48.0*even1 - 0.0625 * even2 + 1 / 24.0 * even3;
		double c5 = 25 / 24.0 * odd2 - 25 / 12.0 * odd3 - 5 / 24.0 * odd1;
		return ((((c5 * z + c4) * z + c3) * z + c2) * z + c1) * z + c0;
	}

public:
	OsculatingResampler(int num_samples) : ResamplerSNSF(num_samples)
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
		this->r_left[0] = this->r_left[1] = this->r_left[2] = this->r_left[3] = this->r_left[4] = this->r_left[5] = 0;
		this->r_right[0] = this->r_right[1] = this->r_right[2] = this->r_right[3] = this->r_right[4] = this->r_right[5] = 0;
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
				data[o_position] = SHORT_CLAMP(osculating(this->r_frac, this->r_left[0], this->r_left[1], this->r_left[2], this->r_left[3], this->r_left[4], this->r_left[5]));
				data[o_position + 1] = SHORT_CLAMP(osculating(this->r_frac, this->r_right[0], this->r_right[1], this->r_right[2], this->r_right[3], this->r_right[4], this->r_right[5]));

				o_position += 2;

				this->r_frac += this->r_step;
			}

			if (this->r_frac > 1.0)
			{
				this->r_left[0] = this->r_left[1];
				this->r_left[1] = this->r_left[2];
				this->r_left[2] = this->r_left[3];
				this->r_left[3] = this->r_left[4];
				this->r_left[4] = this->r_left[5];
				this->r_left[5] = s_left;

				this->r_right[0] = this->r_right[1];
				this->r_right[1] = this->r_right[2];
				this->r_right[2] = this->r_right[3];
				this->r_right[3] = this->r_right[4];
				this->r_right[4] = this->r_right[5];
				this->r_right[5] = s_right;

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
