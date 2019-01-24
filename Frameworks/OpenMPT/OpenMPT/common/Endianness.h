/*
 * Endianness.h
 * ------------
 * Purpose: Code for deadling with endianness.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include <array>
#include <limits>

#include <cmath>
#include <cstdlib>

#include <math.h>
#include <stdlib.h>

#if MPT_COMPILER_MSVC
#include <intrin.h>
#endif



#if MPT_CXX_AT_LEAST(20)

// nothing

#elif MPT_COMPILER_GENERIC

// rely on runtime detection instead of using non-standard macros

#else

#if MPT_COMPILER_MSVC
	#define MPT_PLATFORM_LITTLE_ENDIAN
#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG
	#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
		#define MPT_PLATFORM_BIG_ENDIAN
	#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
		#define MPT_PLATFORM_LITTLE_ENDIAN
	#endif
#endif

// fallback:
#if !defined(MPT_PLATFORM_BIG_ENDIAN) && !defined(MPT_PLATFORM_LITTLE_ENDIAN)
	// taken from boost/detail/endian.hpp
	#if (defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)) \
		|| (defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)) \
		|| (defined(_STLP_BIG_ENDIAN) && !defined(_STLP_LITTLE_ENDIAN))
			#define MPT_PLATFORM_BIG_ENDIAN
	#elif (defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)) \
		|| (defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)) \
		|| (defined(_STLP_LITTLE_ENDIAN) && !defined(_STLP_BIG_ENDIAN))
			#define MPT_PLATFORM_LITTLE_ENDIAN
	#elif defined(__sparc) || defined(__sparc__) \
		|| defined(_POWER) || defined(__powerpc__) \
		|| defined(__ppc__) || defined(__hpux) || defined(__hppa) \
		|| defined(_MIPSEB) || defined(_POWER) \
		|| defined(__s390__)
			#define MPT_PLATFORM_BIG_ENDIAN
	#elif defined(__i386__) || defined(__alpha__) \
		|| defined(__ia64) || defined(__ia64__) \
		|| defined(_M_IX86) || defined(_M_IA64) \
		|| defined(_M_ALPHA) || defined(__amd64) \
		|| defined(__amd64__) || defined(_M_AMD64) \
		|| defined(__x86_64) || defined(__x86_64__) \
		|| defined(_M_X64) || defined(__bfin__)
			#define MPT_PLATFORM_LITTLE_ENDIAN
	#endif
#endif

#endif 

#if defined(MPT_PLATFORM_BIG_ENDIAN) || defined(MPT_PLATFORM_LITTLE_ENDIAN)
#define MPT_PLATFORM_ENDIAN_KNOWN 1
#else
#define MPT_PLATFORM_ENDIAN_KNOWN 0
#endif



#if MPT_PLATFORM_ENDIAN_KNOWN && MPT_CXX_AT_LEAST(14)
//#define MPT_ENDIAN_IS_CONSTEXPR 1
// For now, we do not want to use constexpr endianness functions and types.
// It bloats the binary size somewhat (possibly because of either the zeroing
// constructor or because of not being able to use byteswap intrinsics) and has
// currently no compelling benefit for us
#define MPT_ENDIAN_IS_CONSTEXPR 0
#else
#define MPT_ENDIAN_IS_CONSTEXPR 0
#endif

#if MPT_ENDIAN_IS_CONSTEXPR
#define MPT_ENDIAN_CONSTEXPR_FUN MPT_CONSTEXPR14_FUN
#define MPT_ENDIAN_CONSTEXPR_VAR MPT_CONSTEXPR14_VAR
#else
#define MPT_ENDIAN_CONSTEXPR_FUN MPT_FORCEINLINE
#define MPT_ENDIAN_CONSTEXPR_VAR const
#endif



OPENMPT_NAMESPACE_BEGIN



namespace mpt {



// C++20 std::endian
#if MPT_CXX_AT_LEAST(20)
using std::endian;
#else // !C++20
enum class endian
{
	little = 0x78563412u,
	big    = 0x12345678u,
	weird  = 1u,
#if MPT_PLATFORM_ENDIAN_KNOWN && defined(MPT_PLATFORM_LITTLE_ENDIAN)
	native = little
#elif MPT_PLATFORM_ENDIAN_KNOWN && defined(MPT_PLATFORM_BIG_ENDIAN)
	native = big
#else
	native = 0u
#endif
};
#endif // C++20

MPT_CONSTEXPR11_FUN bool endian_known() noexcept
{
	return ((mpt::endian::native == mpt::endian::little) || (mpt::endian::native == mpt::endian::big));
}

MPT_CONSTEXPR11_FUN bool endian_unknown() noexcept
{
	return ((mpt::endian::native != mpt::endian::little) && (mpt::endian::native != mpt::endian::big));
}



#if MPT_CXX_AT_LEAST(20)

static MPT_CONSTEXPR11_FUN mpt::endian get_endian() noexcept
{
	return mpt::endian::native;
}

static MPT_CONSTEXPR11_FUN bool endian_is_little() noexcept
{
	return get_endian() == mpt::endian::little;
}

static MPT_CONSTEXPR11_FUN bool endian_is_big() noexcept
{
	return get_endian() == mpt::endian::big;
}

static MPT_CONSTEXPR11_FUN bool endian_is_weird() noexcept
{
	return !endian_is_little() && !endian_is_big();
}

#else // !C++20

namespace detail {

	static MPT_FORCEINLINE mpt::endian endian_probe() noexcept
	{
		typedef uint32 endian_probe_type;
		MPT_STATIC_ASSERT(sizeof(endian_probe_type) == 4);
		constexpr endian_probe_type endian_probe_big    = 0x12345678u;
		constexpr endian_probe_type endian_probe_little = 0x78563412u;
		const mpt::byte probe[sizeof(endian_probe_type)] = { mpt::as_byte(0x12), mpt::as_byte(0x34), mpt::as_byte(0x56), mpt::as_byte(0x78) };
		endian_probe_type test;
		std::memcpy(&test, probe, sizeof(endian_probe_type));
		mpt::endian result = mpt::endian::native;
		switch(test)
		{
			case endian_probe_big:
				result = mpt::endian::big;
				break;
			case endian_probe_little:
				result = mpt::endian::little;
				break;
			default:
				result = mpt::endian::weird;
				break;
		}
		return result;
	}

}

static MPT_FORCEINLINE mpt::endian get_endian() noexcept
{
	MPT_CONSTANT_IF(mpt::endian_known())
	{
		return mpt::endian::native;
	} else
	{
		return detail::endian_probe();
	}
}

static MPT_FORCEINLINE bool endian_is_little() noexcept
{
	return get_endian() == mpt::endian::little;
}

static MPT_FORCEINLINE bool endian_is_big() noexcept
{
	return get_endian() == mpt::endian::big;
}

static MPT_FORCEINLINE bool endian_is_weird() noexcept
{
	return !endian_is_little() && !endian_is_big();
}

#endif // C++20



} // namespace mpt



struct BigEndian_tag
{
	static MPT_CONSTEXPR11_VAR mpt::endian endian = mpt::endian::big;
};

struct LittleEndian_tag
{
	static MPT_CONSTEXPR11_VAR mpt::endian endian = mpt::endian::little;
};



namespace mpt {

template <typename Tbyte>
inline void SwapBufferEndian(std::size_t elementSize, Tbyte * buffer, std::size_t elements)
{
	MPT_STATIC_ASSERT(sizeof(Tbyte) == 1);
	for(std::size_t element = 0; element < elements; ++element)
	{
		std::reverse(&buffer[0], &buffer[elementSize]);
		buffer += elementSize;
	}
}

} // namespace mpt



#if !MPT_ENDIAN_IS_CONSTEXPR

#if MPT_COMPILER_GCC
#define MPT_bswap16 __builtin_bswap16
#define MPT_bswap32 __builtin_bswap32
#define MPT_bswap64 __builtin_bswap64
#elif MPT_COMPILER_MSVC
#define MPT_bswap16 _byteswap_ushort
#define MPT_bswap32 _byteswap_ulong
#define MPT_bswap64 _byteswap_uint64
#endif

namespace mpt { namespace detail {
// catch system macros
#ifndef MPT_bswap16
#ifdef bswap16
static MPT_FORCEINLINE uint16 mpt_bswap16(uint16 x) { return bswap16(x); }
#define MPT_bswap16 mpt::detail::mpt_bswap16
#endif
#endif
#ifndef MPT_bswap32
#ifdef bswap32
static MPT_FORCEINLINE uint32 mpt_bswap32(uint32 x) { return bswap32(x); }
#define MPT_bswap32 mpt::detail::mpt_bswap32
#endif
#endif
#ifndef MPT_bswap64
#ifdef bswap64
static MPT_FORCEINLINE uint64 mpt_bswap64(uint64 x) { return bswap64(x); }
#define MPT_bswap64 mpt::detail::mpt_bswap64
#endif
#endif
} } // namespace mpt::detail

#endif // !MPT_ENDIAN_IS_CONSTEXPR


// No intrinsics available
#ifndef MPT_bswap16
#define MPT_bswap16(x) \
	( uint16(0) \
		| ((static_cast<uint16>(x) >> 8) & 0x00FFu) \
		| ((static_cast<uint16>(x) << 8) & 0xFF00u) \
	) \
/**/
#endif
#ifndef MPT_bswap32
#define MPT_bswap32(x) \
	( uint32(0) \
		| ((static_cast<uint32>(x) & 0x000000FFu) << 24) \
		| ((static_cast<uint32>(x) & 0x0000FF00u) <<  8) \
		| ((static_cast<uint32>(x) & 0x00FF0000u) >>  8) \
		| ((static_cast<uint32>(x) & 0xFF000000u) >> 24) \
	) \
