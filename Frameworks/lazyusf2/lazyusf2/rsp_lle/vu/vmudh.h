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

INLINE static void do_mudh(usf_state_t * state, short* VD, short* VS, short* VT)
{

#ifdef ARCH_MIN_ARM_NEON

	int16x8_t zero16 = vdupq_n_s16(0);
	int32x4_t zero32 = vdupq_n_s32(0);

	int16x4_t vs_low =  vld1_s16((const int16_t *)VS);
	int16x4_t vs_high = vld1_s16((const int16_t *)VS+4);
	int16x4_t vt_low =  vld1_s16((const int16_t *)VT);
	int16x4_t vt_high = vld1_s16((const int16_t *)VT+4);
	 
    int32x4_t l1 = vmovl_s16(vs_low);
	int32x4_t h1 = vmovl_s16(vs_high);
	int32x4_t l2 = vmovl_s16(vt_low);
	int32x4_t h2 = vmovl_s16(vt_high);
	
    int32x4_t l = vmulq_s32(l1,l2);
	int32x4_t h = vmulq_s32(h1,h2);
    
	int16x8_t vaccm = vcombine_s16(vmovn_s32(l),vmovn_s32(h));
	int16x8_t vacch = vcombine_s16(vaddhn_s32(l,zero32),vaddhn_s32(h,zero32));

	vst1q_s16(VACC_L, zero16);
	vst1q_s16(VACC_M, vaccm);
	vst1q_s16(VACC_H, vacch);
	
	SIGNED_CLAMP_AM(state, VD);
	return;
	
#else

    register int i;

    for (i = 0; i < N; i++)
        VACC_L[i] = 0x0000;
    for (i = 0; i < N; i++)
        VACC_M[i] = (short)(VS[i]*VT[i] >>  0);
    for (i = 0; i < N; i++)
        VACC_H[i] = (short)(VS[i]*VT[i] >> 16);
    SIGNED_CLAMP_AM(state, VD);
    return;
#endif
}

static void VMUDH(usf_state_t * state, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, state->VR[vt], e);
    do_mudh(state, state->VR[vd], state->VR[vs], ST);
    return;
}
