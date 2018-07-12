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

INLINE static void do_ne(usf_state_t * state, short* VD, short* VS, short* VT)
{

#ifdef ARCH_MIN_ARM_NEON
	
	int16x8_t vs, vt,vaccl, ne;
	int16x8_t zero = vdupq_n_s16(0);
	uint16x8_t cond;
	
	vs = vld1q_s16((const int16_t*)VS);
	vt = vld1q_s16((const int16_t*)VT);
	ne = vld1q_s16((const int16_t*)state->ne);

	cond = vceqq_s16(vs,vt);
	cond = vmvnq_u16(cond); // this is needed if you need to do "not-equal"
	cond = (uint16x8_t)vnegq_s16((int16x8_t)cond);
	uint16x8_t comp = vorrq_u16(cond, (uint16x8_t)ne);
	
	vst1q_s16(state->clip, zero);
	vst1q_s16(state->comp, (int16x8_t)cond);

	vector_copy(VACC_L, VS);		
	vector_copy(VD, VACC_L);
	vst1q_s16(state->ne, zero);
	vst1q_s16(state->co, zero);	
	
	return;
#else

    register int i;

    for (i = 0; i < N; i++)
        state->clip[i] = 0;
    for (i = 0; i < N; i++)
        state->comp[i] = (VS[i] != VT[i]);
    for (i = 0; i < N; i++)
        state->comp[i] = state->comp[i] | state->ne[i];
#if (0)
    merge(VACC_L, state->comp, VS, VT); /* correct but redundant */
#else
    vector_copy(VACC_L, VS);
#endif
    vector_copy(VD, VACC_L);

    for (i = 0; i < N; i++)
        state->ne[i] = 0;
    for (i = 0; i < N; i++)
        state->co[i] = 0;
    return;
#endif
}

static void VNE(usf_state_t * state, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, state->VR[vt], e);
    do_ne(state, state->VR[vd], state->VR[vs], ST);
    return;
}
