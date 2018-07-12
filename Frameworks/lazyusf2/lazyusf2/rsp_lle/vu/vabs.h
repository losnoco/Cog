/******************************************************************************\
* Authors:  Iconoclast                                                         *
* Release:  2013.11.26                                                         *
* License:  CC0 Public Domain Dedication                                       *
*                                                                              *
* To the extent possible under law, the author(s) have dedicated all copyright *
* and related and neighboring rights to this software to the public domain     *
* worldwide. This software is distributed without any warranty.                *
*                                                                              *
* You should have received a copy of the CC0 Public Domain Dedication along    *
* with this software.                                                          *
* If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.             *
\******************************************************************************/
#include "vu.h"

/*
 * -1:  VT *= -1, because VS < 0 // VT ^= -2 if even, or ^= -1, += 1
 *  0:  VT *=  0, because VS = 0 // VT ^= VT
 * +1:  VT *= +1, because VS > 0 // VT ^=  0
 *      VT ^= -1, "negate" -32768 as ~+32767 (corner case hack for N64 SP)
 */
INLINE static void do_abs(usf_state_t * state, short* VD, short* VS, short* VT)
{
    ALIGNED short neg[N], pos[N];
    ALIGNED short nez[N], cch[N]; /* corner case hack -- abs(-32768) == +32767 */
    ALIGNED short res[N];
    register int i;

    vector_copy(res, VT);

#ifdef ARCH_MIN_ARM_NEON
	
	int16x8_t vs = vld1q_s16((const int16_t*)VS);
    int16x8_t resi = vld1q_s16((const int16_t*)VT);
    int16x8_t zero = vdupq_n_s16(0);
    int16x8_t eightk = vdupq_n_s16(0x8000);
    int16x8_t one = vdupq_n_s16(1);

    uint16x8_t negi = vcltq_s16(vs,zero);
    int16x8_t posi = vaddq_s16((int16x8_t)negi,one);
    posi = vorrq_s16(posi,(int16x8_t)negi);
    resi = veorq_s16(resi,posi);
    uint16x8_t ccch = vcgeq_s16(resi,eightk);
    int16x8_t ch = vnegq_s16((int16x8_t)ccch);
    resi = vaddq_s16(resi, (int16x8_t)ch);
	
	vst1q_s16(VACC_L, resi);	
    vector_copy(VD, VACC_L);
	return;
	
#else
	
	
#ifndef ARCH_MIN_SSE2
#define MASK_XOR
#endif
    for (i = 0; i < N; i++)
        neg[i]  = (VS[i] <  0x0000);
    for (i = 0; i < N; i++)
        pos[i]  = (VS[i] >  0x0000);
    for (i = 0; i < N; i++)
        nez[i]  = 0;
#ifdef MASK_XOR
    for (i = 0; i < N; i++)
        neg[i]  = -neg[i];
    for (i = 0; i < N; i++)
        nez[i] += neg[i];
#else
    for (i = 0; i < N; i++)
        nez[i] -= neg[i];
#endif
    for (i = 0; i < N; i++)
        nez[i] += pos[i];
#ifdef MASK_XOR
    for (i = 0; i < N; i++)
        res[i] ^= nez[i];
    for (i = 0; i < N; i++)
        cch[i]  = (res[i] != -32768);
    for (i = 0; i < N; i++)
        res[i] += cch[i]; /* -(x) === (x ^ -1) + 1 */
#else
    for (i = 0; i < N; i++)
        res[i] *= nez[i];
    for (i = 0; i < N; i++)
        cch[i]  = (res[i] == -32768);
    for (i = 0; i < N; i++)
        res[i] -= cch[i];
#endif
    vector_copy(VACC_L, res);
    vector_copy(VD, VACC_L);
    return;
#endif
}

static void VABS(usf_state_t * state, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, state->VR[vt], e);
    do_abs(state, state->VR[vd], state->VR[vs], ST);
    return;
}
