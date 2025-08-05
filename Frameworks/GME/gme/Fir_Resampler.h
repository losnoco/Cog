// Finite impulse response (FIR) resampler with adjustable FIR size

// Game_Music_Emu https://bitbucket.org/mpyne/game-music-emu/
#ifndef FIR_RESAMPLER_H
#define FIR_RESAMPLER_H

#include "blargg_common.h"
#include <string.h>

class Fir_Resampler_ {
public:

	// Use Fir_Resampler<width> (below)

	// Set input/output resampling ratio and optionally low-pass rolloff and gain.
	// Returns actual ratio used (rounded to internal precision).
	double time_ratio( double factor, double rolloff = 0.999, double gain = 1.0 );

	// Current input/output ratio
	double ratio() const { return ratio_; }

// Input

	typedef short sample_t;

	// Resize and clear input buffer
	blargg_err_t buffer_size( int );

	// Clear input buffer. At least two output samples will be available after
	// two input samples are written.
	void clear();

	// Number of input samples that can be written
	int max_write() const { return buf.end() - write_pos; }

	// Pointer to place to write input samples
	sample_t* buffer() { return write_pos; }

	// Notify resampler that 'count' input samples have been written
	void write( long count );

	// Number of input samples in buffer
	int written() const { return write_pos - &buf [write_offset]; }

	// Skip 'count' input samples. Returns number of samples actually skipped.
	int skip_input( long count );

// Output

	// Number of extra input samples needed until 'count' output samples are available
	int input_needed( int32_t count ) const;

	// Number of output samples available
	int avail() const { return avail_( write_pos - &buf [width_ * stereo] ); }

public:
	~Fir_Resampler_();
protected:
	enum { stereo = 2 };
	enum { max_res = 32 };
	blargg_vector<sample_t> buf;
	sample_t* write_pos;
	int res;
	int imp_phase;
	int const width_;
	int const write_offset;
	uint32_t skip_bits;
	int step;
	int input_per_cycle;
	double ratio_;
	sample_t* impulses;

	Fir_Resampler_( int width, sample_t* );
	int avail_( int32_t input_count ) const;
};

// Width is number of points in FIR. Must be even and 4 or more. More points give
// better quality and rolloff effectiveness, and take longer to calculate.
template<int width>
class Fir_Resampler : public Fir_Resampler_ {
	static_assert( width >= 4 && width % 2 == 0, "FIR width must be even and have 4 or more points" );
	short impulses [max_res] [width];
public:
	Fir_Resampler() : Fir_Resampler_( width, impulses [0] ) { }

	// Read at most 'count' samples. Returns number of samples actually read.
	typedef short sample_t;
	int read( sample_t* out, int32_t count );
};

// End of public interface

inline void Fir_Resampler_::write( long count )
{
	write_pos += count;
	assert( write_pos <= buf.end() );
}

template<int width>
int Fir_Resampler<width>::read( sample_t* out_begin, int32_t count )
{
	sample_t* out = out_begin;
	const sample_t* in = buf.begin();
	sample_t* end_pos = write_pos;
	uint32_t skip = skip_bits >> imp_phase;
	sample_t const* imp = impulses [imp_phase];
	int remain = res - imp_phase;
	int const step = this->step;

	count >>= 1;

	// Resampling can add noise so don't actually do it if we've matched sample
	// rate
	const double ratio1 = ratio() - 1.0;
	const bool should_resample =
		( ratio1 >= 0 ? ratio1 : -ratio1 ) >= 0.00001;

	if ( end_pos - in >= width * stereo )
	{
		end_pos -= width * stereo;
		do
		{
			count--;
			if ( count < 0 )
				break;

			if( !should_resample )
			{
				out [0] = static_cast<sample_t>( in [0] );
				out [1] = static_cast<sample_t>( in [1] );
			}
			else
			{
				// accumulate in extended precision
				int32_t l = 0;
				int32_t r = 0;

				const sample_t* i = in;

				for ( int n = width / 2; n; --n )
				{
					int pt0 = imp [0];
					l += pt0 * i [0];
					r += pt0 * i [1];
					int pt1 = imp [1];
					imp += 2;
					l += pt1 * i [2];
					r += pt1 * i [3];
					i += 4;
				}

				remain--;

				l >>= 15;
				r >>= 15;

				in += (skip * stereo) & stereo;
				skip >>= 1;

				if ( !remain )
				{
					imp = impulses [0];
					skip = skip_bits;
					remain = res;
				}

				out [0] = (sample_t) l;
				out [1] = (sample_t) r;
			}

			in += step;
			out += 2;
		}
		while ( in <= end_pos );
	}

	imp_phase = res - remain;

	int left = write_pos - in;
	write_pos = &buf [left];
	memmove( buf.begin(), in, left * sizeof *in );

	return out - out_begin;
}

#endif
