/******************************************************************************\
* Authors:  Iconoclast                                                         *
* Release:  2013.10.07                                                         *
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
#ifndef _CLAMP_H
#define _CLAMP_H

/*
 * for ANSI compliance (null INLINE attribute if not already set to `inline`)
 * Include "rsp.h" for active, non-ANSI inline definition.
 */
#ifndef INLINE
#define INLINE
#endif

/*
 * dependency for 48-bit accumulator access
 */
#include "vu.h"

/*
 * vector select merge (`VMRG`) formula
 *
 * This is really just a vectorizer for ternary conditional storage.
 * I've named it so because it directly maps to the VMRG op-code.
 * -- example --
 * for (i = 0; i < N; i++)
 *     if (c_pass)
 *         dest = element_a;
 *     else
 *         dest = element_b;
 */
static INLINE void merge(short* VD, short* cmp, short* pass, short* fail)
{
    register int i;

#ifdef ARCH_MIN_ARM_NEON

	int16x8_t p,f,d,c,vd,temp;

	p = vld1q_s16((const int16_t*)pass);
	f = vld1q_s16((const int16_t*)fail);
	c = vld1q_s16((const int16_t*)cmp);
	
	d = vsubq_s16(p,f);
	vd = vmlaq_s16(f, c, d); //vd = f + (cmp * d)
	vst1q_s16(VD, vd);
	return;
	
#else

#if (0)
/* Do not use this version yet, as it still does not vectorize to SSE2. */
    for (i = 0; i < N; i++)
        VD[i] = (cmp[i] != 0) ? pass[i] : fail[i];
#else
    ALIGNED short diff[N];

    for (i = 0; i < N; i++)
        diff[i] = pass[i] - fail[i];
    for (i = 0; i < N; i++)
        VD[i] = fail[i] + cmp[i]*diff[i]; /* actually `(cmp[i] != 0)*diff[i]` */
#endif
    return;

#endif
}

#ifdef ARCH_MIN_ARM_NEON
static INLINE void vector_copy(short * VD, short * VS)
{
    int16x8_t xmm;
    xmm = vld1q_s16((const int16_t*)VS);
    vst1q_s16(VD, xmm);

    return;
}

static INLINE void SIGNED_CLAMP_ADD(usf_state_t * state, short* VD, short* VS, short* VT)
{
    int16x8_t dst, src, vco, max, min;

    src = vld1q_s16((const int16_t*)VS);
    dst = vld1q_s16((const int16_t*)VT);
    vco = vld1q_s16((const int16_t*)state->co);

    max = vmaxq_s16(dst, src);
    min = vminq_s16(dst, src);

    min = vqaddq_s16(min, vco);
    max = vqaddq_s16(max, min);

    vst1q_s16(VD, max);
    return;

}

static INLINE void SIGNED_CLAMP_SUB(usf_state_t * state, short* VD, short* VS, short* VT)
{
	int16x8_t dst, src, vco, dif, res, xmm,vd;
	
    src = vld1q_s16((const int16_t*)VS);
	vd = vld1q_s16((const int16_t*)VD);
    dst = vld1q_s16((const int16_t*)VT);
    vco = vld1q_s16((const int16_t*)state->co);

    res = vqsubq_s16(src, dst);

    dif = vaddq_s16(res, vco);
    dif = veorq_s16(dif, res);
    dif = vandq_s16(dif, dst);
    xmm = vsubq_s16(src, dst); 
	src = vbicq_s16(dif, src); 
    xmm = vandq_s16(xmm, src);
    xmm = vshrq_n_s16(xmm, 15);

	xmm = vbicq_s16(vco, xmm);
    res = vqsubq_s16(res, xmm);
    vst1q_s16(VD, res);

    return;
	
}

static INLINE void SIGNED_CLAMP_AM(usf_state_t * state, short* VD)
{
	int16x8_t pvs, pvd;
	int16x8x2_t packed;
	int16x8_t result;
	int16x4_t low, high;
	
	pvs = vld1q_s16((const int16_t*)VACC_H);
	pvd = vld1q_s16((const int16_t*)VACC_M);
		
	packed = vzipq_s16(pvd,pvs);
		
	low = vqmovn_s32((int32x4_t)packed.val[0]);
	high = vqmovn_s32((int32x4_t)packed.val[1]);
	
	result = vcombine_s16(low,high);
		
	vst1q_s16(VD,result);

	return;
}

#endif

#if !defined ARCH_MIN_SSE2 && !defined ARCH_MIN_ARM_NEON

static INLINE void vector_copy(short* VD, short* VS)
{
#if (0)
    memcpy(VD, VS, N*sizeof(short));
#else
    register int i;

    for (i = 0; i < N; i++)
        VD[i] = VS[i];
#endif
    return;
}

