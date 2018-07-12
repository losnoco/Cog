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

INLINE static void do_madl(usf_state_t * state, short* VD, short* VS, short* VT)
{

#ifdef ARCH_MIN_ARM_NEON

	uint16x8_t vs = vld1q_u16((const uint16_t*)VS);
	uint16x8_t vt = vld1q_u16((const uint16_t*)VT);
	uint16x8_t v_vaccl = vld1q_u16((const uint16_t*)VACC_L);
	uint16x8_t v_vaccm = vld1q_u16((const uint16_t*)VACC_M);
	uint16x8_t v_vacch = vld1q_u16((const uint16_t*)VACC_H);

	uint32x4_t zero = vdupq_n_u32(0);
	uint16x4_t vs_low =  vget_low_u16(vs);
    uint16x4_t vs_high = vget_high_u16(vs);
    uint16x4_t vt_low =  vget_low_u16(vt);
    uint16x4_t vt_high = vget_high_u16(vt);
	
	uint32x4_t product_L = vmulq_u32( vmovl_u16(vs_low), vmovl_u16(vt_low) );
	uint32x4_t product_H = vmulq_u32( vmovl_u16(vs_high), vmovl_u16(vt_high) );

	uint16x4_t addend_L = vaddhn_u32(product_L, zero);
	uint16x4_t addend_H = vaddhn_u32(product_H, zero);
	
	uint32x4_t exceed1 = vaddl_u16(addend_L, vget_low_u16(v_vaccl));
	uint32x4_t exceed2 = vaddl_u16(addend_H, vget_high_u16(v_vaccl));
	
	v_vaccl = vcombine_u16(vmovn_u32(exceed1), vmovn_u32(exceed2));
	
	addend_L = vaddhn_u32(exceed1, zero);
	addend_H = vaddhn_u32(exceed2, zero);
	
	exceed1 = vaddl_u16(addend_L, vget_low_u16(v_vaccm));
	exceed2 = vaddl_u16(addend_H, vget_high_u16(v_vaccm));
	
	v_vaccm = vcombine_u16(vmovn_u32(exceed1), vmovn_u32(exceed2));

	addend_L = vaddhn_u32(exceed1, zero);
	addend_H = vaddhn_u32(exceed2, zero);
	
	uint16x8_t v_vacch2 = vcombine_u16(addend_L, addend_H);
	v_vacch = vaddq_u16(v_vacch, v_vacch2);

	vst1q_s16(VACC_L, (int16x8_t)v_vaccl);
	vst1q_s16(VACC_M, (int16x8_t)v_vaccm);
	vst1q_s16(VACC_H, (int16x8_t)v_vacch);
	
	SIGNED_CLAMP_AL(state, VD);
	return;
	
#else


    ALIGNED int32_t product[N];
    ALIGNED uint32_t addend[N];
    register int i;

    for (i = 0; i < N; i++)
        product[i] = (unsigned short)(VS[i]) * (unsigned short)(VT[i]);
    for (i = 0; i < N; i++)
        addend[i] = (product[i] & 0x0000FFFF0000) >> 16;
    for (i = 0; i < N; i++)
        addend[i] = (unsigned short)(VACC_L[i]) + addend[i];
    for (i = 0; i < N; i++)
        VACC_L[i] = (short)(addend[i]);
    for (i = 0; i < N; i++)
        addend[i] = (unsigned short)(addend[i] >> 16);
    for (i = 0; i < N; i++)
        addend[i] = (unsigned short)(VACC_M[i]) + addend[i];
    for (i = 0; i < N; i++)
        VACC_M[i] = (short)(addend[i]);
    for (i = 0; i < N; i++)
        VACC_H[i] += addend[i] >> 16;
    SIGNED_CLAMP_AL(state, VD);
    return;
#endif
}

static void VMADL(usf_state_t * state, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, state->VR[vt], e);
    do_madl(state, state->VR[vd], state->VR[vs], ST);
    return;
}
