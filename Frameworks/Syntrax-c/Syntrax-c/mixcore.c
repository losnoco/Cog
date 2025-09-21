/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (c) Reinier van Vliet - https://bitbucket.org/rhinoid/ */
/* Copyright (c) Christopher Snowhill */

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "mixcore.h"
#include "jaytrax.h"

#ifndef M_PI
#define M_PI 3.14159265
#endif

// ITP_[number of taps]_[input sample width]_[fractional precision]_[readable name]
#define ITP_T02_S16_I08_LINEAR(P, F) (P[0]+(((P[1]-P[0])*F) >> 8))
#define ITP_T03_S16_I15_QUADRA(P, F) (((((((((P[0] + P[2]) >> 1) - P[1]) * F) >> 16) + P[1]) - ((P[2] + P[0] + (P[0] << 1)) >> 2)) * F) >> 14) + P[0]
#define ITP_T04_S16_I08_CUBIC(P, F)  (P[1] + ((F*(P[2] - P[0] + ((F*(2 * P[0] - 5 * P[1] + 4 * P[2] - P[3] + ((F*(3 * (P[1] - P[2]) + P[3] - P[0]) + 0x80) >> 8)) + 0x80) >> 8)) + 0x100) >> 9))
#define ITP_T04_SXX_F01_CUBIC(P, F)  (P[1] + 0.5 * F*(P[2] - P[0] + F*(2.0 * P[0] - 5.0 * P[1] + 4.0 * P[2] - P[3] + F * (3.0 * (P[1] - P[2]) + P[3] - P[0]))))

//---------------------interpolators
#define GET_PT(x) buf[((pos + ((x)<<8)) & sizeMask)>>8]

static int32_t itpNone(int16_t* buf, int32_t pos, int32_t incr, int32_t sizeMask) {
	(void)buf;(void)pos;(void)incr;(void)sizeMask;
	return 0;
}

static int32_t itpNearest(int16_t* buf, int32_t pos, int32_t incr, int32_t sizeMask) {
	return GET_PT(0);
}

static int32_t itpLinear(int16_t* buf, int32_t pos, int32_t incr, int32_t sizeMask) {
	int32_t p[2];
	int32_t frac = pos & 0xFF;

	p[0] = GET_PT(0);
	p[1] = GET_PT(1);

	return ITP_T02_S16_I08_LINEAR(p, frac);
}

static int32_t itpQuad(int16_t* buf, int32_t pos, int32_t incr, int32_t sizeMask) {
	int32_t p[3];
	int32_t frac = (pos & 0xFF)<<7;

	p[0] = GET_PT(0);
	p[1] = GET_PT(1);
	p[2] = GET_PT(2);

	return ITP_T03_S16_I15_QUADRA(p, frac);
}

static int32_t itpCubic(int16_t* buf, int32_t pos, int32_t incr, int32_t sizeMask) {
	int32_t p[4];
	int32_t frac = pos & 0xFF;

	p[0] = GET_PT(0);
	p[1] = GET_PT(1);
	p[2] = GET_PT(2);
	p[3] = GET_PT(3);

	return ITP_T04_S16_I08_CUBIC(p, frac);
}

enum { radius = 2 };
static inline double lanczos(double d) {
	if(d == 0.) return 1.;
	if(fabs(d) > radius) return 0.;
	double dr = (d * M_PI) / radius;
	return sin(d) * sin(dr) / (d * dr);
}

