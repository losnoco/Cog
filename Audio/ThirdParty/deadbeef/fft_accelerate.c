/*
    DeaDBeeF -- the music player
    Copyright (C) 2009-2021 Alexey Yakovenko and other contributors

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#include "fft.h"
#include <Accelerate/Accelerate.h>

// Some newer spectrum calculation methodology, adapted but not copied wholesale
// Mostly about a dozen or two lines of Cocoa and vDSP code

// AudioSpectrum: A sample app using Audio Unit and vDSP
// By Keijiro Takahashi, 2013, 2014
// https://github.com/keijiro/AudioSpectrum

struct SpectrumData
{
	unsigned long length;

	vDSP_DFT_Setup _dftSetup;
	DSPSplitComplex _dftBuffer;
	Float32 *_window;

	// variable size
	Float32 data[0];
};

// Apparently _mm_malloc is Intel-only on newer macOS targets, so use supported posix_memalign
// malloc() is allegedly aligned on macOS, but I don't know for sure
static void *_memalign_calloc(size_t count, size_t size, size_t align) {
	size *= count;
	void *ret = NULL;
	if(posix_memalign(&ret, align, size) != 0) {
		return NULL;
	}
	bzero(ret, size);
	return ret;
}

static void
_init_buffers(void **st, int fft_size) {
	if(!st) return;
	struct SpectrumData *spec = (struct SpectrumData *) *st;
	if(!*st || fft_size != spec->length) {
		fft_free(st);

		*st = calloc(1, sizeof(*spec) + sizeof(Float32) * fft_size);
		if(!*st) return;
		spec = (struct SpectrumData *) *st;

		spec->_dftSetup = vDSP_DFT_zrop_CreateSetup(NULL, fft_size * 2, vDSP_DFT_FORWARD);
		if(!spec->_dftSetup) return;

		spec->_dftBuffer.realp = _memalign_calloc(fft_size, sizeof(Float32), 16);
		spec->_dftBuffer.imagp = _memalign_calloc(fft_size, sizeof(Float32), 16);
		if(!spec->_dftBuffer.realp || !spec->_dftBuffer.imagp) return;

		spec->_window = _memalign_calloc(fft_size * 2, sizeof(Float32), 16);
		if(!spec->_window) return;
		vDSP_blkman_window(spec->_window, fft_size * 2, 0);

		Float32 normFactor = 2.0f / (fft_size * 2);
		vDSP_vsmul(spec->_window, 1, &normFactor, spec->_window, 1, fft_size * 2);

		spec->length = fft_size;
	}
}

void fft_calculate(void **st, const float *data, float *freq, int fft_size) {
	if(!st || !freq || !fft_size) return;
	_init_buffers(st, fft_size);
	struct SpectrumData *spec = (struct SpectrumData *) *st;
	if(!spec || !spec->length || !data) {
		// Decibels
		float kZeroLevel = -128.0;
		vDSP_vfill(&kZeroLevel, freq, 1, fft_size);
		return;
	}

	// Split the waveform
	DSPSplitComplex dest = { spec->_dftBuffer.realp, spec->_dftBuffer.imagp };
	vDSP_ctoz((const DSPComplex*)data, 2, &dest, 1, fft_size);

	// Apply the window function
	vDSP_vmul(spec->_dftBuffer.realp, 1, spec->_window, 2, spec->_dftBuffer.realp, 1, fft_size);
	vDSP_vmul(spec->_dftBuffer.imagp, 1, spec->_window + 1, 2, spec->_dftBuffer.imagp, 1, fft_size);

	// DFT
	vDSP_DFT_Execute(spec->_dftSetup, spec->_dftBuffer.realp, spec->_dftBuffer.imagp, spec->_dftBuffer.realp, spec->_dftBuffer.imagp);

	// Zero out the Nyquist value
	spec->_dftBuffer.imagp[0] = 0.0;

	// Calculate power spectrum
	Float32 *rawSpectrum = spec->data;
	vDSP_zvmags(&spec->_dftBuffer, 1, rawSpectrum, 1, fft_size);

	// Add -128dB offset to avoid log(0)
	float kZeroOffset = 1.5849e-13;
	vDSP_vsadd(rawSpectrum, 1, &kZeroOffset, rawSpectrum, 1, fft_size);

	// Convert power to decibel
	float kZeroDB = 0.70710678118f; // 1/sqrt(2)
	vDSP_vdbcon(rawSpectrum, 1, &kZeroDB, rawSpectrum, 1, fft_size, 0);

	cblas_scopy(fft_size, rawSpectrum, 1, freq, 1);
}

void fft_free(void **st) {
	if(!st || !*st) return;

	struct SpectrumData *spec = (struct SpectrumData *) *st;
	free(spec->_dftBuffer.realp);
	free(spec->_dftBuffer.imagp);
	free(spec->_window);
	if(spec->_dftSetup != NULL) {
		vDSP_DFT_DestroySetup(spec->_dftSetup);
	}
	free(spec);

	*st = NULL;
}
