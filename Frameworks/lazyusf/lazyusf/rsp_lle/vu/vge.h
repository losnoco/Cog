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

INLINE static void do_ge(usf_state_t * state, short* VD, short* VS, short* VT)
{

    ALIGNED short ce[N];
    ALIGNED short eq[N];
    register int i;

#ifdef ARCH_MIN_ARM_NEON


	int16x8_t zero = vdupq_n_s16(0);
	int16x8_t one = vdupq_n_s16(1);
	
	int16x8_t vs = vld1q_s16((const int16_t *)VS);
    int16x8_t vt = vld1q_s16((const int16_t *)VT);
    int16x8_t v_ne = vld1q_s16((const int16_t *)state->ne);
    int16x8_t v_co = vld1q_s16((const int16_t *)state->co);
	
	uint16x8_t v_eq_u = vceqq_s16(vs,vt);
	int16x8_t v_eq_s = vnegq_s16((int16x8_t)v_eq_u);

	int16x8_t v_ce = vandq_s16(v_ne,v_co);
	v_ce = veorq_s16(v_ce,one);
	v_eq_s = vandq_s16(v_eq_s, v_ce);
	vst1q_s16(state->clip, zero);
	uint16x8_t v_comp = vcgtq_s16(vs, vt);
	int16x8_t v_comp_s = vnegq_s16((int16x8_t)v_comp);

	v_comp_s = vorrq_s16(v_comp_s, v_eq_s);
	vst1q_s16(state->comp, v_comp_s);
	
	merge(VACC_L, state->comp, VS, VT);
    vector_copy(VD, VACC_L);
	
	vst1q_s16(state->ne,zero);
	vst1q_s16(state->co,zero);
	
	return;
#else
	
    for (i = 0; i < N; i++)
        eq[i] = (VS[i] == VT[i]);
    for (i = 0; i < N; i++)
        ce[i] = (state->ne[i] & state->co[i]) ^ 1;
    for (i = 0; i < N; i++)
        eq[i] = eq[i] & ce[i];
    for (i = 0; i < N; i++)
        state->clip[i] = 0;
    for (i = 0; i < N; i++)
        state->comp[i] = (VS[i] > VT[i]); /* greater than */
    for (i = 0; i < N; i++)
        state->comp[i] = state->comp[i] | eq[i]; /* ... or equal (commonly) */

    merge(VACC_L, state->comp, VS, VT);
    vector_copy(VD, VACC_L);
    for (i = 0; i < N; i++)
        state->ne[i] = 0;
    for (i = 0; i < N; i++)
        state->co[i] = 0;
    return;
#endif
}

static void VGE(usf_state_t * state, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, state->VR[vt], e);
    do_ge(state, state->VR[vd], state->VR[vs], ST);
    return;
}