/**/
#endif
#ifndef MPT_bswap64
#define MPT_bswap64(x) \
	( uint64(0) \
		| (((static_cast<uint64>(x) >>  0) & 0xffull) << 56) \
		| (((static_cast<uint64>(x) >>  8) & 0xffull) << 48) \
		| (((static_cast<uint64>(x) >> 16) & 0xffull) << 40) \
		| (((static_cast<uint64>(x) >> 24) & 0xffull) << 32) \
		| (((static_cast<uint64>(x) >> 32) & 0xffull) << 24) \
		| (((static_cast<uint64>(x) >> 40) & 0xffull) << 16) \
		| (((static_cast<uint64>(x) >> 48) & 0xffull) <<  8) \
		| (((static_cast<uint64>(x) >> 56) & 0xffull) <<  0) \
	) \
/**/
#endif


template <typename T, typename Tendian, std::size_t size>
static MPT_CONSTEXPR17_FUN std::array<mpt::byte, size> EndianEncode(T val) noexcept
{
	MPT_STATIC_ASSERT(Tendian::endian == mpt::endian::little || Tendian::endian == mpt::endian::big);
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	STATIC_ASSERT(!std::numeric_limits<T>::is_signed);
	STATIC_ASSERT(sizeof(T) == size);
	typedef T base_type;
	typedef typename std::make_unsigned<base_type>::type unsigned_base_type;
	typedef Tendian endian_type;
	unsigned_base_type uval = static_cast<unsigned_base_type>(val);
	std::array<mpt::byte, size> data;
	MPT_CONSTANT_IF(endian_type::endian == mpt::endian::little)
	{
		for(std::size_t i = 0; i < sizeof(base_type); ++i)
		{
			data[i] = static_cast<mpt::byte>(static_cast<uint8>((uval >> (i*8)) & 0xffu));
		}
	} else
	{
		for(std::size_t i = 0; i < sizeof(base_type); ++i)
		{
			data[(sizeof(base_type)-1) - i] = static_cast<mpt::byte>(static_cast<uint8>((uval >> (i*8)) & 0xffu));
		}
	}
	return data;
}

template <typename T, typename Tendian, std::size_t size>
static MPT_CONSTEXPR17_FUN T EndianDecode(std::array<mpt::byte, size> data) noexcept
{
	MPT_STATIC_ASSERT(Tendian::endian == mpt::endian::little || Tendian::endian == mpt::endian::big);
	STATIC_ASSERT(std::numeric_limits<T>::is_integer);
	STATIC_ASSERT(!std::numeric_limits<T>::is_signed);
	STATIC_ASSERT(sizeof(T) == size);
	typedef T base_type;
	typedef typename std::make_unsigned<base_type>::type unsigned_base_type;
	typedef Tendian endian_type;
	base_type val = base_type();
	unsigned_base_type uval = unsigned_base_type();
	MPT_CONSTANT_IF(endian_type::endian == mpt::endian::little)
	{
		for(std::size_t i = 0; i < sizeof(base_type); ++i)
		{
			uval |= static_cast<unsigned_base_type>(static_cast<uint8>(data[i])) << (i*8);
		}
	} else
	{
		for(std::size_t i = 0; i < sizeof(base_type); ++i)
		{
			uval |= static_cast<unsigned_base_type>(static_cast<uint8>(data[(sizeof(base_type)-1) - i])) << (i*8);
		}
	}
	val = static_cast<base_type>(uval);
	return val;
}