static INLINE void SIGNED_CLAMP_ADD(usf_state_t * state, short* VD, short* VS, short* VT)
{
    ALIGNED int32_t sum[N];
    ALIGNED short hi[N], lo[N];
    register int i;

    for (i = 0; i < N; i++)
        sum[i] = VS[i] + VT[i] + state->co[i];
    for (i = 0; i < N; i++)
        lo[i] = (sum[i] + 0x8000) >> 31;
    for (i = 0; i < N; i++)
        hi[i] = (0x7FFF - sum[i]) >> 31;
    vector_copy(VD, VACC_L);
    for (i = 0; i < N; i++)
        VD[i] &= ~lo[i];
    for (i = 0; i < N; i++)
        VD[i] |=  hi[i];
    for (i = 0; i < N; i++)
        VD[i] ^= 0x8000 & (hi[i] | lo[i]);
    return;
}


static INLINE void SIGNED_CLAMP_SUB(usf_state_t * state, short* VD, short* VS, short* VT)
{
    ALIGNED int32_t dif[N];
    ALIGNED short hi[N], lo[N];
    register int i;

    for (i = 0; i < N; i++)
        dif[i] = VS[i] - VT[i] - state->co[i];
    for (i = 0; i < N; i++)
        lo[i] = (dif[i] + 0x8000) >> 31;
    for (i = 0; i < N; i++)
        hi[i] = (0x7FFF - dif[i]) >> 31;
    vector_copy(VD, VACC_L);
    for (i = 0; i < N; i++)
        VD[i] &= ~lo[i];
    for (i = 0; i < N; i++)
        VD[i] |=  hi[i];
    for (i = 0; i < N; i++)
        VD[i] ^= 0x8000 & (hi[i] | lo[i]);
    return;
}

static INLINE void SIGNED_CLAMP_AM(usf_state_t * state, short* VD)
{ 
    ALIGNED short hi[N], lo[N];
    register int i;

    for (i = 0; i < N; i++)
        lo[i]  = (VACC_H[i] < ~0);
    for (i = 0; i < N; i++)
        lo[i] |= (VACC_H[i] < 0) & !(VACC_M[i] < 0);
    for (i = 0; i < N; i++)
        hi[i]  = (VACC_H[i] >  0);
    for (i = 0; i < N; i++)
        hi[i] |= (VACC_H[i] == 0) & (VACC_M[i] < 0);
    vector_copy(VD, VACC_M);
    for (i = 0; i < N; i++)
        VD[i] &= -(lo[i] ^ 1);
    for (i = 0; i < N; i++)
        VD[i] |= -(hi[i] ^ 0);
    for (i = 0; i < N; i++)
        VD[i] ^= 0x8000 * (hi[i] | lo[i]);
    return;
}
#endif

#ifdef ARCH_MIN_SSE2
/*
 * We actually need to write explicit SSE2 code for this because GCC 4.8.1
 * (and possibly later versions) has a code generation bug with vectorizing
 * the accumulator when it's a signed short (but not when it's unsigned, for
 * some stupid and buggy reason).
 *
 * In addition, as of the more stable GCC 4.7.2 release, while vectorizing
 * the accumulator write-backs into SSE2 for me is successfully done, we save
 * just one extra scalar x86 instruction for every RSP vector op-code when we
 * use SSE2 explicitly for this particular goal instead of letting GCC do it.
 */
static INLINE void vector_copy(short* VD, short* VS)
{
    __m128i xmm;

    xmm = _mm_load_si128((__m128i *)VS);
    _mm_store_si128((__m128i *)VD, xmm);
    return;
}

