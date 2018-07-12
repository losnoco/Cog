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

INLINE static void do_mudl(usf_state_t * state, short* VD, short* VS, short* VT)
{

#ifdef ARCH_MIN_ARM_NEON

    uint16x4_t vs_low = vld1_u16((const uint16_t*)VS);
    uint16x4_t vs_high = vld1_u16((const uint16_t*)VS+4);
    uint16x4_t vt_low = vld1_u16((const uint16_t*)VT);
    uint16x4_t vt_high = vld1_u16((const uint16_t*)VT+4);
	int16x8_t zero = vdupq_n_s16(0);

    uint32x4_t lo = vmull_u16(vs_low, vt_low);
    uint32x4_t high = vmull_u16(vs_high, vt_high);
    uint16x8_t result = vcombine_u16(vshrn_n_u32(lo,16),vshrn_n_u32(high,16));
	vst1q_u16(VACC_L, result);
	vst1q_s16(VACC_M, zero);
	vst1q_s16(VACC_H, zero);
	vector_copy(VD, VACC_L);
	return;
	
#else

    register int i;

    for (i = 0; i < N; i++)
        VACC_L[i] = (unsigned short)(VS[i])*(unsigned short)(VT[i]) >> 16;
    for (i = 0; i < N; i++)
        VACC_M[i] = 0x0000;
    for (i = 0; i < N; i++)
        VACC_H[i] = 0x0000;
    vector_copy(VD, VACC_L); /* no possibilities to clamp */
    return;
#endif
}

static void VMUDL(usf_state_t * state, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, state->VR[vt], e);
    do_mudl(state, state->VR[vd], state->VR[vs], ST);
    return;
}
