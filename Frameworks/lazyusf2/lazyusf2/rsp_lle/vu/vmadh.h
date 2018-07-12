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

INLINE static void do_madh(usf_state_t * state, short* VD, short* VS, short* VT)
{

#ifdef ARCH_MIN_ARM_NEON
	
	int16x8_t vs,vt, vaccm, vacch, vacc_h, vacc_m,prod_low,prod_high,one,cond;
	uint16x8_t cond_u,res;
	 	     
	one = vdupq_n_s16(1);
    vs = vld1q_s16((const int16_t *)VS);
    vt = vld1q_s16((const int16_t *)VT);
	vaccm = vld1q_s16((const int16_t *)VACC_M);
	vacch = vld1q_s16((const int16_t *)VACC_H);
		
	prod_low = vmulq_s16(vs,vt);
	vacc_m = vaddq_s16(vaccm,prod_low);

	prod_high = vqdmulhq_s16(vs,vt);
	prod_high = vshrq_n_s16(prod_high, 1);
		
	res = vqaddq_u16((uint16x8_t)vaccm, (uint16x8_t)prod_low);
	cond_u = vceqq_s16((int16x8_t)res,vacc_m);
	cond_u = vaddq_u16(cond_u, (uint16x8_t)one);

	vacc_h = vaddq_s16(prod_high, vacch);
	vacc_h = vaddq_s16((int16x8_t)cond_u, vacc_h);
	
	vst1q_s16(VACC_M, vacc_m);
	vst1q_s16(VACC_H, vacc_h);
	
	SIGNED_CLAMP_AM(state, VD);
	return;
	
#else

    ALIGNED int32_t product[N];
    ALIGNED uint32_t addend[N];
    register int i;

    for (i = 0; i < N; i++)
        product[i] = (signed short)(VS[i]) * (signed short)(VT[i]);
    for (i = 0; i < N; i++)
        addend[i] = (unsigned short)(VACC_M[i]) + (unsigned short)(product[i]);
    for (i = 0; i < N; i++)
        VACC_M[i] += (short)(VS[i] * VT[i]);
    for (i = 0; i < N; i++)
        VACC_H[i] += (addend[i] >> 16) + (product[i] >> 16);
    SIGNED_CLAMP_AM(state, VD);
    return;

#endif
}

static void VMADH(usf_state_t * state, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, state->VR[vt], e);
    do_madh(state, state->VR[vd], state->VR[vs], ST);
    return;
}