namespace mpt
{
namespace detail
{

static MPT_ENDIAN_CONSTEXPR_FUN uint64 SwapBytes(uint64 value) noexcept { return MPT_bswap64(value); }
static MPT_ENDIAN_CONSTEXPR_FUN uint32 SwapBytes(uint32 value) noexcept { return MPT_bswap32(value); }
static MPT_ENDIAN_CONSTEXPR_FUN uint16 SwapBytes(uint16 value) noexcept { return MPT_bswap16(value); }
static MPT_ENDIAN_CONSTEXPR_FUN int64  SwapBytes(int64  value) noexcept { return MPT_bswap64(value); }
static MPT_ENDIAN_CONSTEXPR_FUN int32  SwapBytes(int32  value) noexcept { return MPT_bswap32(value); }
static MPT_ENDIAN_CONSTEXPR_FUN int16  SwapBytes(int16  value) noexcept { return MPT_bswap16(value); }

// Do NOT remove these overloads, even if they seem useless.
// We do not want risking to extend 8bit integers to int and then
// endian-converting and casting back to int.
// Thus these overloads.
static MPT_ENDIAN_CONSTEXPR_FUN uint8  SwapBytes(uint8  value) noexcept { return value; }
static MPT_ENDIAN_CONSTEXPR_FUN int8   SwapBytes(int8   value) noexcept { return value; }
static MPT_ENDIAN_CONSTEXPR_FUN char   SwapBytes(char   value) noexcept { return value; }

} // namespace detail
} // namespace mpt

#undef MPT_bswap16
#undef MPT_bswap32
#undef MPT_bswap64


// 1.0f --> 0x3f800000u
static MPT_FORCEINLINE uint32 EncodeIEEE754binary32(float32 f)
{
	MPT_CONSTANT_IF(std::numeric_limits<float32>::is_iec559 && mpt::endian_known())
	{
		return mpt::bit_cast<uint32>(f);
	} else
	{
		int e = 0;
		float m = std::frexp(f, &e);
		if(e == 0 && std::fabs(m) == 0.0f)
		{
			uint32 expo = 0u;
			uint32 sign = std::signbit(m) ? 0x01u : 0x00u;
			uint32 mant = 0u;
			uint32 i = 0u;
			i |= (mant <<  0) & 0x007fffffu;
			i |= (expo << 23) & 0x7f800000u;
			i |= (sign << 31) & 0x80000000u;
			return i;
		} else
		{
			uint32 expo = e + 127 - 1;
			uint32 sign = std::signbit(m) ? 0x01u : 0x00u;
			uint32 mant = static_cast<uint32>(std::fabs(std::ldexp(m, 24)));
			uint32 i = 0u;
			i |= (mant <<  0) & 0x007fffffu;
			i |= (expo << 23) & 0x7f800000u;
			i |= (sign << 31) & 0x80000000u;
			return i;
		}
	}
}
static MPT_FORCEINLINE uint64 EncodeIEEE754binary64(float64 f)
{
	MPT_CONSTANT_IF(std::numeric_limits<float64>::is_iec559 && mpt::endian_known())
	{
		return mpt::bit_cast<uint64>(f);
	} else
	{
		int e = 0;
		double m = std::frexp(f, &e);
		if(e == 0 && std::fabs(m) == 0.0)
		{
			uint64 expo = 0u;
			uint64 sign = std::signbit(m) ? 0x01u : 0x00u;
			uint64 mant = 0u;
			uint64 i = 0u;
			i |= (mant <<  0) & 0x000fffffffffffffull;
			i |= (expo << 52) & 0x7ff0000000000000ull;
			i |= (sign << 63) & 0x8000000000000000ull;
			return i;
		} else
		{
			uint64 expo = e + 1023 - 1;
			uint64 sign = std::signbit(m) ? 0x01u : 0x00u;
			uint64 mant = static_cast<uint64>(std::fabs(std::ldexp(m, 53)));
			uint64 i = 0u;
			i |= (mant <<  0) & 0x000fffffffffffffull;
			i |= (expo << 52) & 0x7ff0000000000000ull;
			i |= (sign << 63) & 0x8000000000000000ull;
			return i;
		}
	}
}

// 0x3f800000u --> 1.0f
static MPT_FORCEINLINE float32 DecodeIEEE754binary32(uint32 i)
{
	MPT_CONSTANT_IF(std::numeric_limits<float32>::is_iec559 && mpt::endian_known())
	{
		return mpt::bit_cast<float32>(i);
	} else
	{
		uint32 mant = (i & 0x007fffffu) >>  0;
		uint32 expo = (i & 0x7f800000u) >> 23;
		uint32 sign = (i & 0x80000000u) >> 31;
		if(expo == 0)
		{
			float m = sign ? -static_cast<float>(mant) : static_cast<float>(mant);
			int e = static_cast<int>(expo) - 127 + 1 - 24;
			float f = std::ldexp(m, e);
			return static_cast<float32>(f);
		} else
		{
			mant |= 0x00800000u;
			float m = sign ? -static_cast<float>(mant) : static_cast<float>(mant);
			int e = static_cast<int>(expo) - 127 + 1 - 24;
			float f = std::ldexp(m, e);
			return static_cast<float32>(f);
		}
	}
}
static MPT_FORCEINLINE float64 DecodeIEEE754binary64(uint64 i)
{
	MPT_CONSTANT_IF(std::numeric_limits<float64>::is_iec559 && mpt::endian_known())
	{
		return mpt::bit_cast<float64>(i);
	} else
	{
		uint64 mant = (i & 0x000fffffffffffffull) >>  0;
		uint64 expo = (i & 0x7ff0000000000000ull) >> 52;
		uint64 sign = (i & 0x8000000000000000ull) >> 63;
		if(expo == 0)
		{
			double m = sign ? -static_cast<double>(mant) : static_cast<double>(mant);
			int e = static_cast<int>(expo) - 1023 + 1 - 53;
			double f = std::ldexp(m, e);
			return static_cast<float64>(f);
		} else
		{
			mant |= 0x0010000000000000ull;
			double m = sign ? -static_cast<double>(mant) : static_cast<double>(mant);
			int e = static_cast<int>(expo) - 1023 + 1 - 53;
			double f = std::ldexp(m, e);
			return static_cast<float64>(f);
		}
	}
}