static INLINE void SIGNED_CLAMP_ADD(usf_state_t * state, short* VD, short* VS, short* VT)
{
    __m128i dst, src, vco;
    __m128i max, min;

    src = _mm_load_si128((__m128i *)VS);
    dst = _mm_load_si128((__m128i *)VT);
    vco = _mm_load_si128((__m128i *)state->co);

/*
 * Due to premature clamping in between adds, sometimes we need to add the
 * LESSER of two integers, either VS or VT, to the carry-in flag matching the
 * current vector register slice, BEFORE finally adding the greater integer.
 */
    max = _mm_max_epi16(dst, src);
    min = _mm_min_epi16(dst, src);

    min = _mm_adds_epi16(min, vco);
    max = _mm_adds_epi16(max, min);
    _mm_store_si128((__m128i *)VD, max);
    return;
}
static INLINE void SIGNED_CLAMP_SUB(usf_state_t * state, short* VD, short* VS, short* VT)
{
    __m128i dst, src, vco;
    __m128i dif, res, xmm;

    src = _mm_load_si128((__m128i *)VS);
    dst = _mm_load_si128((__m128i *)VT);
    vco = _mm_load_si128((__m128i *)state->co);

    res = _mm_subs_epi16(src, dst);

/*
 * Due to premature clamps in-between subtracting two of the three operands,
 * we must be careful not to offset the result accidentally when subtracting
 * the corresponding VCO flag AFTER the saturation from doing (VS - VT).
 */
    dif = _mm_add_epi16(res, vco);
    dif = _mm_xor_si128(dif, res); /* Adding one suddenly inverts the sign? */
    dif = _mm_and_si128(dif, dst); /* Sign change due to subtracting a neg. */
    xmm = _mm_sub_epi16(src, dst);
    src = _mm_andnot_si128(src, dif); /* VS must be >= 0x0000 for overflow. */
    xmm = _mm_and_si128(xmm, src); /* VS + VT != INT16_MIN; VS + VT >= +32768 */
    xmm = _mm_srli_epi16(xmm, 15); /* src = (INT16_MAX + 1 === INT16_MIN) ? */

    xmm = _mm_andnot_si128(xmm, vco); /* If it's NOT overflow, keep flag. */
    res = _mm_subs_epi16(res, xmm);
    _mm_store_si128((__m128i *)VD, res);
    return;
}
static INLINE void SIGNED_CLAMP_AM(usf_state_t * state, short* VD)
{ /* typical sign-clamp of accumulator-mid (bits 31:16) */
    __m128i dst, src;
    __m128i pvd, pvs;

    pvs = _mm_load_si128((__m128i *)VACC_H);
    pvd = _mm_load_si128((__m128i *)VACC_M);
    dst = _mm_unpacklo_epi16(pvd, pvs);
    src = _mm_unpackhi_epi16(pvd, pvs);

    dst = _mm_packs_epi32(dst, src);
    _mm_store_si128((__m128i *)VD, dst);
    return;
}
#endif

static INLINE void UNSIGNED_CLAMP(usf_state_t * state, short* VD)
{ /* sign-zero hybrid clamp of accumulator-mid (bits 31:16) */

    ALIGNED short cond[N];
    ALIGNED short temp[N];
    register int i;

#ifdef ARCH_MIN_ARM_NEON
	
	uint16x8_t c;
	int16x8_t t = vld1q_s16((const int16_t*)temp);
	int16x8_t vaccm = vld1q_s16((const int16_t*)VACC_M);

    SIGNED_CLAMP_AM(state, temp);

	c = vcgtq_s16(t,vaccm);
	int16x8_t t_ = vshrq_n_s16(t,15);
	int16x8_t vd = vbicq_s16(t,t_);
	vd = vorrq_s16(vd,(int16x8_t)c);
	vst1q_s16(VD, vd);
	
	return;

#else

    SIGNED_CLAMP_AM(state, temp); /* no direct map in SSE, but closely based on this */
    for (i = 0; i < N; i++)
        cond[i] = -(temp[i] >  VACC_M[i]); /* VD |= -(ACC47..16 > +32767) */
    for (i = 0; i < N; i++)
        VD[i] = temp[i] & ~(temp[i] >> 15); /* Only this clamp is unsigned. */
    for (i = 0; i < N; i++)
        VD[i] = VD[i] | cond[i];
    return;
#endif
}

static INLINE void SIGNED_CLAMP_AL(usf_state_t * state, short* VD)
{ /* sign-clamp accumulator-low (bits 15:0) */

    ALIGNED short cond[N];
    ALIGNED short temp[N];
    register int i;
	
	
#ifdef ARCH_MIN_ARM_NEON
	
	SIGNED_CLAMP_AM(state, temp); 
	
	uint16x8_t c;
	int16x8_t eightk = vdupq_n_s16(0x8000);
	uint16x8_t one = vdupq_n_u16(1);
	int16x8_t t = vld1q_s16((const int16_t*)temp);
	int16x8_t vaccm = vld1q_s16((const int16_t*)VACC_M);

	c = vceqq_s16(t,vaccm);
	c = vaddq_u16(c, one); 
	t = veorq_s16(t, eightk);
	vst1q_u16(cond,c);
	vst1q_s16(temp,t);	
	merge(VD, cond, temp, VACC_L);
	
	return;
#else

    SIGNED_CLAMP_AM(state, temp); /* no direct map in SSE, but closely based on this */
    for (i = 0; i < N; i++)
        cond[i] = (temp[i] != VACC_M[i]); /* result_clamped != result_raw ? */
    for (i = 0; i < N; i++)
        temp[i] ^= 0x8000; /* half-assed unsigned saturation mix in the clamp */
    merge(VD, cond, temp, VACC_L);
    return;
#endif
}
#endif
