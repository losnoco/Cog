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

INLINE static void do_ch(usf_state_t * state, short* VD, short* VS, short* VT)
{
    ALIGNED short eq[N], ge[N], le[N];
    ALIGNED short sn[N];
    ALIGNED short VC[N];
    ALIGNED short diff[N];

#ifdef ARCH_MIN_ARM_NEON

	int16x8_t v_vc,neg_sn,vce,v_eq;
	int16x8_t zero = vdupq_n_s16(0);
	int16x8_t one = vdupq_n_s16(1);

	int16x8_t vs = vld1q_s16((const int16_t *)VS);
    int16x8_t vt = vld1q_s16((const int16_t *)VT);
	v_vc = vt;
	int16x8_t v_sn = veorq_s16(vs,v_vc);
	uint16x8_t sn_u = vcltq_s16(v_sn,zero);
	v_vc = veorq_s16(v_vc, (int16x8_t)sn_u);
	neg_sn = vnegq_s16((int16x8_t)sn_u);
	
	uint16x8_t vs_vc_eq = vceqq_s16(vs,v_vc);
	vce = vandq_s16((int16x8_t)vs_vc_eq,v_sn);
	
	v_vc = vaddq_s16(v_vc,v_sn);
	v_eq = vorrq_s16(vce,(int16x8_t)vs_vc_eq);
	v_eq = vnegq_s16(v_eq);

	int16x8_t not_sn = vsubq_s16(neg_sn, one);
	int16x8_t neg_vs = vmvnq_s16(vs);
	int16x8_t v_diff = vorrq_s16(neg_vs, not_sn);
	uint16x8_t ule = vcleq_s16(vt,v_diff);
	int16x8_t v_le = vnegq_s16((int16x8_t)ule);

	v_diff = vorrq_s16(vs, (int16x8_t)sn_u);
	uint16x8_t uge = vcgeq_s16(v_diff,vt);
	int16x8_t v_ge = vnegq_s16((int16x8_t)uge);

	vst1q_s16(ge, v_ge);
	vst1q_s16(le, v_le);
	vst1q_s16(sn, v_sn);
	vst1q_s16(VC, v_vc);
	
	merge(state->comp, sn, le, ge);
    merge(VACC_L, state->comp, VC, VS);
    vector_copy(VD, VACC_L);

	v_eq = veorq_s16(v_eq, one);
	
	vst1q_s16(state->clip, v_ge);
	vst1q_s16(state->comp, v_le);
	vst1q_s16(state->ne, v_eq);
	vst1q_s16(state->co, v_sn);
	   
	return;
	
#else
   
    register int i;

    for (i = 0; i < N; i++)
        VC[i] = VT[i];
    for (i = 0; i < N; i++)
        sn[i] = (VS[i] ^ VC[i]) < 0;
    for (i = 0; i < N; i++)
        VC[i] ^= -sn[i]; /* if (sn == ~0) {VT = ~VT;} else {VT =  VT;} */
    for (i = 0; i < N; i++)
        state->vce[i]  = (VS[i] == VC[i]); /* (VR[vs][i] + ~VC[i] == ~1); */
    for (i = 0; i < N; i++)
        state->vce[i] &= sn[i];
    for (i = 0; i < N; i++)
        VC[i] += sn[i]; /* converts ~(VT) into -(VT) if (sign) */
    for (i = 0; i < N; i++)
        eq[i]  = (VS[i] == VC[i]);
    for (i = 0; i < N; i++)
        eq[i] |= state->vce[i];

#if (0)
    for (i = 0; i < N; i++)
        le[i] = sn[i] ? (VS[i] <= VC[i]) : (VC[i] < 0);
    for (i = 0; i < N; i++)
        ge[i] = sn[i] ? (VC[i] > 0x0000) : (VS[i] >= VC[i]);
#elif (0)
    for (i = 0; i < N; i++)
        le[i] = sn[i] ? (VT[i] <= -VS[i]) : (VT[i] <= ~0x0000);
    for (i = 0; i < N; i++)
        ge[i] = sn[i] ? (~0x0000 >= VT[i]) : (VS[i] >= VT[i]);
#else
    for (i = 0; i < N; i++)
        diff[i] = -VS[i] | -(sn[i] ^ 1);
    for (i = 0; i < N; i++)
        le[i] = VT[i] <= diff[i];
    for (i = 0; i < N; i++)
        diff[i] = +VS[i] | -(sn[i] ^ 0);
    for (i = 0; i < N; i++)
        ge[i] = diff[i] >= VT[i];
#endif

    merge(state->comp, sn, le, ge);
    merge(VACC_L, state->comp, VC, VS);
    vector_copy(VD, VACC_L);

    for (i = 0; i < N; i++)
        state->clip[i] = ge[i];
    for (i = 0; i < N; i++)
        state->comp[i] = le[i];
    for (i = 0; i < N; i++)
        state->ne[i] = eq[i] ^ 1;
    for (i = 0; i < N; i++)
        state->co[i] = sn[i];
    return;
#endif
}

static void VCH(usf_state_t * state, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, state->VR[vt], e);
    do_ch(state, state->VR[vd], state->VR[vs], ST);
    return;
}
