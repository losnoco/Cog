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

INLINE static void do_madn(usf_state_t * state, short* VD, short* VS, short* VT)
{

#ifdef ARCH_MIN_ARM_NEON

	uint32x4_t zero = vdupq_n_u32(0);
	uint16x4_t vs_low = vld1_u16((const uint16_t*)VS);
    uint16x4_t vs_high = vld1_u16((const uint16_t*)VS+4);
    int16x4_t vt_low = vld1_s16((const int16_t*)VT);
    int16x4_t vt_high = vld1_s16((const int16_t*)VT+4);
	uint16x4_t vaccl_low = vld1_u16((const uint16_t*)VACC_L);
	uint16x4_t vaccl_high = vld1_u16((const uint16_t*)VACC_L+4);
	uint16x4_t vaccm_low = vld1_u16((const uint16_t*)VACC_M);
	uint16x4_t vaccm_high = vld1_u16((const uint16_t*)VACC_M+4);
	int16x4_t vacch_low = vld1_s16((const int16_t*)VACC_H);
	int16x4_t vacch_high = vld1_s16((const int16_t*)VACC_H+4);
	
	int32x4_t vaccl_l = vmlaq_s32((int32x4_t)vmovl_u16(vaccl_low),(int32x4_t)vmovl_u16(vs_low),vmovl_s16(vt_low));
	int32x4_t vaccl_h = vmlaq_s32((int32x4_t)vmovl_u16(vaccl_high),(int32x4_t)vmovl_u16(vs_high),vmovl_s16(vt_high));
	uint32x4_t vaccm_l = vaddq_u32(vmovl_u16(vaccm_low), (uint32x4_t)vshrq_n_s32(vaccl_l,16));
	uint32x4_t vaccm_h = vaddq_u32(vmovl_u16(vaccm_high),(uint32x4_t)vshrq_n_s32(vaccl_h,16));
	uint16x4_t vacch_l = vaddhn_u32(vaccm_l, zero);
	uint16x4_t vacch_h = vaddhn_u32(vaccm_h, zero);
	int16x4_t vacch_low2 = vadd_s16(vacch_low,(int16x4_t)vacch_l);
	int16x4_t vacch_high2 = vadd_s16(vacch_high,(int16x4_t)vacch_h);

	int16x8_t vaccl = vcombine_s16(vmovn_s32(vaccl_l),vmovn_s32(vaccl_h));
	uint16x8_t vaccm = vcombine_u16(vmovn_u32(vaccm_l),vmovn_u32(vaccm_h));
	int16x8_t vacch = vcombine_s16(vacch_low2,vacch_high2);

	vst1q_s16(VACC_L, vaccl);
	vst1q_s16(VACC_M, (int16x8_t)vaccm);
	vst1q_s16(VACC_H, vacch);
    SIGNED_CLAMP_AL(state, VD);
	return;
		
#else


    ALIGNED uint32_t addend[N];
    register int i;

    for (i = 0; i < N; i++)
        addend[i] = (unsigned short)(VACC_L[i]) + (unsigned short)(VS[i]*VT[i]);
    for (i = 0; i < N; i++)
        VACC_L[i] += (short)(VS[i] * VT[i]);
    for (i = 0; i < N; i++)
        addend[i] = (addend[i] >> 16) + ((unsigned short)(VS[i])*VT[i] >> 16);
    for (i = 0; i < N; i++)
        addend[i] = (unsigned short)(VACC_M[i]) + addend[i];
    for (i = 0; i < N; i++)
        VACC_M[i] = (short)addend[i];
    for (i = 0; i < N; i++)
        VACC_H[i] += addend[i] >> 16;
    SIGNED_CLAMP_AL(state, VD);
    return;
#endif
}

static void VMADN(usf_state_t * state, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, state->VR[vt], e);
    do_madn(state, state->VR[vd], state->VR[vs], ST);
    return;
}
