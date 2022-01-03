#ifndef __RATIOCNTR_H__
#define __RATIOCNTR_H__

#include "../common_def.h"

#if ! LOW_PRECISION_RATIOCNTR
// by default we use a high-precision 32.32 fixed point counter
#define RC_SHIFT	32
typedef UINT64		RC_TYPE;
typedef INT64		RC_STYPE;
#else
// alternatively we can use lower-precision 12.20 fixed point
#define RC_SHIFT	20
typedef UINT32		RC_TYPE;
typedef INT32		RC_STYPE;
#endif

typedef struct
{
	RC_TYPE inc;	// counter increment
	RC_TYPE val;	// current value
} RATIO_CNTR;

INLINE void RC_SET_RATIO(RATIO_CNTR* rc, UINT32 mul, UINT32 div)
{
	rc->inc = (RC_TYPE)((((UINT64)mul << RC_SHIFT) + div / 2) / div);
}

INLINE void RC_SET_INC(RATIO_CNTR* rc, double val)
{
	rc->inc = (RC_TYPE)(((RC_TYPE)1 << RC_SHIFT) * val + 0.5);
}

INLINE void RC_STEP(RATIO_CNTR* rc)
{
	rc->val += rc->inc;
}

INLINE void RC_STEPS(RATIO_CNTR* rc, UINT32 step)
{
	rc->val += rc->inc * step;
}

INLINE UINT32 RC_GET_VAL(const RATIO_CNTR* rc)
{
	return (UINT32)(rc->val >> RC_SHIFT);
}

INLINE void RC_SET_VAL(RATIO_CNTR* rc, UINT32 val)
{
	rc->val = (RC_TYPE)val << RC_SHIFT;
}

INLINE void RC_RESET(RATIO_CNTR* rc)
{
	rc->val = 0;
}

INLINE void RC_RESET_PRESTEP(RATIO_CNTR* rc)
{
	rc->val = ((RC_TYPE)1 << RC_SHIFT) - rc->inc;
}

INLINE void RC_MASK(RATIO_CNTR* rc)
{
	rc->val &= (((RC_TYPE)1 << RC_SHIFT) - 1);
}

INLINE void RC_VAL_INC(RATIO_CNTR* rc)
{
	rc->val += (RC_TYPE)1 << RC_SHIFT;
}

INLINE void RC_VAL_DEC(RATIO_CNTR* rc)
{
	rc->val -= (RC_TYPE)1 << RC_SHIFT;
}

INLINE void RC_VAL_ADD(RATIO_CNTR* rc, INT32 val)
{
	rc->val += (RC_STYPE)val << RC_SHIFT;
}

INLINE void RC_VAL_SUB(RATIO_CNTR* rc, INT32 val)
{
	rc->val -= (RC_STYPE)val << RC_SHIFT;
}

#endif	// __RATIOCNTR_H__