// template parameters are byte indices corresponding to the individual bytes of iee754 in memory
template<std::size_t hihi, std::size_t hilo, std::size_t lohi, std::size_t lolo>
struct IEEE754binary32Emulated
{
public:
	typedef IEEE754binary32Emulated<hihi,hilo,lohi,lolo> self_t;
	mpt::byte bytes[4];
public:
	MPT_FORCEINLINE mpt::byte GetByte(std::size_t i) const
	{
		return bytes[i];
	}
	MPT_FORCEINLINE IEEE754binary32Emulated() { }
	MPT_FORCEINLINE explicit IEEE754binary32Emulated(float32 f)
	{
		SetInt32(EncodeIEEE754binary32(f));
	}
	// b0...b3 are in memory order, i.e. depend on the endianness of this type
	// little endian: (0x00,0x00,0x80,0x3f)
	// big endian:    (0x3f,0x80,0x00,0x00)
	MPT_FORCEINLINE explicit IEEE754binary32Emulated(mpt::byte b0, mpt::byte b1, mpt::byte b2, mpt::byte b3)
	{
		bytes[0] = b0;
		bytes[1] = b1;
		bytes[2] = b2;
		bytes[3] = b3;
	}
	MPT_FORCEINLINE operator float32 () const
	{
		return DecodeIEEE754binary32(GetInt32());
	}
	MPT_FORCEINLINE self_t & SetInt32(uint32 i)
	{
		bytes[hihi] = static_cast<mpt::byte>(i >> 24);
		bytes[hilo] = static_cast<mpt::byte>(i >> 16);
		bytes[lohi] = static_cast<mpt::byte>(i >>  8);
		bytes[lolo] = static_cast<mpt::byte>(i >>  0);
		return *this;
	}
	MPT_FORCEINLINE uint32 GetInt32() const
	{
		return 0u
			| (static_cast<uint32>(bytes[hihi]) << 24)
			| (static_cast<uint32>(bytes[hilo]) << 16)
			| (static_cast<uint32>(bytes[lohi]) <<  8)
			| (static_cast<uint32>(bytes[lolo]) <<  0)
			;
	}
	MPT_FORCEINLINE bool operator == (const self_t &cmp) const
	{
		return true
			&& bytes[0] == cmp.bytes[0]
			&& bytes[1] == cmp.bytes[1]
			&& bytes[2] == cmp.bytes[2]
			&& bytes[3] == cmp.bytes[3]
			;
	}
	MPT_FORCEINLINE bool operator != (const self_t &cmp) const
	{
		return !(*this == cmp);
	}
};
template<std::size_t hihihi, std::size_t hihilo, std::size_t hilohi, std::size_t hilolo, std::size_t lohihi, std::size_t lohilo, std::size_t lolohi, std::size_t lololo>
struct IEEE754binary64Emulated
{
public:
	typedef IEEE754binary64Emulated<hihihi,hihilo,hilohi,hilolo,lohihi,lohilo,lolohi,lololo> self_t;
	mpt::byte bytes[8];
public:
	MPT_FORCEINLINE mpt::byte GetByte(std::size_t i) const
	{
		return bytes[i];
	}
	MPT_FORCEINLINE IEEE754binary64Emulated() { }
	MPT_FORCEINLINE explicit IEEE754binary64Emulated(float64 f)
	{
		SetInt64(EncodeIEEE754binary64(f));
	}
	MPT_FORCEINLINE explicit IEEE754binary64Emulated(mpt::byte b0, mpt::byte b1, mpt::byte b2, mpt::byte b3, mpt::byte b4, mpt::byte b5, mpt::byte b6, mpt::byte b7)
	{
		bytes[0] = b0;
		bytes[1] = b1;
		bytes[2] = b2;
		bytes[3] = b3;
		bytes[4] = b4;
		bytes[5] = b5;
		bytes[6] = b6;
		bytes[7] = b7;
	}
	MPT_FORCEINLINE operator float64 () const
	{
		return DecodeIEEE754binary64(GetInt64());
	}
	MPT_FORCEINLINE self_t & SetInt64(uint64 i)
	{
		bytes[hihihi] = static_cast<mpt::byte>(i >> 56);
		bytes[hihilo] = static_cast<mpt::byte>(i >> 48);
		bytes[hilohi] = static_cast<mpt::byte>(i >> 40);
		bytes[hilolo] = static_cast<mpt::byte>(i >> 32);
		bytes[lohihi] = static_cast<mpt::byte>(i >> 24);
		bytes[lohilo] = static_cast<mpt::byte>(i >> 16);
		bytes[lolohi] = static_cast<mpt::byte>(i >>  8);
		bytes[lololo] = static_cast<mpt::byte>(i >>  0);
		return *this;
	}
	MPT_FORCEINLINE uint64 GetInt64() const
	{
		return 0u
			| (static_cast<uint64>(bytes[hihihi]) << 56)
			| (static_cast<uint64>(bytes[hihilo]) << 48)
			| (static_cast<uint64>(bytes[hilohi]) << 40)
			| (static_cast<uint64>(bytes[hilolo]) << 32)
			| (static_cast<uint64>(bytes[lohihi]) << 24)
			| (static_cast<uint64>(bytes[lohilo]) << 16)
			| (static_cast<uint64>(bytes[lolohi]) <<  8)
			| (static_cast<uint64>(bytes[lololo]) <<  0)
			;
	}
	MPT_FORCEINLINE bool operator == (const self_t &cmp) const
	{
		return true
			&& bytes[0] == cmp.bytes[0]
			&& bytes[1] == cmp.bytes[1]
			&& bytes[2] == cmp.bytes[2]
			&& bytes[3] == cmp.bytes[3]
			&& bytes[4] == cmp.bytes[4]
			&& bytes[5] == cmp.bytes[5]
			&& bytes[6] == cmp.bytes[6]
			&& bytes[7] == cmp.bytes[7]
			;
	}
	MPT_FORCEINLINE bool operator != (const self_t &cmp) const
	{
		return !(*this == cmp);
	}
};

typedef IEEE754binary32Emulated<0,1,2,3> IEEE754binary32EmulatedBE;
typedef IEEE754binary32Emulated<3,2,1,0> IEEE754binary32EmulatedLE;
typedef IEEE754binary64Emulated<0,1,2,3,4,5,6,7> IEEE754binary64EmulatedBE;
typedef IEEE754binary64Emulated<7,6,5,4,3,2,1,0> IEEE754binary64EmulatedLE;

