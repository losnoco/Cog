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

INLINE static void set_co(usf_state_t * state, short* VD, short* VS, short* VT)
{ /* set CARRY and carry out from sum */

#ifdef ARCH_MIN_ARM_NEON

	uint16x4_t vs_low = vld1_u16((const uint16_t*)VS);
    uint16x4_t vs_high = vld1_u16((const uint16_t*)VS+4);
    uint16x4_t vt_low = vld1_u16((const uint16_t*)VT);
    uint16x4_t vt_high = vld1_u16((const uint16_t*)VT+4);
    uint32x4_t zero = vdupq_n_u32(0);

    uint32x4_t v_l = vaddl_u16(vs_low, vt_low);
    uint32x4_t v_h = vaddl_u16(vs_high, vt_high);
    uint16x4_t vl16 = vaddhn_u32(v_l,zero);
    uint16x4_t vh16 = vaddhn_u32(v_h,zero);
    uint16x8_t vaccl = vcombine_u16(vmovn_u32(v_l),vmovn_u32(v_h));
    uint16x8_t co = vcombine_u16(vl16,vh16);
	
	vst1q_u16(VACC_L, vaccl);
	vector_copy(VD, VACC_L);
	vst1q_u16(state->ne, (uint16x8_t)zero);
	vst1q_u16(state->co, co);
	
	return;
#else

    ALIGNED int32_t sum[N];
    register int i;

    for (i = 0; i < N; i++)
        sum[i] = (unsigned short)(VS[i]) + (unsigned short)(VT[i]);
    for (i = 0; i < N; i++)
        VACC_L[i] = VS[i] + VT[i];
    vector_copy(VD, VACC_L);
    for (i = 0; i < N; i++)
        state->ne[i] = 0;
    for (i = 0; i < N; i++)
        state->co[i] = sum[i] >> 16; /* native:  (sum[i] > +65535) */
    return;

#endif
}

static void VADDC(usf_state_t * state, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, state->VR[vt], e);
    set_co(state, state->VR[vd], state->VR[vs], ST);
    return;
}