static int32_t itpSinc(int16_t* buf, int32_t pos, int32_t incr, int32_t sizeMask) {
	float p[8];
	int32_t frac = pos & 0xFF;
	double ffrac = frac / 256.0;
	double fincr = incr / 256.0;
	double scale = 1. / fincr > 1. ? 1. : 1. / fincr;
	double density = 0.;
	double sample = 0.;
	double fpos = 3. + ffrac; // 3.5 is the center position

	int min = -radius / scale + fpos - 0.5;
	int max =  radius / scale + fpos + 0.5;

	if(min < 0) min = 0;
	if(max > 8) max = 8;

	p[0] = GET_PT(0);
	p[1] = GET_PT(1);
	p[2] = GET_PT(2);
	p[3] = GET_PT(3);
	p[4] = GET_PT(4);
	p[5] = GET_PT(5);
	p[6] = GET_PT(6);
	p[7] = GET_PT(7);

	for(int m = min; m < max; ++m) {
		double factor = lanczos( (m - fpos + 0.5) * scale );
		density += factor;
		sample += p[m] * factor;
	}
	if(density > 0.) sample /= density; // Normalize

	return (int32_t) sample;
}

Interpolator interps[INTERP_COUNT] = {
	{ITP_NONE,      0, &itpNone,    "None"},
	{ITP_NEAREST,   1, &itpNearest, "Nearest"},
	{ITP_LINEAR,    2, &itpLinear,  "Linear"},
	{ITP_QUADRATIC, 3, &itpQuad,    "Quadratic"},
	{ITP_CUBIC,     4, &itpCubic,   "Cubic"},
	{ITP_SINC,      8, &itpSinc,    "Sinc"},
	//{ITP_BLEP,     -1, &mixSynthNearest, &mixSampNearest, "BLEP"} //BLEP needs variable amount of taps
};

#if 0 // unused code
static void smpCpyFrw(int16_t* destination, const int16_t* source, int32_t num) {
	memcpy(destination, source, num * sizeof(int16_t));
}

static void smpCpyRev(int16_t* destination, const int16_t* source, int32_t num) {
	for (int i=0; i < num; i++) {
		destination[i] = source[num-1 - i];
	}
}
#endif

//---------------------API

uint8_t jaymix_setInterp(Interpolator** out, uint8_t id) {
	for (int8_t i = 0; i < INTERP_COUNT; i++) {
		if (interps[i].id == id) {
			*out = &interps[i];
			return 1;
		}
	}
	return 0;
}