MPT_BINARY_STRUCT(IEEE754binary32EmulatedBE, 4)
MPT_BINARY_STRUCT(IEEE754binary32EmulatedLE, 4)
MPT_BINARY_STRUCT(IEEE754binary64EmulatedBE, 8)
MPT_BINARY_STRUCT(IEEE754binary64EmulatedLE, 8)

template <mpt::endian endian = mpt::endian::native>
struct IEEE754binary32Native
{
public:
	float32 value;
public:
	MPT_FORCEINLINE mpt::byte GetByte(std::size_t i) const
	{
		MPT_STATIC_ASSERT(endian == mpt::endian::little || endian == mpt::endian::big);
		MPT_CONSTANT_IF(endian == mpt::endian::little)
		{
			return static_cast<mpt::byte>(EncodeIEEE754binary32(value) >> (i*8));
		}
		MPT_CONSTANT_IF(endian == mpt::endian::big)
		{
			return static_cast<mpt::byte>(EncodeIEEE754binary32(value) >> ((4-1-i)*8));
		}
	}
	MPT_FORCEINLINE IEEE754binary32Native() { }
	MPT_FORCEINLINE explicit IEEE754binary32Native(float32 f)
	{
		value = f;
	}
	// b0...b3 are in memory order, i.e. depend on the endianness of this type
	// little endian: (0x00,0x00,0x80,0x3f)
	// big endian:    (0x3f,0x80,0x00,0x00)
	MPT_FORCEINLINE explicit IEEE754binary32Native(mpt::byte b0, mpt::byte b1, mpt::byte b2, mpt::byte b3)
	{
		MPT_STATIC_ASSERT(endian == mpt::endian::little || endian == mpt::endian::big);
		MPT_CONSTANT_IF(endian == mpt::endian::little)
		{
			value = DecodeIEEE754binary32(0u
				| (static_cast<uint32>(b0) <<  0)
				| (static_cast<uint32>(b1) <<  8)
				| (static_cast<uint32>(b2) << 16)
				| (static_cast<uint32>(b3) << 24)
				);
		}
		MPT_CONSTANT_IF(endian == mpt::endian::big)
		{
			value = DecodeIEEE754binary32(0u
				| (static_cast<uint32>(b0) << 24)
				| (static_cast<uint32>(b1) << 16)
				| (static_cast<uint32>(b2) <<  8)
				| (static_cast<uint32>(b3) <<  0)
				);
		}
	}
	MPT_FORCEINLINE operator float32 () const
	{
		return value;
	}
	MPT_FORCEINLINE IEEE754binary32Native & SetInt32(uint32 i)
	{
		value = DecodeIEEE754binary32(i);
		return *this;
	}
	MPT_FORCEINLINE uint32 GetInt32() const
	{
		return EncodeIEEE754binary32(value);
	}
	MPT_FORCEINLINE bool operator == (const IEEE754binary32Native &cmp) const
	{
		return value == cmp.value;
	}
	MPT_FORCEINLINE bool operator != (const IEEE754binary32Native &cmp) const
	{
		return value != cmp.value;
	}
};

template <mpt::endian endian = mpt::endian::native>
struct IEEE754binary64Native
{
public:
	float64 value;
public:
	MPT_FORCEINLINE mpt::byte GetByte(std::size_t i) const
	{
		MPT_STATIC_ASSERT(endian == mpt::endian::little || endian == mpt::endian::big);
		MPT_CONSTANT_IF(endian == mpt::endian::little)
		{
			return mpt::byte_cast<mpt::byte>(static_cast<uint8>(EncodeIEEE754binary64(value) >> (i*8)));
		}
		MPT_CONSTANT_IF(endian == mpt::endian::big)
		{
			return mpt::byte_cast<mpt::byte>(static_cast<uint8>(EncodeIEEE754binary64(value) >> ((8-1-i)*8)));
		}
	}
	MPT_FORCEINLINE IEEE754binary64Native() { }
	MPT_FORCEINLINE explicit IEEE754binary64Native(float64 f)
	{
		value = f;
	}
	MPT_FORCEINLINE explicit IEEE754binary64Native(mpt::byte b0, mpt::byte b1, mpt::byte b2, mpt::byte b3, mpt::byte b4, mpt::byte b5, mpt::byte b6, mpt::byte b7)
	{
		MPT_STATIC_ASSERT(endian == mpt::endian::little || endian == mpt::endian::big);
		MPT_CONSTANT_IF(endian == mpt::endian::little)
		{
			value = DecodeIEEE754binary64(0ull
				| (static_cast<uint64>(b0) <<  0)
				| (static_cast<uint64>(b1) <<  8)
				| (static_cast<uint64>(b2) << 16)
				| (static_cast<uint64>(b3) << 24)
				| (static_cast<uint64>(b4) << 32)
				| (static_cast<uint64>(b5) << 40)
				| (static_cast<uint64>(b6) << 48)
				| (static_cast<uint64>(b7) << 56)
				);
		}
		MPT_CONSTANT_IF(endian == mpt::endian::big)
		{
			value = DecodeIEEE754binary64(0ull
				| (static_cast<uint64>(b0) << 56)
				| (static_cast<uint64>(b1) << 48)
				| (static_cast<uint64>(b2) << 40)
				| (static_cast<uint64>(b3) << 32)
				| (static_cast<uint64>(b4) << 24)
				| (static_cast<uint64>(b5) << 16)
				| (static_cast<uint64>(b6) <<  8)
				| (static_cast<uint64>(b7) <<  0)
				);
		}
	}
	MPT_FORCEINLINE operator float64 () const
	{
		return value;
	}
	MPT_FORCEINLINE IEEE754binary64Native & SetInt64(uint64 i)
	{
		value = DecodeIEEE754binary64(i);
		return *this;
	}
	MPT_FORCEINLINE uint64 GetInt64() const
	{
		return EncodeIEEE754binary64(value);
	}
	MPT_FORCEINLINE bool operator == (const IEEE754binary64Native &cmp) const
	{
		return value == cmp.value;
	}
	MPT_FORCEINLINE bool operator != (const IEEE754binary64Native &cmp) const
	{
		return value != cmp.value;
	}
};

MPT_STATIC_ASSERT((sizeof(IEEE754binary32Native<>) == 4));
MPT_STATIC_ASSERT((sizeof(IEEE754binary64Native<>) == 8));

namespace mpt {
template <> struct is_binary_safe< IEEE754binary32Native<> > : public std::true_type { };
template <> struct is_binary_safe< IEEE754binary64Native<> > : public std::true_type { };
}

