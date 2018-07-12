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

INLINE static void clr_bi(usf_state_t * state, short* VD, short* VS, short* VT)
{ /* clear CARRY and borrow in to accumulators */

#ifdef ARCH_MIN_ARM_NEON
    
	int16x8_t vs, vt, zero, co;
	zero = vdupq_n_s16(0);
	vs = vld1q_s16((const int16_t*)VS);
	vt = vld1q_s16((const int16_t*)VT);
	co = vld1q_s16((const int16_t*)state->co);
	
	vs = vsubq_s16(vs,vt);
	vs = vsubq_s16(vs,co);
	vst1q_s16(VACC_L, vs);
	
	SIGNED_CLAMP_SUB(state, VD, VS, VT);
	vst1q_s16(state->ne, zero);
	vst1q_s16(state->co, zero);
	return;
#else

    register int i;

    for (i = 0; i < N; i++)
        VACC_L[i] = VS[i] - VT[i] - state->co[i];
    SIGNED_CLAMP_SUB(state, VD, VS, VT);
    for (i = 0; i < N; i++)
        state->ne[i] = 0;
    for (i = 0; i < N; i++)
        state->co[i] = 0;
    return;
#endif
}

static void VSUB(usf_state_t * state, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, state->VR[vt], e);
    clr_bi(state, state->VR[vd], state->VR[vs], ST);
    return;
}
