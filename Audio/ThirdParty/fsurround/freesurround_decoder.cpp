/*
Copyright (C) 2007-2010 Christian Kothe

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "freesurround_decoder.h"
#include "channelmaps.h"
#include <Accelerate/Accelerate.h>
#include <cmath>
#include <vector>
#pragma warning(disable : 4244)

#define pi _pi
const float _pi = 3.141592654f;
const float epsilon = 0.000001f;
using namespace std;

#undef min
#undef max

static void *_memalign_malloc(size_t size, size_t align) {
	void *ret = NULL;
	if(posix_memalign(&ret, align, size) != 0) {
		return NULL;
	}
	return ret;
}

static void _dsp_complexalloc(DSPDoubleSplitComplex *cpx, int count) {
	cpx->realp = (double *)_memalign_malloc(count * sizeof(double), 16);
	cpx->imagp = (double *)_memalign_malloc(count * sizeof(double), 16);
}

static void _dsp_complexfree(DSPDoubleSplitComplex *cpx) {
	free(cpx->realp);
	free(cpx->imagp);
}

// FreeSurround implementation
class decoder_impl {
	public:
	// instantiate the decoder with a given channel setup and processing block size (in samples)
	decoder_impl(channel_setup setup, unsigned N)
	: N(N),
	  wnd(N), inbuf(3 * N), setup(setup), C((unsigned)chn_alloc[setup].size()),
	  buffer_empty(true), lt(N), rt(N), dst(N), dstf(N),
	  dftsetupF(vDSP_DFT_zrop_CreateSetupD(0, N, vDSP_DFT_FORWARD)),
	  dftsetupB(vDSP_DFT_zrop_CreateSetupD(0, N, vDSP_DFT_INVERSE)) {
		_dsp_complexalloc(&lf, N/2 + 1);
		_dsp_complexalloc(&rf, N/2 + 1);

		// allocate per-channel buffers
		outbuf.resize((N + N / 2) * C);
		signal.resize(C);
		for(unsigned k = 0; k < C; k++)
			_dsp_complexalloc(&signal[k], N/2 + 1);

		// init the window function
		for(unsigned k = 0; k < N; k++)
			wnd[k] = sqrt(0.5 * (1 - cos(2 * pi * k / N)) / N);

		// set default parameters
		set_circular_wrap(90);
		set_shift(0);
		set_depth(1);
		set_focus(0);
		set_center_image(1);
		set_front_separation(1);
		set_rear_separation(1);
		set_low_cutoff(40.0 / 22050);
		set_high_cutoff(90.0 / 22050);
		set_bass_redirection(false);

		flush();
	}

	~decoder_impl() {
		_dsp_complexfree(&lf);
		_dsp_complexfree(&rf);

		for(unsigned k = 0; k < C; k++)
			_dsp_complexfree(&signal[k]);

		vDSP_DFT_DestroySetupD(dftsetupF);
		vDSP_DFT_DestroySetupD(dftsetupB);
	}

	// decode a stereo chunk, produces a multichannel chunk of the same size (lagged)
	float *decode(const float *input) {
		// append incoming data to the end of the input buffer
		memcpy(&inbuf[N], &input[0], 8 * N);
		// process first and second half, overlapped
		buffered_decode(&inbuf[0]);
		buffered_decode(&inbuf[N]);
		// shift last half of the input to the beginning (for overlapping with a future block)
		memcpy(&inbuf[0], &inbuf[2 * N], 4 * N);
		buffer_empty = false;
		return &outbuf[0];
	}

	// flush the internal buffers
	void flush() {
		memset(&outbuf[0], 0, outbuf.size() * 4);
		memset(&inbuf[0], 0, inbuf.size() * 4);
		buffer_empty = true;
	}

	// number of samples currently held in the buffer
	unsigned buffered() {
		return buffer_empty ? 0 : N / 2;
	}

	// set soundfield & rendering parameters
	void set_circular_wrap(float v) {
		circular_wrap = v;
	}
	void set_shift(float v) {
		shift = v;
	}
	void set_depth(float v) {
		depth = v;
	}
	void set_focus(float v) {
		focus = v;
	}
	void set_center_image(float v) {
		center_image = v;
	}
	void set_front_separation(float v) {
		front_separation = v;
	}
	void set_rear_separation(float v) {
		rear_separation = v;
	}
	void set_low_cutoff(float v) {
		lo_cut = v * (N / 2);
	}
	void set_high_cutoff(float v) {
		hi_cut = v * (N / 2);
	}
	void set_bass_redirection(bool v) {
		use_lfe = v;
	}

	private:
	// helper functions
	static inline float sqr(double x) {
		return x * x;
	}
	static inline double amplitude(const DSPDoubleSplitComplex &cpx, size_t index) {
		return sqrt(sqr(cpx.realp[index]) + sqr(cpx.imagp[index]));
	}
	static inline double phase(const DSPDoubleSplitComplex &cpx, size_t index) {
		return atan2(cpx.imagp[index], cpx.realp[index]);
	}
	static inline void polar(double a, double p, DSPDoubleSplitComplex &cpx, size_t index) {
		cpx.realp[index] = a * cos(p);
		cpx.imagp[index] = a * sin(p);
	}
	static inline float min(double a, double b) {
		return a < b ? a : b;
	}
	static inline float max(double a, double b) {
		return a > b ? a : b;
	}
	static inline float clamp(double x) {
		return max(-1, min(1, x));
	}
	static inline float sign(double x) {
		return x < 0 ? -1 : (x > 0 ? 1 : 0);
	}
	// get the distance of the soundfield edge, along a given angle
	static inline double edgedistance(double a) {
		return min(sqrt(1 + sqr(tan(a))), sqrt(1 + sqr(1 / tan(a))));
	}
	// get the index (and fractional offset!) in a piecewise-linear channel allocation grid
	int map_to_grid(double &x) {
		double gp = ((x + 1) * 0.5) * (grid_res - 1), i = min(grid_res - 2, floor(gp));
		x = gp - i;
		return i;
	}

	// decode a block of data and overlap-add it into outbuf
	void buffered_decode(const float *input) {
		// demultiplex and apply window function
		vDSP_vspdp(input, 2, &lt[0], 1, N);
		vDSP_vspdp(input + 1, 2, &rt[0], 1, N);
		vDSP_vmulD(&lt[0], 1, &wnd[0], 1, &lt[0], 1, N);
		vDSP_vmulD(&rt[0], 1, &wnd[0], 1, &rt[0], 1, N);

		// map into spectral domain
		vDSP_ctozD((DSPDoubleComplex *)(&lt[0]), 2, &lf, 1, N / 2);
		vDSP_ctozD((DSPDoubleComplex *)(&rt[0]), 2, &rf, 1, N / 2);

		vDSP_DFT_ExecuteD(dftsetupF, lf.realp, lf.imagp, lf.realp, lf.imagp);
		vDSP_DFT_ExecuteD(dftsetupF, rf.realp, rf.imagp, rf.realp, rf.imagp);

		for(unsigned c = 0; c < C; c++) {
			signal[c].realp[0] = 0;
			signal[c].imagp[0] = 0;
			signal[c].realp[N/2] = 0;
			signal[c].imagp[N/2] = 0;
		}

		bzero(signal[C - 1].realp, sizeof(double) * (N / 2 + 1));
		bzero(signal[C - 1].imagp, sizeof(double) * (N / 2 + 1));

		// compute multichannel output signal in the spectral domain
		for(unsigned f = 1; f < N / 2; f++) {
			// get Lt/Rt amplitudes & phases
			double ampL = amplitude(lf, f), ampR = amplitude(rf, f);
			double phaseL = phase(lf, f), phaseR = phase(rf, f);
			// calculate the amplitude & phase differences
			double ampDiff = clamp((ampL + ampR < epsilon) ? 0 : (ampR - ampL) / (ampR + ampL));
			double phaseDiff = abs(phaseL - phaseR);
			if(phaseDiff > pi) phaseDiff = 2 * pi - phaseDiff;

			// decode into x/y soundfield position
			double x, y;
			transform_decode(ampDiff, phaseDiff, x, y);
			// add wrap control
			transform_circular_wrap(x, y, circular_wrap);
			// add shift control
			y = clamp(y - shift);
			// add depth control
			y = clamp(1 - (1 - y) * depth);
			// add focus control
			transform_focus(x, y, focus);
			// add crossfeed control
			x = clamp(x * (front_separation * (1 + y) / 2 + rear_separation * (1 - y) / 2));

			// get total signal amplitude
			double amp_total = sqrt(ampL * ampL + ampR * ampR);
			// and total L/C/R signal phases
			double phase_of[] = { phaseL, atan2(lf.imagp[f] + rf.imagp[f], lf.realp[f] + rf.realp[f]), phaseR };
			// compute 2d channel map indexes p/q and update x/y to fractional offsets in the map grid
			int p = map_to_grid(x), q = map_to_grid(y);
			// map position to channel volumes
			for(unsigned c = 0; c < C - 1; c++) {
				// look up channel map at respective position (with bilinear interpolation) and build the signal
				const vector<float *> &a = chn_alloc[setup][c];
				polar(amp_total * ((1 - x) * (1 - y) * a[q][p] + x * (1 - y) * a[q][p + 1] + (1 - x) * y * a[q + 1][p] + x * y * a[q + 1][p + 1]),
				      phase_of[1 + (int)sign(chn_xsf[setup][c])], signal[c], f);
			}

			// optionally redirect bass
			if(use_lfe && f < hi_cut) {
				// level of LFE channel according to normalized frequency
				double lfe_level = f < lo_cut ? 1 : 0.5 * (1 + cos(pi * (f - lo_cut) / (hi_cut - lo_cut)));
				// assign LFE channel
				polar(amp_total, phase_of[1], signal[C - 1], f);
				signal[C - 1].realp[f] *= lfe_level;
				signal[C - 1].imagp[f] *= lfe_level;
				// subtract the signal from the other channels
				for(unsigned c = 0; c < C - 1; c++) {
					signal[c].realp[f] *= (1 - lfe_level);
					signal[c].imagp[f] *= (1 - lfe_level);
				}
			}
		}

		// shift the last 2/3 to the first 2/3 of the output buffer
		memmove(&outbuf[0], &outbuf[C * N / 2], N * C * 4);
		// and clear the rest
		memset(&outbuf[C * N], 0, C * 4 * N / 2);
		// backtransform each channel and overlap-add
		for(unsigned c = 0; c < C; c++) {
			// back-transform into time domain
			vDSP_DFT_ExecuteD(dftsetupB, signal[c].realp, signal[c].imagp, signal[c].realp, signal[c].imagp);
			vDSP_ztocD(&signal[c], 1, (DSPDoubleComplex *)(&dst[0]), 2, N / 2);
			// add the result to the last 2/3 of the output buffer, windowed (and remultiplex)
			vDSP_vmulD(&dst[0], 1, &wnd[0], 1, &dst[0], 1, N);
			vDSP_vdpsp(&dst[0], 1, &dstf[0], 1, N);
			vDSP_vadd(&outbuf[C * N / 2 + c], C, &dstf[0], 1, &outbuf[C * N / 2 + c], C, N);
		}
	}

	// transform amp/phase difference space into x/y soundfield space
	void transform_decode(double a, double p, double &x, double &y) {
		x = clamp(1.0047 * a + 0.46804 * a * p * p * p - 0.2042 * a * p * p * p * p + 0.0080586 * a * p * p * p * p * p * p * p - 0.0001526 * a * p * p * p * p * p * p * p * p * p * p - 0.073512 * a * a * a * p - 0.2499 * a * a * a * p * p * p * p + 0.016932 * a * a * a * p * p * p * p * p * p * p - 0.00027707 * a * a * a * p * p * p * p * p * p * p * p * p * p + 0.048105 * a * a * a * a * a * p * p * p * p * p * p * p - 0.0065947 * a * a * a * a * a * p * p * p * p * p * p * p * p * p * p + 0.0016006 * a * a * a * a * a * p * p * p * p * p * p * p * p * p * p * p - 0.0071132 * a * a * a * a * a * a * a * p * p * p * p * p * p * p * p * p + 0.0022336 * a * a * a * a * a * a * a * p * p * p * p * p * p * p * p * p * p * p - 0.0004804 * a * a * a * a * a * a * a * p * p * p * p * p * p * p * p * p * p * p * p);
		y = clamp(0.98592 - 0.62237 * p + 0.077875 * p * p - 0.0026929 * p * p * p * p * p + 0.4971 * a * a * p - 0.00032124 * a * a * p * p * p * p * p * p + 9.2491e-006 * a * a * a * a * p * p * p * p * p * p * p * p * p * p + 0.051549 * a * a * a * a * a * a * a * a + 1.0727e-014 * a * a * a * a * a * a * a * a * a * a);
	}

	// apply a circular_wrap transformation to some position
	void transform_circular_wrap(double &x, double &y, double refangle) {
		if(refangle == 90)
			return;
		refangle = refangle * pi / 180;
		double baseangle = 90 * pi / 180;
		// translate into edge-normalized polar coordinates
		double ang = atan2(x, y), len = sqrt(x * x + y * y);
		len = len / edgedistance(ang);
		// apply circular_wrap transform
		if(abs(ang) < baseangle / 2)
			// angle falls within the front region (to be enlarged)
			ang *= refangle / baseangle;
		else
			// angle falls within the rear region (to be shrunken)
			ang = pi - (-(((refangle - 2 * pi) * (pi - abs(ang)) * sign(ang)) / (2 * pi - baseangle)));
		// translate back into soundfield position
		len = len * edgedistance(ang);
		x = clamp(sin(ang) * len);
		y = clamp(cos(ang) * len);
	}

	// apply a focus transformation to some position
	void transform_focus(double &x, double &y, double focus) {
		if(focus == 0)
			return;
		// translate into edge-normalized polar coordinates
		double ang = atan2(x, y), len = clamp(sqrt(x * x + y * y) / edgedistance(ang));
		// apply focus
		len = focus > 0 ? 1 - pow(1 - len, 1 + focus * 20) : pow(len, 1 - focus * 20);
		// back-transform into euclidian soundfield position
		len = len * edgedistance(ang);
		x = clamp(sin(ang) * len);
		y = clamp(cos(ang) * len);
	}

	// constants
	unsigned N, C; // number of samples per input/output block, number of output channels
	channel_setup setup; // the channel setup

	// parameters
	float circular_wrap; // angle of the front soundstage around the listener (90�=default)
	float shift; // forward/backward offset of the soundstage
	float depth; // backward extension of the soundstage
	float focus; // localization of the sound events
	float center_image; // presence of the center speaker
	float front_separation; // front stereo separation
	float rear_separation; // rear stereo separation
	float lo_cut, hi_cut; // LFE cutoff frequencies
	bool use_lfe; // whether to use the LFE channel

	// FFT data structures
	vector<double> lt, rt, dst; // left total, right total (source arrays), time-domain destination buffer array
	vector<float> dstf; // float conversion destination array
	DSPDoubleSplitComplex lf, rf; // left total / right total in frequency domain
	vDSP_DFT_SetupD dftsetupF, dftsetupB; // FFT objects

	// buffers
	bool buffer_empty; // whether the buffer is currently empty or dirty
	vector<float> inbuf; // stereo input buffer (multiplexed)
	vector<float> outbuf; // multichannel output buffer (multiplexed)
	vector<double> wnd; // the window function, precomputed
	vector<DSPDoubleSplitComplex> signal; // the signal to be constructed in every channel, in the frequency domain
};

// implementation of the shell class
freesurround_decoder::freesurround_decoder(channel_setup setup, unsigned blocksize)
: impl(new decoder_impl(setup, blocksize)) {
}
freesurround_decoder::~freesurround_decoder() {
	delete impl;
}
float *freesurround_decoder::decode(const float *input) {
	return impl->decode(input);
}
void freesurround_decoder::flush() {
	impl->flush();
}
void freesurround_decoder::circular_wrap(float v) {
	impl->set_circular_wrap(v);
}
void freesurround_decoder::shift(float v) {
	impl->set_shift(v);
}
void freesurround_decoder::depth(float v) {
	impl->set_depth(v);
}
void freesurround_decoder::focus(float v) {
	impl->set_focus(v);
}
void freesurround_decoder::center_image(float v) {
	impl->set_center_image(v);
}
void freesurround_decoder::front_separation(float v) {
	impl->set_front_separation(v);
}
void freesurround_decoder::rear_separation(float v) {
	impl->set_rear_separation(v);
}
void freesurround_decoder::low_cutoff(float v) {
	impl->set_low_cutoff(v);
}
void freesurround_decoder::high_cutoff(float v) {
	impl->set_high_cutoff(v);
}
void freesurround_decoder::bass_redirection(bool v) {
	impl->set_bass_redirection(v);
}
unsigned freesurround_decoder::buffered() {
	return impl->buffered();
}
unsigned freesurround_decoder::num_channels(channel_setup s) {
	return (unsigned)chn_id[s].size();
}
channel_id freesurround_decoder::channel_at(channel_setup s, unsigned i) {
	return i < chn_id[s].size() ? chn_id[s][i] : ci_none;
}
