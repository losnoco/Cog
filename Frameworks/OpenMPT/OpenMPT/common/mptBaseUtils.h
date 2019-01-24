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




namespace mpt {

//  GCC 4.5 and up provides templated overloads of std::abs that convert
// integer type narrower than int to double.
//  This is fixed as of GCC 7.1.
//  As this is apparently valid by the current standard, Library Working Group
// Issue #2735 has been filed (see
// <https://cplusplus.github.io/LWG/lwg-defects.html#2735>).
//  In any case, avoid this insanity and provide our own mpt::abs implementation
// for signed integer and floating point types.
//  Note: We stick to a C++98-style implementation only overloading int and
// greater types in order to keep promotion rules consistent for narrower types,
// which a templated version returning the argument type would not do. OpenMPT
// probably assumes this semantic when calling abs(int8) in various places.
inline int abs(int x)
{
	return std::abs(x);
}
inline long abs(long x)
{
	return std::abs(x);
}
inline long long abs(long long x)
{
	return std::abs(x);
}
inline float abs(float x)
{
	return std::fabs(x);
}
inline double abs(double x)
{
	return std::fabs(x);
}
inline long double abs(long double x)
{
	return std::fabs(x);
}

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
	STATIC_ASSERT(std::numeric_limits<Tdst>::is_integer);
	STATIC_ASSERT(std::numeric_limits<Tsrc>::is_integer);
	MPT_CONSTANT_IF(std::numeric_limits<Tdst>::is_signed && std::numeric_limits<Tsrc>::is_signed)
	{
		MPT_CONSTANT_IF(sizeof(Tdst) >= sizeof(Tsrc))
		{
			return static_cast<Tdst>(src);
		} else
		{
			return static_cast<Tdst>(std::max<Tsrc>(static_cast<Tsrc>(std::numeric_limits<Tdst>::min()), std::min<Tsrc>(src, static_cast<Tsrc>(std::numeric_limits<Tdst>::max()))));
		}
	} else MPT_CONSTANT_IF(!std::numeric_limits<Tdst>::is_signed && !std::numeric_limits<Tsrc>::is_signed)
	{
		MPT_CONSTANT_IF(sizeof(Tdst) >= sizeof(Tsrc))
		{
			return static_cast<Tdst>(src);
		} else
		{
			return static_cast<Tdst>(std::min<Tsrc>(src, static_cast<Tsrc>(std::numeric_limits<Tdst>::max())));
		}
	} else MPT_CONSTANT_IF(std::numeric_limits<Tdst>::is_signed && !std::numeric_limits<Tsrc>::is_signed)
	{
		MPT_CONSTANT_IF(sizeof(Tdst) > sizeof(Tsrc))
		{
			return static_cast<Tdst>(src);
		} else MPT_CONSTANT_IF(sizeof(Tdst) == sizeof(Tsrc))
		{
			return static_cast<Tdst>(std::min<Tsrc>(src, static_cast<Tsrc>(std::numeric_limits<Tdst>::max())));
		} else
		{
			return static_cast<Tdst>(std::min<Tsrc>(src, static_cast<Tsrc>(std::numeric_limits<Tdst>::max())));
		}
	} else // Tdst unsigned, Tsrc signed
	{
		MPT_CONSTANT_IF(sizeof(Tdst) >= sizeof(Tsrc))
		{
			return static_cast<Tdst>(std::max<Tsrc>(0, src));
		} else
		{
			return static_cast<Tdst>(std::max<Tsrc>(0, std::min<Tsrc>(src, static_cast<Tsrc>(std::numeric_limits<Tdst>::max()))));
		}
	}
}

template <typename Tdst>
inline Tdst saturate_cast(double src)
{
	if(src >= std::numeric_limits<Tdst>::max())
	{
		return std::numeric_limits<Tdst>::max();
	}
	if(src <= std::numeric_limits<Tdst>::min())
	{
		return std::numeric_limits<Tdst>::min();
	}
	return static_cast<Tdst>(src);
}

template <typename Tdst>
inline Tdst saturate_cast(float src)
{
	if(src >= std::numeric_limits<Tdst>::max())
	{
		return std::numeric_limits<Tdst>::max();
	}
	if(src <= std::numeric_limits<Tdst>::min())
	{
		return std::numeric_limits<Tdst>::min();
	}
	return static_cast<Tdst>(src);
}


template <typename T>
MPT_CONSTEXPR14_FUN std::size_t weight(T val) noexcept
{
	MPT_STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	typedef typename std::make_unsigned<T>::type Tunsigned;
	Tunsigned uval = static_cast<Tunsigned>(val);
	std::size_t result = 0;
	while(uval > 0)
	{
		if(uval & 0x1)
		{
			result++;
		}
		uval >>= 1;
	}
	return result;
}

#if MPT_CXX_AT_LEAST(20)

using std::ispow2;
using std::ceil2;
using std::floor2;
using std::log2p1;

#else

// C++20 <bit> header.
// Note that we do not use SFINAE here but instead rely on static_assert.
// Also note that for C++11 compilers, these functions are not constexpr.
// They could be implemented recursively to make them C++11 constexpr compatible if needed.

template <typename T>
MPT_CONSTEXPR14_FUN bool ispow2(T x) noexcept
{
	MPT_STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	MPT_STATIC_ASSERT(std::is_unsigned<T>::value);
	return mpt::weight(x) == 1;
}

