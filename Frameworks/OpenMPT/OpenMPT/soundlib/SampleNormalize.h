/*
 * SampleNormalize.h
 * -----------------
 * Purpose: Functions for normalizing samples.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "openmpt/all/BuildSettings.hpp"


OPENMPT_NAMESPACE_BEGIN


namespace SC
{  // SC = _S_ample_C_onversion



template <typename Tsample>
struct Normalize;

template <>
struct Normalize<int32>
{
	typedef int32 input_t;
	typedef int32 output_t;
	typedef uint32 peak_t;
	uint32 maxVal;
	MPT_FORCEINLINE Normalize()
		: maxVal(0) {}
	MPT_FORCEINLINE void FindMax(input_t val)
	{
		if(val < 0)
		{
			if(val == std::numeric_limits<int32>::min())
			{
				maxVal = static_cast<uint32>(-static_cast<int64>(std::numeric_limits<int32>::min()));
				return;
			}
			val = -val;
		}
		if(static_cast<uint32>(val) > maxVal)
		{
			maxVal = static_cast<uint32>(val);
		}
	}
	MPT_FORCEINLINE bool IsSilent() const
	{
		return maxVal == 0;
	}
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return Util::muldivrfloor(val, static_cast<uint32>(1) << 31, maxVal);
	}
	MPT_FORCEINLINE peak_t GetSrcPeak() const
	{
		return maxVal;
	}
};

template <>
struct Normalize<float32>
{
	typedef float32 input_t;
	typedef float32 output_t;
	typedef float32 peak_t;
	float maxVal;
	float maxValInv;
	MPT_FORCEINLINE Normalize()
		: maxVal(0.0f), maxValInv(1.0f) {}
	MPT_FORCEINLINE void FindMax(input_t val)
	{
		float absval = std::fabs(val);
		if(absval > maxVal)
		{
			maxVal = absval;
		}
	}
	MPT_FORCEINLINE bool IsSilent()
	{
		if(maxVal == 0.0f)
		{
			maxValInv = 1.0f;
			return true;
		} else
		{
			maxValInv = 1.0f / maxVal;
			return false;
		}
	}
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return val * maxValInv;
	}
	MPT_FORCEINLINE peak_t GetSrcPeak() const
	{
		return maxVal;
	}
};

template <>
struct Normalize<float64>
{
	typedef float64 input_t;
	typedef float64 output_t;
	typedef float64 peak_t;
	double maxVal;
	double maxValInv;
	MPT_FORCEINLINE Normalize()
		: maxVal(0.0), maxValInv(1.0) {}
	MPT_FORCEINLINE void FindMax(input_t val)
	{
		double absval = std::fabs(val);
		if(absval > maxVal)
		{
			maxVal = absval;
		}
	}
	MPT_FORCEINLINE bool IsSilent()
	{
		if(maxVal == 0.0)
		{
			maxValInv = 1.0;
			return true;
		} else
		{
			maxValInv = 1.0 / maxVal;
			return false;
		}
	}
	MPT_FORCEINLINE output_t operator()(input_t val)
	{
		return val * maxValInv;
	}
	MPT_FORCEINLINE peak_t GetSrcPeak() const
	{
		return maxVal;
	}
};


// Reads sample data with Func1, then normalizes the sample data, and then converts it with Func2.
// Func1::output_t and Func2::input_t must be identical.
// Func1 can also be the identity decode (DecodeIdentity<T>).
// Func2 can also be the identity conversion (Convert<T,T>).
template <typename Func2, typename Func1>
struct NormalizationChain
{
	typedef typename Func1::input_t input_t;
	typedef typename Func1::output_t normalize_t;
	typedef typename Normalize<normalize_t>::peak_t peak_t;
	typedef typename Func2::output_t output_t;
	static constexpr std::size_t input_inc = Func1::input_inc;
	Func1 func1;
	Normalize<normalize_t> normalize;
	Func2 func2;
	MPT_FORCEINLINE void FindMax(const input_t *inBuf)
	{
		normalize.FindMax(func1(inBuf));
	}
	MPT_FORCEINLINE bool IsSilent()
	{
		return normalize.IsSilent();
	}
	MPT_FORCEINLINE output_t operator()(const input_t *inBuf)
	{
		return func2(normalize(func1(inBuf)));
	}
	MPT_FORCEINLINE peak_t GetSrcPeak() const
	{
		return normalize.GetSrcPeak();
	}
	MPT_FORCEINLINE NormalizationChain(Func2 f2 = Func2(), Func1 f1 = Func1())
		: func1(f1)
		, func2(f2)
	{
		return;
	}
};



}  // namespace SC



OPENMPT_NAMESPACE_END
