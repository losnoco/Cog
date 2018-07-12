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

INLINE static void do_macu(usf_state_t * state, short* VD, short* VS, short* VT)
{

#ifdef ARCH_MIN_ARM_NEON
	
	uint16x8_t vs = vld1q_u16((const uint16_t*)VS);
	uint16x8_t vt = vld1q_u16((const uint16_t*)VT);
	uint16x4_t v_vaccl_low = vld1_u16((const uint16_t*)VACC_L);
	uint16x4_t v_vaccl_high = vld1_u16((const uint16_t*)VACC_L+4);
	uint16x4_t v_vaccm_low = vld1_u16((const uint16_t*)VACC_M);
	uint16x4_t v_vaccm_high = vld1_u16((const uint16_t*)VACC_M+4);
	uint16x4_t v_vacch_low = vld1_u16((const uint16_t*)VACC_H);
	uint16x4_t v_vacch_high = vld1_u16((const uint16_t*)VACC_H+4);
	int32x4_t zero = vdupq_n_s32(0);
	uint32x4_t zero_u = vdupq_n_u32(0);
	uint32x4_t highmask = vdupq_n_u32(0x0000ffff);

	uint16x4_t vs_low =  vget_low_u16(vs);
    uint16x4_t vs_high = vget_high_u16(vs);
    uint16x4_t vt_low =  vget_low_u16(vt);
    uint16x4_t vt_high = vget_high_u16(vt);
	
	int32x4_t product_L = vqdmlal_s16(zero, (int16x4_t)vs_low, (int16x4_t)vt_low);
	int32x4_t product_H = vqdmlal_s16(zero, (int16x4_t)vs_high, (int16x4_t)vt_high);
	uint32x4_t addend_L = vandq_u32(highmask, (uint32x4_t)product_L);
	uint32x4_t addend_H = vandq_u32(highmask, (uint32x4_t)product_H);

	addend_L = vaddw_u16((uint32x4_t)addend_L, v_vaccl_low);
	addend_H = vaddw_u16((uint32x4_t)addend_H, v_vaccl_high);
	
	uint16x8_t v_vaccl = vcombine_u16(vmovn_u32(addend_L), vmovn_u32(addend_H));


	addend_L = vaddl_u16( 
		vaddhn_u32((uint32x4_t)product_L, zero_u), 
		vaddhn_u32((uint32x4_t)addend_L, zero_u)		
		);

	addend_H = vaddl_u16( 
		vaddhn_u32((uint32x4_t)product_H, zero_u), 
		vaddhn_u32((uint32x4_t)addend_H, zero_u)		
		);

	addend_L = vaddw_u16(addend_L, v_vaccm_low);
	addend_H = vaddw_u16(addend_H, v_vaccm_high);

	uint16x8_t v_vaccm = vcombine_u16(vmovn_u32(addend_L), vmovn_u32(addend_H));

	//product_L = vshrq_n_s32(product_L, 1);
	//product_H = vshrq_n_s32(product_H, 1);

	uint32x4_t cond_L = vcltq_s32(product_L,zero);
	int32x4_t cond_L_s = vnegq_s32((int32x4_t)cond_L);
	uint32x4_t cond_H = vcltq_s32(product_H,zero);
	int32x4_t cond_H_s = vnegq_s32((int32x4_t)cond_H);
	
	v_vacch_low = vsub_u16(v_vacch_low, vmovn_u32((uint32x4_t)cond_L_s));
	v_vacch_high = vsub_u16(v_vacch_high, vmovn_u32((uint32x4_t)cond_H_s));
	
	v_vacch_low = vadd_u16(vshrn_n_u32(addend_L,16), v_vacch_low);
	v_vacch_high = vadd_u16(vshrn_n_u32(addend_H,16), v_vacch_high);

	uint16x8_t v_vacch = vcombine_u16(v_vacch_low, v_vacch_high);

	vst1q_s16(VACC_L, (int16x8_t)v_vaccl);
	vst1q_s16(VACC_M, (int16x8_t)v_vaccm);
	vst1q_s16(VACC_H, (int16x8_t)v_vacch);
		
	UNSIGNED_CLAMP(state, VD);
	return;
	
#else


    ALIGNED int32_t product[N];
    ALIGNED uint32_t addend[N];
    register int i;

    for (i = 0; i < N; i++)
        product[i] = VS[i] * VT[i];
    for (i = 0; i < N; i++)
        addend[i] = (product[i] << 1) & 0x00000000FFFF;
    for (i = 0; i < N; i++)
        addend[i] = (unsigned short)(VACC_L[i]) + addend[i];
    for (i = 0; i < N; i++)
        VACC_L[i] = (short)(addend[i]);
    for (i = 0; i < N; i++)
        addend[i] = (addend[i] >> 16) + (unsigned short)(product[i] >> 15);
    for (i = 0; i < N; i++)
        addend[i] = (unsigned short)(VACC_M[i]) + addend[i];
    for (i = 0; i < N; i++)
        VACC_M[i] = (short)(addend[i]);
    for (i = 0; i < N; i++)
        VACC_H[i] -= (product[i] < 0);
    for (i = 0; i < N; i++)
        VACC_H[i] += addend[i] >> 16;
    UNSIGNED_CLAMP(state, VD);
    return;
#endif
}

static void VMACU(usf_state_t * state, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, state->VR[vt], e);
    do_macu(state, state->VR[vd], state->VR[vs], ST);
    return;
}
