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

INLINE void do_and(usf_state_t * state, short* VD, short* VS, short* VT)
{
#ifdef ARCH_MIN_ARM_NEON

	int16x8_t vs = vld1q_s16((const int16_t *)VS);
    int16x8_t vt = vld1q_s16((const int16_t *)VT);
	int16x8_t vaccl = vandq_s16(vs,vt);
	vst1q_s16(VACC_L, vaccl);
	vector_copy(VD, VACC_L);
	return;
	
#else
    register int i;

    for (i = 0; i < N; i++)
        VACC_L[i] = VS[i] & VT[i];
    vector_copy(VD, VACC_L);
    return;
#endif
}

static void VAND(usf_state_t * state, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, state->VR[vt], e);
    do_and(state, state->VR[vd], state->VR[vs], ST);
    return;
}