template <typename T>
MPT_CONSTEXPR14_FUN T ceil2(T x) noexcept
{
	MPT_STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	MPT_STATIC_ASSERT(std::is_unsigned<T>::value);
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
MPT_CONSTEXPR14_FUN T floor2(T x) noexcept
{
	MPT_STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	MPT_STATIC_ASSERT(std::is_unsigned<T>::value);
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
MPT_CONSTEXPR14_FUN T log2p1(T x) noexcept
{
	MPT_STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	MPT_STATIC_ASSERT(std::is_unsigned<T>::value);
	T result = 0;
	while(x > 0)
	{
		x >>= 1;
		result += 1;
	}
	return result;
}

#endif

} // namespace mpt


#if defined(MODPLUG_TRACKER)
// Tracker code requires MIN/MAX to work in constexpr contexts.
// We could make MIN/MAX constexpr for supporting compilers,
// but that would just needlessly complicate the support matrix
// for now.
#ifndef MPT_MINMAX_MACROS
#define MPT_MINMAX_MACROS
#endif
#endif

#if MPT_COMPILER_MSVC
// MSVC disables a bunch of type conversion warnings once a macro is involved.
// Replacing the macro with a template thus spews a TON OF WARNINGS for now.
#ifndef MPT_MINMAX_MACROS
#define MPT_MINMAX_MACROS
#endif
#endif

#if defined(MPT_MINMAX_MACROS)

#define MAX(a,b) (((a) > (b)) ? (a) : (b))

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#else

namespace mpt { namespace Legacy {

template <typename Ta, typename Tb>
MPT_FORCEINLINE auto MAX(const Ta &a, const Tb &b) -> decltype((a>b)?a:b)
{
	return (a > b) ? a : b;
}

template <typename Ta, typename Tb>
MPT_FORCEINLINE auto MIN(const Ta &a, const Tb &b) -> decltype((a<b)?a:b)
{
	return (a < b) ? a : b;
}

} } // namespace mpt::Legacy

using namespace mpt::Legacy;

#endif


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
		STATIC_ASSERT(std::numeric_limits<Tmod>::is_integer);
		STATIC_ASSERT(!std::numeric_limits<Tmod>::is_signed);
		STATIC_ASSERT(std::numeric_limits<Tval>::is_integer);
		STATIC_ASSERT(!std::numeric_limits<Tval>::is_signed);
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
	T add = std::min<T>(x >> 1, std::numeric_limits<T>::max() - x);
	return std::min<T>(x + add, mpt::saturate_cast<T>(limit));
}
									
template <typename T>
inline T ExponentialGrow(const T &x)
{
	return Util::ExponentialGrow(x, std::numeric_limits<T>::max());
}

} //namespace Util


namespace mpt
{

// C++17 clamp

#if MPT_CXX_AT_LEAST(17)

using std::clamp;

#else

template<typename T, typename Compare>
MPT_CONSTEXPR11_FUN const T & clamp(const T & v, const T & lo, const T & hi, Compare comp)
{
	return comp(v, lo) ? lo : comp(hi, v) ? hi : v;
}

template<typename T>
MPT_CONSTEXPR11_FUN const T & clamp(const T & v, const T & lo, const T & hi)
{
	return mpt::clamp(v, lo, hi, std::less<T>());
}

#endif

} // namespace mpt


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
	MPT_STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	MPT_STATIC_ASSERT(std::numeric_limits<T>::is_signed);
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
	MPT_STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	MPT_STATIC_ASSERT(std::numeric_limits<T>::is_signed);
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
	MPT_STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	MPT_STATIC_ASSERT(std::numeric_limits<T>::is_signed);
	return x >> y;
}

template <typename T>
MPT_FORCEINLINE auto lshift_signed_undefined(T x, int y) -> decltype(x << y)
{
	MPT_STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	MPT_STATIC_ASSERT(std::numeric_limits<T>::is_signed);
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

} // namespace mpt



namespace Util
{

	// Returns maximum value of given integer type.
	template <class T> constexpr T MaxValueOfType(const T&) {static_assert(std::numeric_limits<T>::is_integer == true, "Only integer types are allowed."); return (std::numeric_limits<T>::max)();}

}


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



namespace mpt
{

#if MPT_CXX_AT_LEAST(17)

using std::gcd;
using std::lcm;

#else

	// Greatest Common Divisor. Always returns non-negative number.
	// compatible with C++17 std::gcd
	template <typename A, typename B>
	inline typename std::common_type<A, B>::type gcd(A a_, B b_)
	{
		typename std::common_type<A, B>::type a = a_;
		typename std::common_type<A, B>::type b = b_;
		if(a < 0)
			a = -a;
		if(b < 0)
			b = -b;
		for(;;)
		{
			if(a == 0)
				return b;
			b %= a;
			if(b == 0)
				return a;
			a %= b;
		}
	}

	// Least Common Multiple. Always returns non-negative number.
	// compatible with C++17 std::lcm
	template <typename A, typename B>
	inline typename std::common_type<A, B>::type lcm(A a_, B b_)
	{
		typename std::common_type<A, B>::type a = a_;
		typename std::common_type<A, B>::type b = b_;
		if(a < 0)
			a = -a;
		if(b < 0)
			b = -b;
		if((a | b) == 0)
			return 0;
		return a / mpt::gcd(a, b) * b;
	}

#endif

} // namespace mpt



OPENMPT_NAMESPACE_END
