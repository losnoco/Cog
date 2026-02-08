/*****************************************************************************\
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
                This file is licensed under the Snes9x License.
   For further information, consult the LICENSE file in the root directory.
\*****************************************************************************/

#pragma once

#include <algorithm>
#include <limits>
#include <memory>
#include <cstdint>
#include <cmath>

inline bool fEqual(float a, float b) { return fabs(a - b) < 1e-6; }

class Resampler
{
public:
	int size;
	int buffer_size;
	int start;
	std::unique_ptr<int16_t[]> buffer;

	float r_step;
	float r_frac;
	int   r_left[4], r_right[4];

	static float hermite(float mu1, float a, float b, float c, float d)
	{
		float mu2 = mu1 * mu1;
		float mu3 = mu2 * mu1;

		float m0 = (c - a) * 0.5;
		float m1 = (d - b) * 0.5;

		float a0 = +2 * mu3 - 3 * mu2 + 1;
		float a1 = mu3 - 2 * mu2 + mu1;
		float a2 = mu3 - mu2;
		float a3 = -2 * mu3 + 3 * mu2;

		return (a0 * b) + (a1 * m0) + (a2 * m1) + (a3 * c);
	}

	Resampler(int num_samples)
	{
		this->buffer_size = num_samples;
		this->buffer.reset(new int16_t[this->buffer_size]);
		this->r_step = 1.0;
		this->clear();
	}

	void time_ratio(double ratio)
	{
		this->r_step = ratio;
	}

	void clear()
	{
		this->start = 0;
		this->size = 0;
		std::fill_n(&this->buffer[0], this->buffer_size, 0);

		this->r_frac = 0.0;
		this->r_left[0] = this->r_left[1] = this->r_left[2] = this->r_left[3] = 0;
		this->r_right[0] = this->r_right[1] = this->r_right[2] = this->r_right[3] = 0;
	}

	bool pull(int16_t *dst, int num_samples)
	{
		if (this->space_filled() < num_samples)
			return false;

		std::copy_n(&this->buffer[start], std::min(num_samples, this->buffer_size - this->start), &dst[0]);

		if (num_samples > this->buffer_size - this->start)
			std::copy_n(&this->buffer[0], num_samples - (this->buffer_size - this->start), &dst[this->buffer_size - this->start]);

		this->start = (this->start + num_samples) % this->buffer_size;
		this->size -= num_samples;

		return true;
	}

	void push_sample(int16_t l, int16_t r)
	{
		if (this->space_empty() >= 2)
		{
			int end = this->start + this->size;
			if (end >= this->buffer_size)
				end -= this->buffer_size;
			this->buffer[end] = l;
			this->buffer[end + 1] = r;
			this->size += 2;
		}
	}

	bool push(int16_t *src, int num_samples)
	{
		if (this->space_empty() < num_samples)
			return false;

		int end = this->start + this->size;
		if (end > this->buffer_size)
			end -= this->buffer_size;
		int first_write_size = std::min(num_samples, this->buffer_size - end);

		std::copy_n(&src[0], first_write_size, &this->buffer[end]);

		if (num_samples > first_write_size)
			std::copy_n(&src[first_write_size], num_samples - first_write_size, &this->buffer[0]);

		this->size += num_samples;

		return true;
	}

	void read(int16_t *data, int num_samples)
	{
		// If we are outputting the exact same ratio as the input, pull directly from the input buffer
		if (fEqual(this->r_step, 1.0f))
		{
			this->pull(data, num_samples);
			return;
		}

		int o_position = 0;

		while (o_position < num_samples && this->size > 0)
		{
			int s_left = this->buffer[start];
			int s_right = this->buffer[start + 1];
			int hermite_val[2];

			while (this->r_frac <= 1.0 && o_position < num_samples)
			{
				hermite_val[0] = static_cast<int>(hermite(this->r_frac, this->r_left[0], this->r_left[1], this->r_left[2], this->r_left[3]));
				hermite_val[1] = static_cast<int>(hermite(this->r_frac, this->r_right[0], this->r_right[1], this->r_right[2], this->r_right[3]));
				data[o_position] = static_cast<int16_t>(std::clamp<int>(hermite_val[0], std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max()));
				data[o_position + 1] = static_cast<int16_t>(std::clamp<int>(hermite_val[1], std::numeric_limits<int16_t>::min(), std::numeric_limits<int16_t>::max()));

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

				--this->r_frac;

				this->start += 2;
				if (this->start >= this->buffer_size)
					this->start -= this->buffer_size;
				this->size -= 2;
			}
		}
	}

	int space_empty() const
	{
		return this->buffer_size - this->size;
	}

	int space_filled() const
	{
		return this->size;
	}

	int avail()
	{
		// If we are outputting the exact same ratio as the input, find out directly from the input buffer
		if (fEqual(this->r_step, 1.0f))
			return this->size;

		return static_cast<int>(std::trunc(((this->size >> 1) - this->r_frac) / this->r_step)) * 2;
	}

	void resize(int num_samples)
	{
		this->buffer_size = num_samples;
		this->buffer.reset(new int16_t[this->buffer_size]);
		this->clear();
	}
};