template <bool is_iec559, mpt::endian endian = mpt::endian::native> struct IEEE754binary_types {
	typedef IEEE754binary32EmulatedLE IEEE754binary32LE;
	typedef IEEE754binary32EmulatedBE IEEE754binary32BE;
	typedef IEEE754binary64EmulatedLE IEEE754binary64LE;
	typedef IEEE754binary64EmulatedBE IEEE754binary64BE;
};
template <> struct IEEE754binary_types<true, mpt::endian::little> {
	typedef IEEE754binary32Native<>   IEEE754binary32LE;
	typedef IEEE754binary32EmulatedBE IEEE754binary32BE;
	typedef IEEE754binary64Native<>   IEEE754binary64LE;
	typedef IEEE754binary64EmulatedBE IEEE754binary64BE;
};
template <> struct IEEE754binary_types<true, mpt::endian::big> {
	typedef IEEE754binary32EmulatedLE IEEE754binary32LE;
	typedef IEEE754binary32Native<>   IEEE754binary32BE;
	typedef IEEE754binary64EmulatedLE IEEE754binary64LE;
	typedef IEEE754binary64Native<>   IEEE754binary64BE;
};

typedef IEEE754binary_types<std::numeric_limits<float32>::is_iec559 && std::numeric_limits<float64>::is_iec559, mpt::endian::native>::IEEE754binary32LE IEEE754binary32LE;
typedef IEEE754binary_types<std::numeric_limits<float32>::is_iec559 && std::numeric_limits<float64>::is_iec559, mpt::endian::native>::IEEE754binary32BE IEEE754binary32BE;
typedef IEEE754binary_types<std::numeric_limits<float32>::is_iec559 && std::numeric_limits<float64>::is_iec559, mpt::endian::native>::IEEE754binary64LE IEEE754binary64LE;
typedef IEEE754binary_types<std::numeric_limits<float32>::is_iec559 && std::numeric_limits<float64>::is_iec559, mpt::endian::native>::IEEE754binary64BE IEEE754binary64BE;

STATIC_ASSERT(sizeof(IEEE754binary32LE) == 4);
STATIC_ASSERT(sizeof(IEEE754binary32BE) == 4);
STATIC_ASSERT(sizeof(IEEE754binary64LE) == 8);
STATIC_ASSERT(sizeof(IEEE754binary64BE) == 8);


// unaligned

typedef IEEE754binary32EmulatedLE float32le;
typedef IEEE754binary32EmulatedBE float32be;
typedef IEEE754binary64EmulatedLE float64le;
typedef IEEE754binary64EmulatedBE float64be;

STATIC_ASSERT(sizeof(float32le) == 4);
STATIC_ASSERT(sizeof(float32be) == 4);
STATIC_ASSERT(sizeof(float64le) == 8);
STATIC_ASSERT(sizeof(float64be) == 8);


// potentially aligned

typedef IEEE754binary32LE float32le_fast;
typedef IEEE754binary32BE float32be_fast;
typedef IEEE754binary64LE float64le_fast;
typedef IEEE754binary64BE float64be_fast;

STATIC_ASSERT(sizeof(float32le_fast) == 4);
STATIC_ASSERT(sizeof(float32be_fast) == 4);
STATIC_ASSERT(sizeof(float64le_fast) == 8);
STATIC_ASSERT(sizeof(float64be_fast) == 8);



// On-disk integer types with defined endianness and no alignemnt requirements
// Note: To easily debug module loaders (and anything else that uses this
// wrapper struct), you can use the Debugger Visualizers available in
// build/vs/debug/ to conveniently view the wrapped contents.

template<typename T, typename Tendian>
struct packed
{
public:
	typedef T base_type;
	typedef Tendian endian_type;
public:
#if MPT_ENDIAN_IS_CONSTEXPR
	mpt::byte data[sizeof(base_type)]{};
#else // !MPT_ENDIAN_IS_CONSTEXPR
	std::array<mpt::byte, sizeof(base_type)> data;
#endif // MPT_ENDIAN_IS_CONSTEXPR
public:
	MPT_ENDIAN_CONSTEXPR_FUN void set(base_type val) noexcept
	{
		STATIC_ASSERT(std::numeric_limits<T>::is_integer);
		#if MPT_ENDIAN_IS_CONSTEXPR
			MPT_CONSTANT_IF(endian_type::endian == mpt::endian::big)
			{
				typename std::make_unsigned<base_type>::type uval = val;
				for(std::size_t i = 0; i < sizeof(base_type); ++i)
				{
					data[i] = static_cast<mpt::byte>((uval >> (8*(sizeof(base_type)-1-i))) & 0xffu);
				}
			} else
			{
				typename std::make_unsigned<base_type>::type uval = val;
				for(std::size_t i = 0; i < sizeof(base_type); ++i)
				{
					data[i] = static_cast<mpt::byte>((uval >> (8*i)) & 0xffu);
				}
			}
		#else // !MPT_ENDIAN_IS_CONSTEXPR
			MPT_CONSTANT_IF(mpt::endian::native == mpt::endian::little || mpt::endian::native == mpt::endian::big)
			{
				MPT_CONSTANT_IF(mpt::endian::native != endian_type::endian)
				{
					val = mpt::detail::SwapBytes(val);
				}
				std::memcpy(data.data(), &val, sizeof(val));
			} else
			{
				typedef typename std::make_unsigned<base_type>::type unsigned_base_type;
				data = EndianEncode<unsigned_base_type, Tendian, sizeof(T)>(val);
			}
		#endif // MPT_ENDIAN_IS_CONSTEXPR
	}
	MPT_ENDIAN_CONSTEXPR_FUN base_type get() const noexcept
	{
		STATIC_ASSERT(std::numeric_limits<T>::is_integer);
		#if MPT_ENDIAN_IS_CONSTEXPR
			MPT_CONSTANT_IF(endian_type::endian == mpt::endian::big)
			{
				typename std::make_unsigned<base_type>::type uval = 0;
				for(std::size_t i = 0; i < sizeof(base_type); ++i)
				{
					uval |= static_cast<typename std::make_unsigned<base_type>::type>(data[i]) << (8*(sizeof(base_type)-1-i));
				}
				return static_cast<base_type>(uval);
			} else
			{
				typename std::make_unsigned<base_type>::type uval = 0;
				for(std::size_t i = 0; i < sizeof(base_type); ++i)
				{
					uval |= static_cast<typename std::make_unsigned<base_type>::type>(data[i]) << (8*i);
				}
				return static_cast<base_type>(uval);
			}
		#else // !MPT_ENDIAN_IS_CONSTEXPR
			MPT_CONSTANT_IF(mpt::endian::native == mpt::endian::little || mpt::endian::native == mpt::endian::big)
			{
				base_type val = base_type();
				std::memcpy(&val, data.data(), sizeof(val));
				MPT_CONSTANT_IF(mpt::endian::native != endian_type::endian)
				{
					val = mpt::detail::SwapBytes(val);
				}
				return val;
			} else
			{
				typedef typename std::make_unsigned<base_type>::type unsigned_base_type;
				return EndianDecode<unsigned_base_type, Tendian, sizeof(T)>(data);
			}
		#endif // MPT_ENDIAN_IS_CONSTEXPR
	}
	MPT_ENDIAN_CONSTEXPR_FUN packed & operator = (const base_type & val) noexcept { set(val); return *this; }
	MPT_ENDIAN_CONSTEXPR_FUN operator base_type () const noexcept { return get(); }
public:
	MPT_ENDIAN_CONSTEXPR_FUN packed & operator &= (base_type val) noexcept { set(get() & val); return *this; }
	MPT_ENDIAN_CONSTEXPR_FUN packed & operator |= (base_type val) noexcept { set(get() | val); return *this; }
	MPT_ENDIAN_CONSTEXPR_FUN packed & operator ^= (base_type val) noexcept { set(get() ^ val); return *this; }
	MPT_ENDIAN_CONSTEXPR_FUN packed & operator += (base_type val) noexcept { set(get() + val); return *this; }
	MPT_ENDIAN_CONSTEXPR_FUN packed & operator -= (base_type val) noexcept { set(get() - val); return *this; }
	MPT_ENDIAN_CONSTEXPR_FUN packed & operator *= (base_type val) noexcept { set(get() * val); return *this; }
	MPT_ENDIAN_CONSTEXPR_FUN packed & operator /= (base_type val) noexcept { set(get() / val); return *this; }
	MPT_ENDIAN_CONSTEXPR_FUN packed & operator %= (base_type val) noexcept { set(get() % val); return *this; }
	MPT_ENDIAN_CONSTEXPR_FUN packed & operator ++ () noexcept { set(get() + 1); return *this; } // prefix
	MPT_ENDIAN_CONSTEXPR_FUN packed & operator -- () noexcept { set(get() - 1); return *this; } // prefix
	MPT_ENDIAN_CONSTEXPR_FUN base_type operator ++ (int) noexcept { base_type old = get(); set(old + 1); return old; } // postfix
	MPT_ENDIAN_CONSTEXPR_FUN base_type operator -- (int) noexcept { base_type old = get(); set(old - 1); return old; } // postfix
};

