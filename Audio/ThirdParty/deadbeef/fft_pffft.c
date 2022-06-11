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
#include "pffft.h"
#include <Accelerate/Accelerate.h>

static int _fft_size;
static float *_input;
static float *_output;
static float *_output_real;
static float *_output_imaginary;
static float *_work;
static float *_hamming;
static float *_sq_mags;

static PFFFT_Setup *_fft_setup;

static void
_init_buffers(int fft_size) {
	if(fft_size != _fft_size) {
		fft_free();

		_input = pffft_aligned_malloc(fft_size * 4 * sizeof(float));
		_hamming = pffft_aligned_malloc(fft_size * 2 * sizeof(float));
		_sq_mags = pffft_aligned_malloc(fft_size * sizeof(float));
		_output = pffft_aligned_malloc(fft_size * 4 * sizeof(float));
		_output_real = pffft_aligned_malloc(fft_size * 2 * sizeof(float));
		_output_imaginary = pffft_aligned_malloc(fft_size * 2 * sizeof(float));
		_work = pffft_aligned_malloc(fft_size * 4 * sizeof(float));

		bzero(_input, fft_size * 4 * sizeof(float));

		_fft_setup = pffft_new_setup(fft_size * 2, PFFFT_COMPLEX);
		vDSP_hamm_window(_hamming, fft_size * 2, 0);

		_fft_size = fft_size;
	}
}

void fft_calculate(const float *data, float *freq, int fft_size) {
	int dft_size = fft_size * 2;

	_init_buffers(fft_size);

	vDSP_vmul(data, 1, _hamming, 1, _input, 2, dft_size);

	pffft_transform_ordered(_fft_setup, _input, _output, _work, PFFFT_FORWARD);

	DSPSplitComplex split_complex = {
		.realp = _output_real,
		.imagp = _output_imaginary
	};
	vDSP_ctoz((const DSPComplex *)_output, 2, &split_complex, 1, dft_size);
	vDSP_zvmags(&split_complex, 1, _sq_mags, 1, fft_size);

	int sq_count = fft_size;
	vvsqrtf(_sq_mags, _sq_mags, &sq_count);

	float mult = 2.f / fft_size;
	vDSP_vsmul(_sq_mags, 1, &mult, freq, 1, fft_size);
}

void fft_free(void) {
	pffft_aligned_free(_input);
	pffft_aligned_free(_hamming);
	pffft_aligned_free(_sq_mags);
	pffft_aligned_free(_output);
	pffft_aligned_free(_output_real);
	pffft_aligned_free(_output_imaginary);
	if(_fft_setup != NULL) {
		pffft_destroy_setup(_fft_setup);
	}
	_input = NULL;
	_hamming = NULL;
	_sq_mags = NULL;
	_fft_setup = NULL;
	_output = NULL;
	_output_real = NULL;
	_output_imaginary = NULL;
}