void jaymix_mixCore(JT1Player* SELF, int32_t numSamples) {
	int32_t  tempBuf[MIXBUF_LEN];
	int32_t  ic, is, doneSmp;
	int32_t* outBuf = &SELF->tempBuf[0];
	int32_t  chanNr = SELF->subsong->nrofchans;

	assert(numSamples <= MIXBUF_LEN);
	memset(&outBuf[0], 0, numSamples * MIXBUF_NR * sizeof(int32_t));

	for (ic = 0; ic < chanNr; ic++) {
		JT1Voice* vc = &SELF->voices[ic];
		int32_t (*fItp) (int16_t* buf, int32_t pos, int32_t incr, int32_t sizeMask);
		int32_t loopLen = vc->endpoint - vc->looppoint;

		doneSmp = 0;
		if (vc->sampledata) {
			if (!vc->wavePtr) continue;
			if (vc->samplepos < 0) continue;
			fItp = SELF->itp->fItp;

			int tapPtr;
			int16_t tapArr[32] = {0};
			int32_t tapOfs = SELF->itp->numTaps;
			uint8_t tapDir = vc->freqOffset < 0 ? 1 : 0;
			int32_t tapPos = (vc->samplepos>>8) - (1 - (2 * (int8_t)tapDir)) * tapOfs;

			for(tapPtr = 0; tapPtr < SELF->itp->numTaps; tapPtr++) {
				if (tapDir) { //backwards
					if (tapPos < (vc->looppoint>>8)) {
						tapPos += 2;
						tapDir = 0;
					}
				} else { //forwards
					if (tapPos >= (vc->endpoint>>8)) {
						if (vc->loopflg) { //has loop
							if(vc->bidirecflg) { //bidi
								tapPos -= 2;
								tapDir = 1;
							} else { //straight
								tapPos -= (loopLen>>8);
							}
						}
					}
				}

				if(tapPos >= 0)
					tapArr[tapPtr] = vc->wavePtr[tapPos];

				if (tapDir) { //backwards
					tapPos--;
				} else { //forwards
					tapPos++;

					if (tapPos >= (vc->endpoint>>8)) {
						if (!vc->loopflg) { //oneshot
							break;
						}
					}
				}
			}

			while (doneSmp < numSamples) {
				int32_t tapPtr_ = (tapPtr - tapOfs) & 31;
				tempBuf[doneSmp++] = fItp(tapArr, (tapPtr_ << 8) + (vc->samplepos&0xFF), tapDir ? -vc->freqOffset : vc->freqOffset, (32 << 8) - 1);

				int32_t newPos = vc->samplepos + vc->freqOffset;
				int32_t samplesDone = (newPos >> 8) - (vc->samplepos >> 8);
				samplesDone = (samplesDone < 0) ? -samplesDone : samplesDone;
				vc->samplepos = newPos;

				while(samplesDone--) {
					tapArr[tapPtr] = vc->wavePtr[tapPos];
					tapPtr = (tapPtr + 1) & 31;

					if (tapDir) { //backwards
						tapPos--;

						if (tapPos < (vc->looppoint>>8)) {
							tapPos += 2;
							tapDir = 0;
						}
					} else { //forwards
						tapPos++;

						if (tapPos >= (vc->endpoint>>8)) {
							if (vc->loopflg) { //has loop
								if(vc->bidirecflg) { //bidi
									tapPos -= 2;
									tapDir = 1;
								} else { //straight
									tapPos -= (loopLen>>8);
								}
							} else { //oneshot
								break;
							}
						}
					}
				}

				if (vc->freqOffset < 0) { //backwards
					if (vc->samplepos < vc->looppoint) {
						vc->freqOffset *= -1;
						vc->samplepos  += vc->freqOffset;
					}
				} else { //forwards
					if (vc->samplepos >= vc->endpoint) {
						if (vc->loopflg) { //has loop
							if(vc->bidirecflg) { //bidi
								vc->freqOffset *= -1;
								vc->samplepos += vc->freqOffset;
							} else { //straight
								vc->samplepos -= loopLen;
							}
						} else { //oneshot
							vc->samplepos = -1;
							break;
						}
					}
				}
			}
		} else { //synth render
			int32_t nos;

			if (!vc->wavePtr) {
				//original replayer plays through an empty wave
				vc->synthPos += vc->freqOffset * numSamples;
				vc->synthPos &= vc->waveLength;
				continue;
			}
			fItp = SELF->itp->fItp;

			//loop unroll optimization
			nos = numSamples;
			if (nos & 1) {
				tempBuf[doneSmp++] = fItp(vc->wavePtr, vc->synthPos, vc->freqOffset, vc->waveLength);
				vc->synthPos += vc->freqOffset;
				vc->synthPos &= vc->waveLength;
				nos--;
			}
			for(is = 0; is < nos; is += 2) {
				tempBuf[doneSmp++] = fItp(vc->wavePtr, vc->synthPos, vc->freqOffset, vc->waveLength);
				vc->synthPos += vc->freqOffset;
				vc->synthPos &= vc->waveLength;
				tempBuf[doneSmp++] = fItp(vc->wavePtr, vc->synthPos, vc->freqOffset, vc->waveLength);
				vc->synthPos += vc->freqOffset;
				vc->synthPos &= vc->waveLength;
			}
		}

		for(is = 0; is < doneSmp; is++) {
			int32_t samp, off;

			samp = tempBuf[is];
			off  = is * MIXBUF_NR;
			outBuf[off + BUF_MAINL] += (samp * vc->gainMainL) >> 8;
			outBuf[off + BUF_MAINR] += (samp * vc->gainMainR) >> 8;
			outBuf[off + BUF_ECHOL] += (samp * vc->gainEchoL) >> 8;
			outBuf[off + BUF_ECHOR] += (samp * vc->gainEchoR) >> 8;
		}
	}
}
