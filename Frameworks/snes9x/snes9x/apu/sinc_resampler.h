/* Simple resampler based on bsnes's ruby audio library */

#pragma once

#include <algorithm>
#define _USE_MATH_DEFINES
#include <cmath>
#include "resamplerSNSF.h"

#ifndef M_PI
const double M_PI = 3.14159265358979323846;
#endif

class SincResampler : public ResamplerSNSF
{
protected:
	static bool initializedLUTs;
	static const unsigned SINC_RESOLUTION = 8192;
	static const unsigned SINC_WIDTH = 8;
	static const unsigned SINC_SAMPLES = SINC_RESOLUTION * SINC_WIDTH;
	static double sinc_lut[SINC_SAMPLES + 1];

	double r_step;
	double r_frac;
	int r_left[SINC_WIDTH * 2], r_right[SINC_WIDTH * 2];

	template<typename T1, typename T2> static T1 CLAMP(T1 x, T2 low, T2 high) { return x > high ? high : (x < low ? low : x); }
	template<typename T> static short SHORT_CLAMP(T n) { return static_cast<short>(CLAMP(n, -32768, 32767)); }

	static inline double sinc(double x)
	{
		return fEqual(x, 0.0) ? 1.0 : std::sin(x * M_PI) / (x * M_PI);
	}

	double sinc(const int *data)
	{
		double kernel[SINC_WIDTH * 2], kernel_sum = 0.0;
		int i = SINC_WIDTH, shift = static_cast<int>(std::floor(this->r_frac * SINC_RESOLUTION));
		int step = this->r_step > 1.0 ? static_cast<int>(SINC_RESOLUTION / this->r_step) : SINC_RESOLUTION;
		int shift_adj = shift * step / SINC_RESOLUTION;
		for (; i >= -static_cast<int>(SINC_WIDTH - 1); --i)
		{
			int pos = i * step;
			kernel_sum += kernel[i + SINC_WIDTH - 1] = this->sinc_lut[std::abs(shift_adj - pos)];
		}
		double sum = 0.0;
		for (i = 0; i < static_cast<int>(SINC_WIDTH * 2); ++i)
			sum += data[i] * kernel[i];
		return sum / kernel_sum;
	}

public:
	SincResampler(int num_samples) : ResamplerSNSF(num_samples)
	{
		if (!this->initializedLUTs)
		{
			double dx = static_cast<double>(SINC_WIDTH) / SINC_SAMPLES, x = 0.0;
			for (unsigned i = 0; i <= SINC_SAMPLES; ++i, x += dx)
				this->sinc_lut[i] = std::abs(x) < SINC_WIDTH ? sinc(x) * sinc(x / SINC_WIDTH) : 0.0;
			this->initializedLUTs = true;
		}
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
		std::fill_n(&this->r_left[0], SINC_WIDTH * 2, 0);
		std::fill_n(&this->r_right[0], SINC_WIDTH * 2, 0);
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
				data[o_position] = SHORT_CLAMP(sinc(this->r_left));
				data[o_position + 1] = SHORT_CLAMP(sinc(this->r_right));

				o_position += 2;

				this->r_frac += this->r_step;
			}

			if (this->r_frac > 1.0)
			{
				std::copy_n(&this->r_left[1], SINC_WIDTH * 2 - 1, &this->r_left[0]);
				this->r_left[SINC_WIDTH * 2 - 1] = s_left;

				std::copy_n(&this->r_right[1], SINC_WIDTH * 2 - 1, &this->r_right[0]);
				this->r_right[SINC_WIDTH * 2 - 1] = s_right;

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
