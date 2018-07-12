/******************************************************************************\
* Authors:  Iconoclast                                                         *
* Release:  2013.10.11                                                         *
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

INLINE static void do_mudn(usf_state_t * state, short* VD, short* VS, short* VT)
{

#ifdef ARCH_MIN_ARM_NEON

	int16x8_t vd, vs,vt,res,four,vacc_l, vacc_m, vacc_h;
	
	int32x4_t zero = vdupq_n_s32(0);
	int16x8_t zero16 = vdupq_n_s16(0);

	uint16x4_t vs_low = vld1_u16((const uint16_t *)VS);
	uint16x4_t vs_high = vld1_u16((const uint16_t *)VS+4);
	int16x4_t vt_low = vld1_s16((const int16_t *)VT);
	int16x4_t vt_high = vld1_s16((const int16_t *)VT+4);
	 
    uint32x4_t l1 = vmovl_u16(vs_low);
	uint32x4_t h1 = vmovl_u16(vs_high);
	int32x4_t l2 = vmovl_s16(vt_low);
	int32x4_t h2 = vmovl_s16(vt_high);
	
    int32x4_t l = vmulq_s32((int32x4_t)l1,l2);
	int32x4_t h = vmulq_s32((int32x4_t)h1,h2);
    
	int16x8_t vaccl = vcombine_s16(vmovn_s32(l),vmovn_s32(h));
	int16x8_t vaccm = vcombine_s16(vaddhn_s32(l,zero),vaddhn_s32(h,zero));
	uint16x8_t uvacch = vcltq_s16(vaccm, zero16);
	vst1q_s16(VACC_L, vaccl);
	vst1q_s16(VACC_M, vaccm);
	vst1q_s16(VACC_H, (int16x8_t)uvacch);
	
	vector_copy(VD, VACC_L); /* no possibilities to clamp */
	
	return;
	
#else

    register int i;

    for (i = 0; i < N; i++)
        VACC_L[i] = ((unsigned short)(VS[i])*VT[i] & 0x00000000FFFF) >>  0;
    for (i = 0; i < N; i++)
        VACC_M[i] = ((unsigned short)(VS[i])*VT[i] & 0x0000FFFF0000) >> 16;
    for (i = 0; i < N; i++)
        VACC_H[i] = -(VACC_M[i] < 0);
    vector_copy(VD, VACC_L); /* no possibilities to clamp */
    return;
#endif
}

static void VMUDN(usf_state_t * state, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, state->VR[vt], e);
    do_mudn(state, state->VR[vd], state->VR[vs], ST);
    return;
}
