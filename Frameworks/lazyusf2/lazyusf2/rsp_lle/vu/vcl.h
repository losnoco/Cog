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

INLINE static void do_cl(usf_state_t * state, short* VD, short* VS, short* VT)
{

    ALIGNED short eq[N], ge[N], le[N];
    ALIGNED short gen[N], len[N], lz[N], uz[N], sn[N];
    ALIGNED short diff[N];
    ALIGNED short cmp[N];
    ALIGNED unsigned short VB[N], VC[N];
    register int i;

#ifdef ARCH_MIN_ARM_NEON

	int16x8_t v_vc,neg_sn,v_eq,v_vb,v_sn,v_diff,v_uz,v_cmp;

	int16x8_t zero = vdupq_n_s16(0);
	int16x8_t minus1 = vdupq_n_s16(-1);
	int16x8_t one = vdupq_n_s16(1);
	
	int16x8_t vs = vld1q_s16((const int16_t *)VS);
    int16x8_t vt = vld1q_s16((const int16_t *)VT);
	int16x8_t ne = vld1q_s16((const int16_t *)state->ne);
	int16x8_t co = vld1q_s16((const int16_t *)state->co);
	int16x8_t vce = vld1q_s16((const int16_t *)state->vce);

	v_vb = vs;
	v_vc = vt;

	v_eq = veorq_s16(ne, one);
	v_sn = co;

	neg_sn = vnegq_s16((int16x8_t)v_sn);
	v_vc = veorq_s16(v_vc, (int16x8_t)neg_sn);
	v_vc = vaddq_s16(v_vc, v_sn);
		
	v_diff = vsubq_s16(v_vb,v_vc);
	
	uint16x8_t vb_cond = vceqq_s16(v_vb,minus1);
	uint16x8_t vc_cond = vceqq_s16(v_vc,zero);
	vb_cond = vmvnq_u16(vb_cond);
	vc_cond = vmvnq_u16(vc_cond);
	v_uz = vorrq_s16((int16x8_t)vb_cond, (int16x8_t)vc_cond);

	uint16x8_t v_lz = vceqq_s16(v_diff,zero);
	int16x8_t lz_s = vnegq_s16((int16x8_t)v_lz);

	int16x8_t v_gen = vorrq_s16(lz_s,v_uz);
	int16x8_t v_len = vandq_s16(lz_s,v_uz);
	v_gen = vandq_s16(v_gen,vce);
	
	vce = veorq_s16(vce,one);
	v_len = vandq_s16(v_len,vce);
	
	v_len = vorrq_s16(v_len,v_gen);
	uint16x8_t gen_u = vcgeq_u16((uint16x8_t)v_vb,(uint16x8_t)v_vc);
	v_gen = vnegq_s16((int16x8_t)gen_u);

	v_cmp = vandq_s16(v_eq,v_sn);
	
	vst1q_s16(cmp, v_cmp);
	vst1q_s16(len, v_len);
	
	merge(le, cmp, len, state->comp);
	int16x8_t sn_xor = veorq_s16(v_sn,one);
	v_cmp = vandq_s16(v_eq,sn_xor);

	vst1q_s16(cmp, v_cmp);
	vst1q_s16(gen, v_gen);
	vst1q_s16(sn, v_sn);
	vst1q_s16(VC, v_vc);
	
    merge(ge, cmp, gen, state->clip);

    merge(cmp, sn, le, ge);
    merge(VACC_L, cmp, (short *)VC, VS);
    vector_copy(VD, VACC_L);
	
	int16x8_t v_ge = vld1q_s16((const int16_t *)ge);
	int16x8_t v_le = vld1q_s16((const int16_t *)le);
		
	vst1q_s16(state->clip,v_ge);
	vst1q_s16(state->comp,v_le);	
	
	vst1q_s16(state->ne,zero);
	vst1q_s16(state->co,zero);
	vst1q_s16(state->vce,zero);
	
	return;

#else



    for (i = 0; i < N; i++)
        VB[i] = VS[i];
    for (i = 0; i < N; i++)
        VC[i] = VT[i];

/*
    for (i = 0; i < N; i++)
        ge[i] = clip[i];
    for (i = 0; i < N; i++)
        le[i] = comp[i];
*/
    for (i = 0; i < N; i++)
        eq[i] = state->ne[i] ^ 1;
    for (i = 0; i < N; i++)
        sn[i] = state->co[i];
/*
 * Now that we have extracted all the flags, we will essentially be masking
 * them back in where they came from redundantly, unless the corresponding
 * NOTEQUAL bit from VCO upper was not set....
 */
    for (i = 0; i < N; i++)
        VC[i] = VC[i] ^ -sn[i];
    for (i = 0; i < N; i++)
        VC[i] = VC[i] + sn[i]; /* conditional negation, if sn */
    for (i = 0; i < N; i++)
        diff[i] = VB[i] - VC[i];
    for (i = 0; i < N; i++)
        uz[i] = (VB[i] - VC[i] - 0xFFFF) >> 31;
    for (i = 0; i < N; i++)
        lz[i] = (diff[i] == 0x0000);
    for (i = 0; i < N; i++)
        gen[i] = lz[i] | uz[i];
    for (i = 0; i < N; i++)
        len[i] = lz[i] & uz[i];
    for (i = 0; i < N; i++)
        gen[i] = gen[i] & state->vce[i];
    for (i = 0; i < N; i++)
        len[i] = len[i] & (state->vce[i] ^ 1);
    for (i = 0; i < N; i++)
        len[i] = len[i] | gen[i];
    for (i = 0; i < N; i++)
        gen[i] = (VB[i] >= VC[i]);

    for (i = 0; i < N; i++)
        cmp[i] = eq[i] & sn[i];
    merge(le, cmp, len, state->comp);

    for (i = 0; i < N; i++)
        cmp[i] = eq[i] & (sn[i] ^ 1);
    merge(ge, cmp, gen, state->clip);

    merge(cmp, sn, le, ge);
    merge(VACC_L, cmp, (short *)VC, VS);
    vector_copy(VD, VACC_L);

    for (i = 0; i < N; i++)
        state->clip[i] = ge[i];
    for (i = 0; i < N; i++)
        state->comp[i] = le[i];
    for (i = 0; i < N; i++)
        state->ne[i] = 0;
    for (i = 0; i < N; i++)
        state->co[i] = 0;
    for (i = 0; i < N; i++)
        state->vce[i] = 0;
    return;
#endif
}

static void VCL(usf_state_t * state, int vd, int vs, int vt, int e)
{
    ALIGNED short ST[N];

    SHUFFLE_VECTOR(ST, state->VR[vt], e);
    do_cl(state, state->VR[vd], state->VR[vs], ST);
    return;
}