typedef packed< int64, LittleEndian_tag> int64le;
typedef packed< int32, LittleEndian_tag> int32le;
typedef packed< int16, LittleEndian_tag> int16le;
typedef packed< int8 , LittleEndian_tag> int8le;
typedef packed<uint64, LittleEndian_tag> uint64le;
typedef packed<uint32, LittleEndian_tag> uint32le;
typedef packed<uint16, LittleEndian_tag> uint16le;
typedef packed<uint8 , LittleEndian_tag> uint8le;

typedef packed< int64, BigEndian_tag> int64be;
typedef packed< int32, BigEndian_tag> int32be;
typedef packed< int16, BigEndian_tag> int16be;
typedef packed< int8 , BigEndian_tag> int8be;
typedef packed<uint64, BigEndian_tag> uint64be;
typedef packed<uint32, BigEndian_tag> uint32be;
typedef packed<uint16, BigEndian_tag> uint16be;
typedef packed<uint8 , BigEndian_tag> uint8be;

namespace mpt {
template <typename T, typename Tendian> struct limits<packed<T, Tendian>> : mpt::limits<T> {};
} // namespace mpt

MPT_BINARY_STRUCT(int64le, 8)
MPT_BINARY_STRUCT(int32le, 4)
MPT_BINARY_STRUCT(int16le, 2)
MPT_BINARY_STRUCT(int8le , 1)
MPT_BINARY_STRUCT(uint64le, 8)
MPT_BINARY_STRUCT(uint32le, 4)
MPT_BINARY_STRUCT(uint16le, 2)
MPT_BINARY_STRUCT(uint8le , 1)

MPT_BINARY_STRUCT(int64be, 8)
MPT_BINARY_STRUCT(int32be, 4)
MPT_BINARY_STRUCT(int16be, 2)
MPT_BINARY_STRUCT(int8be , 1)
MPT_BINARY_STRUCT(uint64be, 8)
MPT_BINARY_STRUCT(uint32be, 4)
MPT_BINARY_STRUCT(uint16be, 2)
MPT_BINARY_STRUCT(uint8be , 1)

namespace mpt {

template <typename T> struct make_le { typedef packed<typename std::remove_const<T>::type, LittleEndian_tag> type; };
template <typename T> struct make_be { typedef packed<typename std::remove_const<T>::type, BigEndian_tag> type; };

template <typename T>
MPT_ENDIAN_CONSTEXPR_FUN auto as_le(T v) noexcept -> typename mpt::make_le<typename std::remove_const<T>::type>::type
{
	typename mpt::make_le<typename std::remove_const<T>::type>::type res;
	res = v;
	return res;
}
template <typename T>
MPT_ENDIAN_CONSTEXPR_FUN auto as_be(T v) noexcept -> typename mpt::make_be<typename std::remove_const<T>::type>::type
{
	typename mpt::make_be<typename std::remove_const<T>::type>::type res;
	res = v;
	return res;
}

template <typename Tpacked>
MPT_ENDIAN_CONSTEXPR_FUN Tpacked as_endian(typename Tpacked::base_type v) noexcept
{
	Tpacked res;
	res = v;
	return res;
}

} // namespace mpt



// 24-bit integer wrapper (for 24-bit PCM)
struct int24
{
	uint8 bytes[3];
	int24() noexcept
	{
		bytes[0] = bytes[1] = bytes[2] = 0;
	}
	explicit int24(int other) noexcept
	{
		MPT_MAYBE_CONSTANT_IF(mpt::endian_is_big())
		{
			bytes[0] = (static_cast<unsigned int>(other)>>16)&0xff;
			bytes[1] = (static_cast<unsigned int>(other)>> 8)&0xff;
			bytes[2] = (static_cast<unsigned int>(other)>> 0)&0xff;
		} else
		{
			bytes[0] = (static_cast<unsigned int>(other)>> 0)&0xff;
			bytes[1] = (static_cast<unsigned int>(other)>> 8)&0xff;
			bytes[2] = (static_cast<unsigned int>(other)>>16)&0xff;
		}
	}
	operator int() const noexcept
	{
		MPT_MAYBE_CONSTANT_IF(mpt::endian_is_big())
		{
			return (static_cast<int8>(bytes[0]) * 65536) + (bytes[1] * 256) + bytes[2];
		} else
		{
			return (static_cast<int8>(bytes[2]) * 65536) + (bytes[1] * 256) + bytes[0];
		}
	}
};
MPT_STATIC_ASSERT(sizeof(int24) == 3);
#define int24_min (0-0x00800000)
#define int24_max (0+0x007fffff)



