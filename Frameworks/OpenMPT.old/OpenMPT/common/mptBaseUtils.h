/*
 * mptBaseUtils.h
 * --------------
 * Purpose: Various useful utility functions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"


#include "mptBaseMacros.h"
#include "mptBaseTypes.h"

#include <algorithm>
#if MPT_CXX_AT_LEAST(20)
#include <bit>
#endif
#include <limits>
#include <numeric>
#include <utility>

#include <cmath>
#include <cstdlib>

#include <math.h>
#include <stdlib.h>

#if MPT_COMPILER_MSVC
#include <intrin.h>
#endif



OPENMPT_NAMESPACE_BEGIN



// cmath fixups
#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif
#ifndef M_PI_2
#define M_PI_2     1.57079632679489661923
#endif
#ifndef M_LN2
#define M_LN2      0.69314718055994530942
#endif



namespace mpt
{
template <typename T, std::size_t N, typename Tx>
MPT_CONSTEXPR14_FUN std::array<T, N> init_array(const Tx & x)
{
	std::array<T, N> result{};
	for(std::size_t i = 0; i < N; ++i)
	{
		result[i] = x;
	}
	return result;
}
} // namespace mpt



namespace mpt
{

// Work-around for the requirement of at least 1 non-throwing function argument combination in C++ (17,2a).

template <typename Exception>
MPT_CONSTEXPR14_FUN bool constexpr_throw_helper(Exception && e, bool really = true)
{
	//return !really ? really : throw std::forward<Exception>(e);
	if(really)
	{
		throw std::forward<Exception>(e);
	}
	// cppcheck-suppress identicalConditionAfterEarlyExit
	return really;
}
template <typename Exception>
MPT_CONSTEXPR14_FUN bool constexpr_throw(Exception && e)
{
	return mpt::constexpr_throw_helper(std::forward<Exception>(e));
}

template <typename T, typename Exception>
constexpr T constexpr_throw_helper(Exception && e, bool really = true)
{
	//return !really ? really : throw std::forward<Exception>(e);
	if(really)
	{
		throw std::forward<Exception>(e);
	}
	return T{};
}
template <typename T, typename Exception>
constexpr T constexpr_throw(Exception && e)
{
	return mpt::constexpr_throw_helper<T>(std::forward<Exception>(e));
}

}  // namespace mpt



namespace mpt {

// Modulo with more intuitive behaviour for some contexts:
// Instead of being symmetrical around 0, the pattern for positive numbers is repeated in the negative range.
// For example, wrapping_modulo(-1, m) == (m - 1).
// Behaviour is undefined if m<=0.
template<typename T, typename M>
MPT_CONSTEXPR11_FUN auto wrapping_modulo(T x, M m) -> decltype(x % m)
{
	return (x >= 0) ? (x % m) : (m - 1 - ((-1 - x) % m));
}

template<typename T, typename D>
MPT_CONSTEXPR11_FUN auto wrapping_divide(T x, D d) -> decltype(x / d)
{
	return (x >= 0) ? (x / d) : (((x + 1) / d) - 1);
}

} // namespace mpt



namespace mpt {



// Saturate the value of src to the domain of Tdst
template <typename Tdst, typename Tsrc>
inline Tdst saturate_cast(Tsrc src)
{
	// This code tries not only to obviously avoid overflows but also to avoid signed/unsigned comparison warnings and type truncation warnings (which in fact would be safe here) by explicit casting.
	static_assert(std::numeric_limits<Tdst>::is_integer);
	static_assert(std::numeric_limits<Tsrc>::is_integer);
	if constexpr(std::numeric_limits<Tdst>::is_signed && std::numeric_limits<Tsrc>::is_signed)
	{
		if constexpr(sizeof(Tdst) >= sizeof(Tsrc))
		{
			return static_cast<Tdst>(src);
		} else
		{
			return static_cast<Tdst>(std::max(static_cast<Tsrc>(std::numeric_limits<Tdst>::min()), std::min(src, static_cast<Tsrc>(std::numeric_limits<Tdst>::max()))));
		}
	} else if constexpr(!std::numeric_limits<Tdst>::is_signed && !std::numeric_limits<Tsrc>::is_signed)
	{
		if constexpr(sizeof(Tdst) >= sizeof(Tsrc))
		{
			return static_cast<Tdst>(src);
		} else
		{
			return static_cast<Tdst>(std::min(src, static_cast<Tsrc>(std::numeric_limits<Tdst>::max())));
		}
	} else if constexpr(std::numeric_limits<Tdst>::is_signed && !std::numeric_limits<Tsrc>::is_signed)
	{
		if constexpr(sizeof(Tdst) > sizeof(Tsrc))
		{
			return static_cast<Tdst>(src);
		} else if constexpr(sizeof(Tdst) == sizeof(Tsrc))
		{
			return static_cast<Tdst>(std::min(src, static_cast<Tsrc>(std::numeric_limits<Tdst>::max())));
		} else
		{
			return static_cast<Tdst>(std::min(src, static_cast<Tsrc>(std::numeric_limits<Tdst>::max())));
		}
	} else // Tdst unsigned, Tsrc signed
	{
		if constexpr(sizeof(Tdst) >= sizeof(Tsrc))
		{
			return static_cast<Tdst>(std::max(static_cast<Tsrc>(0), src));
		} else
		{
			return static_cast<Tdst>(std::max(static_cast<Tsrc>(0), std::min(src, static_cast<Tsrc>(std::numeric_limits<Tdst>::max()))));
		}
	}
}

template <typename Tdst>
inline Tdst saturate_cast(double src)
{
	if(src >= static_cast<double>(std::numeric_limits<Tdst>::max()))
	{
		return std::numeric_limits<Tdst>::max();
	}
	if(src <= static_cast<double>(std::numeric_limits<Tdst>::min()))
	{
		return std::numeric_limits<Tdst>::min();
	}
	return static_cast<Tdst>(src);
}

template <typename Tdst>
inline Tdst saturate_cast(float src)
{
	if(src >= static_cast<float>(std::numeric_limits<Tdst>::max()))
	{
		return std::numeric_limits<Tdst>::max();
	}
	if(src <= static_cast<float>(std::numeric_limits<Tdst>::min()))
	{
		return std::numeric_limits<Tdst>::min();
	}
	return static_cast<Tdst>(src);
}


#if MPT_CXX_AT_LEAST(20)

using std::popcount;
using std::has_single_bit;
using std::bit_ceil;
using std::bit_floor;
using std::bit_width;
using std::rotl;
using std::rotr;

#else

// C++20 <bit> header.
// Note that we do not use SFINAE here but instead rely on static_assert.

template <typename T>
MPT_CONSTEXPR14_FUN int popcount(T val) noexcept
{
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::is_unsigned<T>::value);
	int result = 0;
	while(val > 0)
	{
		if(val & 0x1)
		{
			result++;
		}
		val >>= 1;
	}
	return result;
}

template <typename T>
MPT_CONSTEXPR14_FUN bool has_single_bit(T x) noexcept
{
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::is_unsigned<T>::value);
	return mpt::popcount(x) == 1;
}

template <typename T>
MPT_CONSTEXPR14_FUN T bit_ceil(T x) noexcept
{
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::is_unsigned<T>::value);
	T result = 1;
	while(result < x)
	{
		T newresult = result << 1;
		if(newresult < result)
		{
			return 0;
		}
		result = newresult;
	}
	return result;
}

template <typename T>
MPT_CONSTEXPR14_FUN T bit_floor(T x) noexcept
{
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::is_unsigned<T>::value);
	if(x == 0)
	{
		return 0;
	}
	T result = 1;
	do
	{
		T newresult = result << 1;
		if(newresult < result)
		{
			return result;
		}
		result = newresult;
	} while(result <= x);
	return result >> 1;
}
 
template <typename T>
MPT_CONSTEXPR14_FUN T bit_width(T x) noexcept
{
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::is_unsigned<T>::value);
	T result = 0;
	while(x > 0)
	{
		x >>= 1;
		result += 1;
	}
	return result;
}

namespace detail
{

template <typename T>
MPT_CONSTEXPR14_FUN T rotl(T x, int r) noexcept
{
	auto N = std::numeric_limits<T>::digits;
	return (x >> (N - r)) | (x << r);
}

template <typename T>
MPT_CONSTEXPR14_FUN T rotr(T x, int r) noexcept
{
	auto N = std::numeric_limits<T>::digits;
	return (x << (N - r)) | (x >> r);
}

} // namespace detail

template <typename T>
MPT_CONSTEXPR14_FUN T rotl(T x, int s) noexcept
{
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::is_unsigned<T>::value);
	auto N = std::numeric_limits<T>::digits;
	auto r = s % N;
	return (s < 0) ? detail::rotr(x, -s) : ((x >> (N - r)) | (x << r));
}

template <typename T>
MPT_CONSTEXPR14_FUN T rotr(T x, int s) noexcept
{
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::is_unsigned<T>::value);
	auto N = std::numeric_limits<T>::digits;
	auto r = s % N;
	return (s < 0) ? detail::rotl(x, -s) : ((x << (N - r)) | (x >> r));
}

#endif

} // namespace mpt


namespace Util
{

namespace detail
{
template <typename Tmod, Tmod m>
struct ModIfNotZeroImpl
{
	template <typename Tval>
	inline Tval mod(Tval x)
	{
		static_assert(std::numeric_limits<Tmod>::is_integer);
		static_assert(!std::numeric_limits<Tmod>::is_signed);
		static_assert(std::numeric_limits<Tval>::is_integer);
		static_assert(!std::numeric_limits<Tval>::is_signed);
		return static_cast<Tval>(x % m);
	}
};
template <> struct ModIfNotZeroImpl<uint8 , 0> { template <typename Tval> inline Tval mod(Tval x) { return x; } };
template <> struct ModIfNotZeroImpl<uint16, 0> { template <typename Tval> inline Tval mod(Tval x) { return x; } };
template <> struct ModIfNotZeroImpl<uint32, 0> { template <typename Tval> inline Tval mod(Tval x) { return x; } };
template <> struct ModIfNotZeroImpl<uint64, 0> { template <typename Tval> inline Tval mod(Tval x) { return x; } };
} // namespace detail
// Returns x % m if m != 0, x otherwise.
// i.e. "return (m == 0) ? x : (x % m);", but without causing a warning with stupid older compilers
template <typename Tmod, Tmod m, typename Tval>
inline Tval ModIfNotZero(Tval x)
{
	return detail::ModIfNotZeroImpl<Tmod, m>().mod(x);
}

// Returns true iff Tdst can represent the value val.
// Use as if(Util::TypeCanHoldValue<uint8>(-1)).
template <typename Tdst, typename Tsrc>
inline bool TypeCanHoldValue(Tsrc val)
{
	return (static_cast<Tsrc>(mpt::saturate_cast<Tdst>(val)) == val);
}

// Grows x with an exponential factor suitable for increasing buffer sizes.
// Clamps the result at limit.
// And avoids integer overflows while doing its business.
// The growth factor is 1.5, rounding down, execpt for the initial x==1 case.
template <typename T, typename Tlimit>
inline T ExponentialGrow(const T &x, const Tlimit &limit)
{
	MPT_ASSERT(x > 0);
	MPT_ASSERT(limit > 0);
	if(x == 1)
	{
		return 2;
	}
	T add = std::min(x >> 1, std::numeric_limits<T>::max() - x);
	return std::min(x + add, mpt::saturate_cast<T>(limit));
}
									
template <typename T>
inline T ExponentialGrow(const T &x)
{
	return Util::ExponentialGrow(x, std::numeric_limits<T>::max());
}

} //namespace Util


// Limits 'val' to given range. If 'val' is less than 'lowerLimit', 'val' is set to value 'lowerLimit'.
// Similarly if 'val' is greater than 'upperLimit', 'val' is set to value 'upperLimit'.
// If 'lowerLimit' > 'upperLimit', 'val' won't be modified.
template<class T, class C>
inline void Limit(T& val, const C lowerLimit, const C upperLimit)
{
	if(lowerLimit > upperLimit) return;
	if(val < lowerLimit) val = lowerLimit;
	else if(val > upperLimit) val = upperLimit;
}


// Like Limit, but returns value
template<class T, class C>
inline T Clamp(T val, const C lowerLimit, const C upperLimit)
{
	if(val < lowerLimit) return lowerLimit;
	else if(val > upperLimit) return upperLimit;
	else return val;
}

// Check if val is in [lo,hi] without causing compiler warnings
// if theses checks are always true due to the domain of T.
// GCC does not warn if the type is templated.
template<typename T, typename C>
inline bool IsInRange(T val, C lo, C hi)
{
	return lo <= val && val <= hi;
}

// Like Limit, but with upperlimit only.
template<class T, class C>
inline void LimitMax(T& val, const C upperLimit)
{
	if(val > upperLimit)
		val = upperLimit;
}


// Returns sign of a number (-1 for negative numbers, 1 for positive numbers, 0 for 0)
template <class T>
int sgn(T value)
{
	return (value > T(0)) - (value < T(0));
}



// mpt::rshift_signed
// mpt::lshift_signed
// Shift a signed integer value in a well-defined manner.
// Does the same thing as MSVC would do. This is verified by the test suite.

namespace mpt
{

template <typename T>
MPT_FORCEINLINE auto rshift_signed_standard(T x, int y) -> decltype(x >> y)
{
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::numeric_limits<T>::is_signed);
	typedef decltype(x >> y) result_type;
	typedef typename std::make_unsigned<result_type>::type unsigned_result_type;
	const unsigned_result_type roffset = static_cast<unsigned_result_type>(1) << ((sizeof(result_type) * 8) - 1);
	result_type rx = x;
	unsigned_result_type urx = static_cast<unsigned_result_type>(rx);
	urx += roffset;
	urx >>= y;
	urx -= roffset >> y;
	return static_cast<result_type>(urx);
}

template <typename T>
MPT_FORCEINLINE auto lshift_signed_standard(T x, int y) -> decltype(x << y)
{
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::numeric_limits<T>::is_signed);
	typedef decltype(x << y) result_type;
	typedef typename std::make_unsigned<result_type>::type unsigned_result_type;
	const unsigned_result_type roffset = static_cast<unsigned_result_type>(1) << ((sizeof(result_type) * 8) - 1);
	result_type rx = x;
	unsigned_result_type urx = static_cast<unsigned_result_type>(rx);
	urx += roffset;
	urx <<= y;
	urx -= roffset << y;
	return static_cast<result_type>(urx);
}

#if MPT_COMPILER_SHIFT_SIGNED

template <typename T>
MPT_FORCEINLINE auto rshift_signed_undefined(T x, int y) -> decltype(x >> y)
{
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::numeric_limits<T>::is_signed);
	return x >> y;
}

template <typename T>
MPT_FORCEINLINE auto lshift_signed_undefined(T x, int y) -> decltype(x << y)
{
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::numeric_limits<T>::is_signed);
	return x << y;
}

template <typename T>
MPT_FORCEINLINE auto rshift_signed(T x, int y) -> decltype(x >> y)
{
	return mpt::rshift_signed_undefined(x, y);
}

template <typename T>
MPT_FORCEINLINE auto lshift_signed(T x, int y) -> decltype(x << y)
{
	return mpt::lshift_signed_undefined(x, y);
}

#else

template <typename T>
MPT_FORCEINLINE auto rshift_signed(T x, int y) -> decltype(x >> y)
{
	return mpt::rshift_signed_standard(x, y);
}

template <typename T>
MPT_FORCEINLINE auto lshift_signed(T x, int y) -> decltype(x << y)
{
	return mpt::lshift_signed_standard(x, y);
}

#endif

template<typename>
struct array_size;

template <typename T, std::size_t N>
struct array_size<std::array<T, N>>
{
	static constexpr std::size_t size = N;
};

template <typename T, std::size_t N>
struct array_size<T[N]>
{
	static constexpr std::size_t size = N;
};

}  // namespace mpt

namespace Util
{

	// Returns maximum value of given integer type.
	template <class T> constexpr T MaxValueOfType(const T&) {static_assert(std::numeric_limits<T>::is_integer == true, "Only integer types are allowed."); return (std::numeric_limits<T>::max)();}

}  // namespace Util

namespace mpt
{

#if MPT_OS_DJGPP

	inline double round(const double& val) { return ::round(val); }
	inline float round(const float& val) { return ::roundf(val); }

#else // !MPT_OS_DJGPP

	// C++11 std::round
	using std::round;

#endif // MPT_OS_DJGPP


	// Rounds given double value to nearest integer value of type T.
	// Out-of-range values are saturated to the specified integer type's limits.
	template <class T> inline T saturate_round(double val)
	{
		static_assert(std::numeric_limits<T>::is_integer == true, "Type is a not an integer");
		return mpt::saturate_cast<T>(mpt::round(val));
	}

	template <class T> inline T saturate_round(float val)
	{
		static_assert(std::numeric_limits<T>::is_integer == true, "Type is a not an integer");
		return mpt::saturate_cast<T>(mpt::round(val));
	}

}


namespace Util {

	// Multiply two 32-bit integers, receive 64-bit result.
	// MSVC generates unnecessarily complicated code for the unoptimized variant using _allmul.
	MPT_FORCEINLINE int64 mul32to64(int32 a, int32 b)
	{
#if MPT_COMPILER_MSVC && (defined(_M_IX86) || defined(_M_X64))
		return __emul(a, b);
#else
		return static_cast<int64>(a) * b;
#endif
	}

	MPT_FORCEINLINE uint64 mul32to64_unsigned(uint32 a, uint32 b)
	{
#if MPT_COMPILER_MSVC && (defined(_M_IX86) || defined(_M_X64))
		return __emulu(a, b);
#else
		return static_cast<uint64>(a) * b;
#endif
	}

	MPT_FORCEINLINE int32 muldiv(int32 a, int32 b, int32 c)
	{
		return mpt::saturate_cast<int32>( mul32to64( a, b ) / c );
	}

	MPT_FORCEINLINE int32 muldivr(int32 a, int32 b, int32 c)
	{
		return mpt::saturate_cast<int32>( ( mul32to64( a, b ) + ( c / 2 ) ) / c );
	}

	// Do not use overloading because catching unsigned version by accident results in slower X86 code.
	MPT_FORCEINLINE uint32 muldiv_unsigned(uint32 a, uint32 b, uint32 c)
	{
		return mpt::saturate_cast<uint32>( mul32to64_unsigned( a, b ) / c );
	}
	MPT_FORCEINLINE uint32 muldivr_unsigned(uint32 a, uint32 b, uint32 c)
	{
		return mpt::saturate_cast<uint32>( ( mul32to64_unsigned( a, b ) + ( c / 2u ) ) / c );
	}

	MPT_FORCEINLINE int32 muldivrfloor(int64 a, uint32 b, uint32 c)
	{
		a *= b;
		a += c / 2u;
		return (a >= 0) ? mpt::saturate_cast<int32>(a / c) : mpt::saturate_cast<int32>((a - (c - 1)) / c);
	}

	// rounds x up to multiples of target
	template <typename T>
	inline T AlignUp(T x, T target)
	{
		return ((x + (target - 1)) / target) * target;
	}

	// rounds x down to multiples of target
	template <typename T>
	inline T AlignDown(T x, T target)
	{
		return (x / target) * target;
	}

} // namespace Util



OPENMPT_NAMESPACE_END
