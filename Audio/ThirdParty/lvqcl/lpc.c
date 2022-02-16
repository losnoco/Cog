/*
 * Copyright (c) 2013, 2018 lvqcl
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <memory.h>
#include <stdlib.h>
#include <stdbool.h>
#include "lpc.h"

static void apply_window(float *const data, const size_t data_len) {
#if 0
	if (0) // subtract the mean
	{
		double mean = 0;
		for(int i = 0; i < (int)data_len; i++)
			mean += data[i];
		mean /= data_len;

		for(int i = 0; i < (int)data_len; i++)
			data[i] -= (float)mean;
	}
#endif

	if(1) // Welch window
	{
		const float n2 = (data_len + 1) / 2.0f;
		for(int i = 0; i < (int)data_len; i++) {
			float k = (i + 1 - n2) / n2;
			data[data_len - 1 - i] *= 1.0f - k * k;
		}
	}
}

static float vorbis_lpc_from_data(float *data, float *lpci, int n, int m, double *aut, double *lpc) {
	double error;
	double epsilon;
	int i, j;

	/* autocorrelation, p+1 lag coefficients */
	j = m + 1;
	while(j--) {
		double d = 0; /* double needed for accumulator depth */
		for(i = j; i < n; i++) d += (double)data[i] * data[i - j];
		aut[j] = d;
	}

	/* Generate lpc coefficients from autocorr values */

	/* set our noise floor to about -100dB */
	error = aut[0] * (1. + 1e-10);
	epsilon = 1e-9 * aut[0] + 1e-10;

	for(i = 0; i < m; i++) {
		double r = -aut[i + 1];

		if(error < epsilon) {
			memset(lpc + i, 0, (m - i) * sizeof(*lpc));
			goto done;
		}

		/* Sum up this iteration's reflection coefficient; note that in
		   Vorbis we don't save it.  If anyone wants to recycle this code
		   and needs reflection coefficients, save the results of 'r' from
		   each iteration. */

		for(j = 0; j < i; j++) r -= lpc[j] * aut[i - j];
		r /= error;

		/* Update LPC coefficients and total error */

		lpc[i] = r;
		for(j = 0; j < i / 2; j++) {
			double tmp = lpc[j];

			lpc[j] += r * lpc[i - 1 - j];
			lpc[i - 1 - j] += r * tmp;
		}
		if(i & 1) lpc[j] += lpc[j] * r;

		error *= 1. - r * r;
	}

done:

	/* slightly damp the filter */
	{
		double g = .99;
		double damp = g;
		for(j = 0; j < m; j++) {
			lpc[j] *= damp;
			damp *= g;
		}
	}

	for(j = 0; j < m; j++) lpci[j] = (float)lpc[j];

	/* we need the error value to know how big an impulse to hit the
	   filter with later */

	return error;
}

static void vorbis_lpc_predict(float *coeff, float *prime, int m, float *data, long n, float *work) {
	/* in: coeff[0...m-1] LPC coefficients
	       prime[0...m-1] initial values (allocated size of n+m-1)
	  out: data[0...n-1] data samples */

	long i, j, o, p;
	float y;

	if(!prime)
		for(i = 0; i < m; i++)
			work[i] = 0.f;
	else
		for(i = 0; i < m; i++)
			work[i] = prime[i];

	for(i = 0; i < n; i++) {
		y = 0;
		o = i;
		p = m;
		for(j = 0; j < m; j++)
			y -= work[o++] * coeff[--p];

		data[i] = work[o] = y;
	}
}

void lpc_extrapolate2(float *const data, const size_t data_len, const int nch, const int lpc_order, const size_t extra_bkwd, const size_t extra_fwd, void **extrapolate_buffer, size_t *extrapolate_buffer_size) {
	const size_t tdata_size = sizeof(float) * (extra_bkwd + data_len + extra_fwd);
	const size_t aut_size = sizeof(double) * (lpc_order + 1);
	const size_t lpc_size = sizeof(double) * lpc_order;
	const size_t lpci_size = sizeof(float) * lpc_order;
	const size_t work_size = sizeof(float) * (extra_bkwd + lpc_order + extra_fwd);

	const size_t new_size = tdata_size + aut_size + lpc_size + lpci_size + work_size;

	if(new_size > *extrapolate_buffer_size) {
		*extrapolate_buffer = realloc(*extrapolate_buffer, new_size);
		*extrapolate_buffer_size = new_size;
	}

	float *tdata = (float *)(*extrapolate_buffer); // for 1 channel only

	double *aut = (double *)(*extrapolate_buffer + tdata_size);
	double *lpc = (double *)(*extrapolate_buffer + tdata_size + aut_size);
	float *lpci = (float *)(*extrapolate_buffer + tdata_size + aut_size + lpc_size);
	float *work = (float *)(*extrapolate_buffer + tdata_size + aut_size + lpc_size + lpci_size);

	for(int c = 0; c < nch; c++) {
		if(extra_bkwd) {
			for(int i = 0; i < (int)data_len; i++)
				tdata[data_len - 1 - i] = data[i * nch + c];
		} else {
			for(int i = 0; i < (int)data_len; i++)
				tdata[i] = data[i * nch + c];
		}

		apply_window(tdata, data_len);
		vorbis_lpc_from_data(tdata, lpci, (int)data_len, lpc_order, aut, lpc);

		// restore after apply_window
		if(extra_bkwd) {
			for(int i = 0; i < (int)data_len; i++)
				tdata[data_len - 1 - i] = data[i * nch + c];
		} else {
			for(int i = 0; i < (int)data_len; i++)
				tdata[i] = data[i * nch + c];
		}

		vorbis_lpc_predict(lpci, tdata + data_len - lpc_order, lpc_order, tdata + data_len, extra_fwd + extra_bkwd, work);

		if(extra_bkwd) {
			for(int i = 0; i < extra_bkwd; i++)
				data[(-i - 1) * nch + c] = tdata[data_len + i];
		} else {
			for(int i = 0; i < extra_fwd; i++)
				data[(i + data_len) * nch + c] = tdata[data_len + i];
		}
	}
}