// Small helper class to support unaligned memory access on all platforms.
// This is only used to make old module loaders work everywhere.
// Do not use in new code.
template <typename T>
class const_unaligned_ptr_le
{
public:
	typedef T value_type;
private:
	const mpt::byte *mem;
	value_type Read() const
	{
		typename mpt::make_le<T>::type val;
		std::memcpy(&val, mem, sizeof(value_type));
		return val;
	}
public:
	const_unaligned_ptr_le() : mem(nullptr) {}
	const_unaligned_ptr_le(const const_unaligned_ptr_le<value_type> & other) : mem(other.mem) {}
	const_unaligned_ptr_le & operator = (const const_unaligned_ptr_le<value_type> & other) { mem = other.mem; return *this; }
	template <typename Tbyte> explicit const_unaligned_ptr_le(const Tbyte *mem) : mem(mpt::byte_cast<const mpt::byte*>(mem)) {}
	const_unaligned_ptr_le & operator += (std::size_t count) { mem += count * sizeof(value_type); return *this; }
	const_unaligned_ptr_le & operator -= (std::size_t count) { mem -= count * sizeof(value_type); return *this; }
	const_unaligned_ptr_le & operator ++ () { mem += sizeof(value_type); return *this; }
	const_unaligned_ptr_le & operator -- () { mem -= sizeof(value_type); return *this; }
	const_unaligned_ptr_le operator ++ (int) { const_unaligned_ptr_le<value_type> result = *this; ++result; return result; }
	const_unaligned_ptr_le operator -- (int) { const_unaligned_ptr_le<value_type> result = *this; --result; return result; }
	const_unaligned_ptr_le operator + (std::size_t count) const { const_unaligned_ptr_le<value_type> result = *this; result += count; return result; }
	const_unaligned_ptr_le operator - (std::size_t count) const { const_unaligned_ptr_le<value_type> result = *this; result -= count; return result; }
	const value_type operator * () const { return Read(); }
	const value_type operator [] (std::size_t i) const { return *((*this) + i); }
	operator bool () const { return mem != nullptr; }
};

template <typename T>
class const_unaligned_ptr_be
{
public:
	typedef T value_type;
private:
	const mpt::byte *mem;
	value_type Read() const
	{
		typename mpt::make_be<T>::type val;
		std::memcpy(&val, mem, sizeof(value_type));
		return val;
	}
public:
	const_unaligned_ptr_be() : mem(nullptr) {}
	const_unaligned_ptr_be(const const_unaligned_ptr_be<value_type> & other) : mem(other.mem) {}
	const_unaligned_ptr_be & operator = (const const_unaligned_ptr_be<value_type> & other) { mem = other.mem; return *this; }
	template <typename Tbyte> explicit const_unaligned_ptr_be(const Tbyte *mem) : mem(mpt::byte_cast<const mpt::byte*>(mem)) {}
	const_unaligned_ptr_be & operator += (std::size_t count) { mem += count * sizeof(value_type); return *this; }
	const_unaligned_ptr_be & operator -= (std::size_t count) { mem -= count * sizeof(value_type); return *this; }
	const_unaligned_ptr_be & operator ++ () { mem += sizeof(value_type); return *this; }
	const_unaligned_ptr_be & operator -- () { mem -= sizeof(value_type); return *this; }
	const_unaligned_ptr_be operator ++ (int) { const_unaligned_ptr_be<value_type> result = *this; ++result; return result; }
	const_unaligned_ptr_be operator -- (int) { const_unaligned_ptr_be<value_type> result = *this; --result; return result; }
	const_unaligned_ptr_be operator + (std::size_t count) const { const_unaligned_ptr_be<value_type> result = *this; result += count; return result; }
	const_unaligned_ptr_be operator - (std::size_t count) const { const_unaligned_ptr_be<value_type> result = *this; result -= count; return result; }
	const value_type operator * () const { return Read(); }
	const value_type operator [] (std::size_t i) const { return *((*this) + i); }
	operator bool () const { return mem != nullptr; }
};

template <typename T>
class const_unaligned_ptr
{
public:
	typedef T value_type;
private:
	const mpt::byte *mem;
	value_type Read() const
	{
		value_type val = value_type();
		std::memcpy(&val, mem, sizeof(value_type));
		return val;
	}
public:
	const_unaligned_ptr() : mem(nullptr) {}
	const_unaligned_ptr(const const_unaligned_ptr<value_type> & other) : mem(other.mem) {}
	const_unaligned_ptr & operator = (const const_unaligned_ptr<value_type> & other) { mem = other.mem; return *this; }
	template <typename Tbyte> explicit const_unaligned_ptr(const Tbyte *mem) : mem(mpt::byte_cast<const mpt::byte*>(mem)) {}
	const_unaligned_ptr & operator += (std::size_t count) { mem += count * sizeof(value_type); return *this; }
	const_unaligned_ptr & operator -= (std::size_t count) { mem -= count * sizeof(value_type); return *this; }
	const_unaligned_ptr & operator ++ () { mem += sizeof(value_type); return *this; }
	const_unaligned_ptr & operator -- () { mem -= sizeof(value_type); return *this; }
	const_unaligned_ptr operator ++ (int) { const_unaligned_ptr<value_type> result = *this; ++result; return result; }
	const_unaligned_ptr operator -- (int) { const_unaligned_ptr<value_type> result = *this; --result; return result; }
	const_unaligned_ptr operator + (std::size_t count) const { const_unaligned_ptr<value_type> result = *this; result += count; return result; }
	const_unaligned_ptr operator - (std::size_t count) const { const_unaligned_ptr<value_type> result = *this; result -= count; return result; }
	const value_type operator * () const { return Read(); }
	const value_type operator [] (std::size_t i) const { return *((*this) + i); }
	operator bool () const { return mem != nullptr; }
};


OPENMPT_NAMESPACE_END

