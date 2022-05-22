//
//  cqt.c
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 5/21/22.
//

#include "cqt.h"

#include <stdlib.h>

#include <Accelerate/Accelerate.h>

struct CQTState {
	float m_minFreq;
	float m_maxFreq;
	int m_bins;
	float m_sampleFreq;
	// enum WindowFunction; // Always Hamm

	float m_Q;
	FFTSetup m_fftSetup;
	int m_fftLength;
	int m_fftLogLength;
	DSPSplitComplex m_fftLengthMatrix;
	DSPSplitComplex m_kernel;
	int m_K;
	DSPSplitComplex m_KVector;
};

static struct CQTState m_state = { 0 };

static void *malloc_aligned(size_t size) {
	void *ret = NULL;
	if(posix_memalign(&ret, 8, size)) {
		return NULL;
	}
	return ret;
}

static int dspsplit_alloc(DSPSplitComplex *out, size_t size) {
	size_t realSize = size * sizeof(float);
	out->realp = malloc_aligned(realSize);
	out->imagp = malloc_aligned(realSize);
	if(!out->realp || !out->imagp) return -1;
	bzero(out->realp, realSize);
	bzero(out->imagp, realSize);
	return 0;
}

static void dspsplit_free(DSPSplitComplex *in) {
	if(in->realp) free(in->realp);
	if(in->imagp) free(in->imagp);
	in->realp = NULL;
	in->imagp = NULL;
}

static int cqtstate_alloc(struct CQTState *state, float minFreq, float maxFreq, int binsPerOctave, float sampleFreq /*, enum WindowFunction */) {
	state->m_minFreq = minFreq;
	state->m_maxFreq = maxFreq;
	state->m_bins = binsPerOctave;
	state->m_sampleFreq = sampleFreq;
	/* state->m_windowFunction = func; */

	const int K = (int)(ceilf(binsPerOctave * log2f(maxFreq / minFreq)));
	state->m_K = K;
	float Q = state->m_Q = 1 / (powf(2.0, 1.0f / binsPerOctave) - 1.0);
	const int fftLogLength = state->m_fftLogLength = (int)(ceilf(log2f(Q * sampleFreq / minFreq)));
	const int fftLength = state->m_fftLength = (int)(powf(2.0, fftLogLength));

	if(dspsplit_alloc(&state->m_kernel, K * fftLength) < 0)
		return -1;

	state->m_fftSetup = vDSP_create_fftsetup(fftLogLength, FFT_RADIX2);
	if(!state->m_fftSetup)
		return -1;

	const int maxN = (int)(ceilf(Q * sampleFreq / minFreq));

	DSPSplitComplex window;
	if(dspsplit_alloc(&window, maxN) < 0)
		return -1;

	DSPSplitComplex exponents;
	if(dspsplit_alloc(&exponents, maxN) < 0) {
		dspsplit_free(&window);
		return -1;
	}

	for(int k = K; k > 0; --k) {
		const int N = (int)(ceilf(Q * sampleFreq / (minFreq * pow(2.0, (float)(k - 1) / binsPerOctave))));
		assert(N <= maxN);

		bzero(window.imagp, sizeof(float) * N);

		vDSP_hamm_window(window.realp, N, 0);

		for(int i = 0; i < N; ++i) {
			float inner = 2 * M_PI * Q * (float)(i) / (float)(N);
			exponents.realp[i] = inner;
			exponents.imagp[i] = inner;
		}

		vvcosf(exponents.realp, exponents.realp, &N);
		vvsinf(exponents.imagp, exponents.imagp, &N);

		const float nFloat = (float)(N);
		vDSP_vsdiv(exponents.realp, 1, &nFloat, exponents.realp, 1, N);
		vDSP_vsdiv(exponents.imagp, 1, &nFloat, exponents.imagp, 1, N);

		DSPSplitComplex complexPointer;
		complexPointer.realp = &state->m_kernel.realp[(k - 1) * fftLength];
		complexPointer.imagp = &state->m_kernel.imagp[(k - 1) * fftLength];

		vDSP_zvmul(&window, 1, &exponents, 1, &complexPointer, 1, N < fftLength ? N : fftLength, 1);
		vDSP_fft_zip(state->m_fftSetup, &complexPointer, 1, fftLogLength, FFT_FORWARD);
	}

	dspsplit_free(&window);
	dspsplit_free(&exponents);

	vDSP_mtrans(state->m_kernel.realp, 1, state->m_kernel.realp, 1, fftLength, K);
	vDSP_mtrans(state->m_kernel.imagp, 1, state->m_kernel.imagp, 1, fftLength, K);

	vDSP_zvconj(&state->m_kernel, 1, &state->m_kernel, 1, fftLength * K);

	const float lengthFloat = (float)(fftLength);
	vDSP_vsdiv(state->m_kernel.realp, 1, &lengthFloat, state->m_kernel.realp, 1, fftLength * K);
	vDSP_vsdiv(state->m_kernel.imagp, 1, &lengthFloat, state->m_kernel.imagp, 1, fftLength * K);

	if(dspsplit_alloc(&state->m_fftLengthMatrix, fftLength) < 0)
		return -1;
	if(dspsplit_alloc(&state->m_KVector, K) < 0)
		return -1;

	return 0;
}

static void cqtstate_free(struct CQTState *in) {
	if(in) {
		dspsplit_free(&in->m_KVector);
		dspsplit_free(&in->m_kernel);
		dspsplit_free(&in->m_fftLengthMatrix);

		if(in->m_fftSetup) vDSP_destroy_fftsetup(in->m_fftSetup);

		in->m_sampleFreq = 0.0;
	}
}

void cqt_calculate(const float *data, const float sampleRate, float *freq, int samples_in) {
	if(!sampleRate) {
		bzero(freq, 88 * sizeof(float));
		return;
	}
	if(sampleRate != m_state.m_sampleFreq) {
		cqtstate_free(&m_state);
		if(cqtstate_alloc(&m_state, 27.5f, 4434.92f, 12, sampleRate) < 0) {
			cqtstate_free(&m_state);
			bzero(freq, 88 * sizeof(float));
			return;
		}
	}

	const int fftLength = m_state.m_fftLength;
	const int K = m_state.m_K;

	if(samples_in < fftLength) {
		bzero(m_state.m_fftLengthMatrix.realp, sizeof(float) * fftLength);
	}

	cblas_scopy(samples_in > fftLength ? fftLength : samples_in, data, 1, m_state.m_fftLengthMatrix.realp, 1);
	bzero(m_state.m_fftLengthMatrix.imagp, sizeof(float) * fftLength);

	vDSP_fft_zip(m_state.m_fftSetup, &m_state.m_fftLengthMatrix, 1, m_state.m_fftLogLength, FFT_FORWARD);

	vDSP_zmmul(&m_state.m_fftLengthMatrix, 1, &m_state.m_kernel, 1, &m_state.m_KVector, 1, 1, K, fftLength);

	vDSP_zvmags(&m_state.m_KVector, 1, freq, 1, K);

	vvsqrtf(freq, freq, &K);
}

void cqt_free() {
	cqtstate_free(&m_state);
}
