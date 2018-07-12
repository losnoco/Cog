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

#ifndef SEMIFRAC
/*
 * acc = VS * VT;
 * acc = acc + 0x8000; // rounding value
 * acc = acc << 1; // partial value shifting
 *
 * Wrong:  ACC(HI) = -((INT32)(acc) < 0)
 * Right:  ACC(HI) = -(SEMIFRAC < 0)
 */

#define SEMIFRAC    (VS[i]*VT[i] + 0x4000)
#endif

INLINE static void do_mulf(usf_state_t * state, short* VD, short* VS, short* VT)
{

#ifdef ARCH_MIN_ARM_NEON

	int16x8_t vs,vt,res,four,zero,vacc_l, vacc_m, vacc_h;
	uint16x8_t cond_u, vacc_m_cond_u,one;
	   
	one = vdupq_n_u16(1);
    four = vdupq_n_s16(0x4000);
    zero = vdupq_n_s16(0);
   
    vs = vld1q_s16((const int16_t *)VS);
    vt = vld1q_s16((const int16_t *)VT);

    vacc_m = vqrdmulhq_s16(vs, vt);
    vacc_l = vmlaq_s16(four, vs,vt);
    vacc_l = vshlq_n_s16(vacc_l,1);
   
    cond_u = vceqq_s16(vs,vt);
    cond_u = vaddq_u16(cond_u, one);
    vacc_m_cond_u = vcltq_s16(vacc_m, zero);
    cond_u = vandq_u16(vacc_m_cond_u, cond_u);
    vacc_h = vqnegq_s16((int16x8_t)cond_u);

	vst1q_s16(VACC_L,vacc_l);
	vst1q_s16(VACC_M,vacc_m);
	vst1q_s16(VACC_H,vacc_h);
	
	SIGNED_CLAMP_AM(state, VD);
	return;
	
#else

    register int i;

    for (i = 0; i < N; i++)
        VACC_L[i] = (SEMIFRAC << 1) >>  0;
    for (i = 0; i < N; i++)
        VACC_M[i] = (SEMIFRAC << 1) >> 16;
    for (i = 0; i < N; i++)
        VACC_H[i] = -((VACC_M[i] < 0) & (VS[i] != VT[i])); /* -32768 * -32768 */
#if !defined ARCH_MIN_SSE2
    vector_copy(VD, VACC_M);
    for (i = 0; i < N; i++)
        VD[i] -= (VACC_M[i] < 0) & (VS[i] == VT[i]); /* ACC b 31 set, min*min */
#else
    SIGNED_CLAMP_AM(state, VD);
#endif
    return;
#endif
}

static void VMULF(usf_state_t * state, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, state->VR[vt], e);
    do_mulf(state, state->VR[vd], state->VR[vs], ST);
    return;
}
