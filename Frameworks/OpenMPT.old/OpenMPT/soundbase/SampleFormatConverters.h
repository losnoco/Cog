/*
 * SampleFormatConverters.h
 * ------------------------
 * Purpose: Functions and functors for reading and converting pretty much any uncompressed sample format supported by OpenMPT.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"


#include "../common/Endianness.h"


OPENMPT_NAMESPACE_BEGIN


// Byte offsets, from lowest significant to highest significant byte (for various functor template parameters)
#define littleEndian64 0, 1, 2, 3, 4, 5, 6, 7
#define littleEndian32 0, 1, 2, 3
#define littleEndian24 0, 1, 2
#define littleEndian16 0, 1

#define bigEndian64 7, 6, 5, 4, 3, 2, 1, 0
#define bigEndian32 3, 2, 1, 0
#define bigEndian24 2, 1, 0
#define bigEndian16 1, 0


namespace SC { // SC = _S_ample_C_onversion



#if MPT_COMPILER_MSVC
#define MPT_SC_AVOID_ROUND 1
#else
#define MPT_SC_AVOID_ROUND 0
#endif

#if MPT_SC_AVOID_ROUND
#define MPT_SC_FASTROUND(x) std::floor((x) + 0.5f)
#else
#define MPT_SC_FASTROUND(x) mpt::round(x)
#endif

#if MPT_COMPILER_SHIFT_SIGNED

#define MPT_SC_RSHIFT_SIGNED(val, shift) ((val) >> (shift))
#define MPT_SC_LSHIFT_SIGNED(val, shift) ((val) << (shift))

#else

#define MPT_SC_RSHIFT_SIGNED(val, shift) mpt::rshift_signed((val), (shift))
#define MPT_SC_LSHIFT_SIGNED(val, shift) mpt::lshift_signed((val), (shift))

#endif



// Every sample decoding functor has to typedef its input_t and output_t
// and has to provide a static constexpr input_inc member
// which describes by how many input_t elements inBuf has to be incremented between invocations.
// input_inc is normally 1 except when decoding e.g. bigger sample values
// from multiple std::byte values.


// decodes signed 7bit values stored as signed int8
struct DecodeInt7
{
	typedef std::byte input_t;
	typedef int8 output_t;
	static constexpr std::size_t input_inc = 1;
	MPT_FORCEINLINE output_t operator() (const input_t *inBuf)
	{
		return Clamp(mpt::byte_cast<int8>(*inBuf), static_cast<int8>(-64), static_cast<int8>(63)) * 2;
	}
};

struct DecodeInt8
{
	typedef std::byte input_t;
	typedef int8 output_t;
	static constexpr std::size_t input_inc = 1;
	MPT_FORCEINLINE output_t operator() (const input_t *inBuf)
	{
		return mpt::byte_cast<int8>(*inBuf);
	}
};

struct DecodeUint8
{
	typedef std::byte input_t;
	typedef int8 output_t;
	static constexpr std::size_t input_inc = 1;
	MPT_FORCEINLINE output_t operator() (const input_t *inBuf)
	{
		return static_cast<int8>(int(mpt::byte_cast<uint8>(*inBuf)) - 128);
	}
};

struct DecodeInt8Delta
{
	typedef std::byte input_t;
	typedef int8 output_t;
	static constexpr std::size_t input_inc = 1;
	uint8 delta;
	DecodeInt8Delta() : delta(0) { }
	MPT_FORCEINLINE output_t operator() (const input_t *inBuf)
	{
		delta += mpt::byte_cast<uint8>(*inBuf);
		return static_cast<int8>(delta);
	}
};

template <uint16 offset, size_t loByteIndex, size_t hiByteIndex>
struct DecodeInt16
{
	typedef std::byte input_t;
	typedef int16 output_t;
	static constexpr std::size_t input_inc = 2;
	MPT_FORCEINLINE output_t operator() (const input_t *inBuf)
	{
		return (mpt::byte_cast<uint8>(inBuf[loByteIndex]) | (mpt::byte_cast<uint8>(inBuf[hiByteIndex]) << 8)) - offset;
	}
};

template <size_t loByteIndex, size_t hiByteIndex>
struct DecodeInt16Delta
{
	typedef std::byte input_t;
	typedef int16 output_t;
	static constexpr std::size_t input_inc = 2;
	uint16 delta;
	DecodeInt16Delta() : delta(0) { }
	MPT_FORCEINLINE output_t operator() (const input_t *inBuf)
	{
		delta += mpt::byte_cast<uint8>(inBuf[loByteIndex]) | (mpt::byte_cast<uint8>(inBuf[hiByteIndex]) << 8);
		return static_cast<int16>(delta);
	}
};

struct DecodeInt16Delta8
{
	typedef std::byte input_t;
	typedef int16 output_t;
	static constexpr std::size_t input_inc = 2;
	uint16 delta;
	DecodeInt16Delta8() : delta(0) { }
	MPT_FORCEINLINE output_t operator() (const input_t *inBuf)
	{
		delta += mpt::byte_cast<uint8>(inBuf[0]);
		int16 result = delta & 0xFF;
		delta += mpt::byte_cast<uint8>(inBuf[1]);
		result |= (delta << 8);
		return result;
	}
};

template <uint32 offset, size_t loByteIndex, size_t midByteIndex, size_t hiByteIndex>
struct DecodeInt24
{
	typedef std::byte input_t;
	typedef int32 output_t;
	static constexpr std::size_t input_inc = 3;
	MPT_FORCEINLINE output_t operator() (const input_t *inBuf)
	{
		return ((mpt::byte_cast<uint8>(inBuf[loByteIndex]) << 8) | (mpt::byte_cast<uint8>(inBuf[midByteIndex]) << 16) | (mpt::byte_cast<uint8>(inBuf[hiByteIndex]) << 24)) - offset;
	}
};

template <uint32 offset, size_t loLoByteIndex, size_t loHiByteIndex, size_t hiLoByteIndex, size_t hiHiByteIndex>
struct DecodeInt32
{
	typedef std::byte input_t;
	typedef int32 output_t;
	static constexpr std::size_t input_inc = 4;
	MPT_FORCEINLINE output_t operator() (const input_t *inBuf)
	{
		return (mpt::byte_cast<uint8>(inBuf[loLoByteIndex]) | (mpt::byte_cast<uint8>(inBuf[loHiByteIndex]) << 8) | (mpt::byte_cast<uint8>(inBuf[hiLoByteIndex]) << 16) | (mpt::byte_cast<uint8>(inBuf[hiHiByteIndex]) << 24)) - offset;
	}
};

template <uint64 offset, size_t b0, size_t b1, size_t b2, size_t b3, size_t b4, size_t b5, size_t b6, size_t b7>
struct DecodeInt64
{
	typedef std::byte input_t;
	typedef int64 output_t;
	static constexpr std::size_t input_inc = 8;
	MPT_FORCEINLINE output_t operator() (const input_t *inBuf)
	{
		return (uint64(0)
			| (static_cast<uint64>(mpt::byte_cast<uint8>(inBuf[b0])) <<  0)
			| (static_cast<uint64>(mpt::byte_cast<uint8>(inBuf[b1])) <<  8)
			| (static_cast<uint64>(mpt::byte_cast<uint8>(inBuf[b2])) << 16)
			| (static_cast<uint64>(mpt::byte_cast<uint8>(inBuf[b3])) << 24)
			| (static_cast<uint64>(mpt::byte_cast<uint8>(inBuf[b4])) << 32)
			| (static_cast<uint64>(mpt::byte_cast<uint8>(inBuf[b5])) << 40)
			| (static_cast<uint64>(mpt::byte_cast<uint8>(inBuf[b6])) << 48)
			| (static_cast<uint64>(mpt::byte_cast<uint8>(inBuf[b7])) << 56)
			) - offset;
	}
};

template <size_t loLoByteIndex, size_t loHiByteIndex, size_t hiLoByteIndex, size_t hiHiByteIndex>
struct DecodeFloat32
{
	typedef std::byte input_t;
	typedef float32 output_t;
	static constexpr std::size_t input_inc = 4;
	MPT_FORCEINLINE output_t operator() (const input_t *inBuf)
	{
		return IEEE754binary32LE(inBuf[loLoByteIndex], inBuf[loHiByteIndex], inBuf[hiLoByteIndex], inBuf[hiHiByteIndex]);
	}
};

template <size_t loLoByteIndex, size_t loHiByteIndex, size_t hiLoByteIndex, size_t hiHiByteIndex>
struct DecodeScaledFloat32
{
	typedef std::byte input_t;
	typedef float32 output_t;
	static constexpr std::size_t input_inc = 4;
	float factor;
	MPT_FORCEINLINE output_t operator() (const input_t *inBuf)
	{
		return factor * IEEE754binary32LE(inBuf[loLoByteIndex], inBuf[loHiByteIndex], inBuf[hiLoByteIndex], inBuf[hiHiByteIndex]);
	}
	MPT_FORCEINLINE DecodeScaledFloat32(float scaleFactor)
		: factor(scaleFactor)
	{
		return;
	}
};

template <size_t b0, size_t b1, size_t b2, size_t b3, size_t b4, size_t b5, size_t b6, size_t b7>
struct DecodeFloat64
{
	typedef std::byte input_t;
	typedef float64 output_t;
	static constexpr std::size_t input_inc = 8;
	MPT_FORCEINLINE output_t operator() (const input_t *inBuf)
	{
		return IEEE754binary64LE(inBuf[b0], inBuf[b1], inBuf[b2], inBuf[b3], inBuf[b4], inBuf[b5], inBuf[b6], inBuf[b7]);
	}
};

template <typename Tsample>
struct DecodeIdentity
{
	typedef Tsample input_t;
	typedef Tsample output_t;
	static constexpr std::size_t input_inc = 1;
	MPT_FORCEINLINE output_t operator() (const input_t *inBuf)
	{
		return *inBuf;
	}
};



// Shift input_t down by shift and saturate to output_t.
template <typename Tdst, typename Tsrc, int shift>
struct ConvertShift
{
	typedef Tsrc input_t;
	typedef Tdst output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return mpt::saturate_cast<output_t>(MPT_SC_RSHIFT_SIGNED(val, shift));
	}
};



// Shift input_t up by shift and saturate to output_t.
template <typename Tdst, typename Tsrc, int shift>
struct ConvertShiftUp
{
	typedef Tsrc input_t;
	typedef Tdst output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return mpt::saturate_cast<output_t>(MPT_SC_LSHIFT_SIGNED(val, shift));
	}
};




// Every sample conversion functor has to typedef its input_t and output_t.
// The input_t argument is taken by value because we only deal with per-single-sample conversions here.


// straight forward type conversions, clamping when converting from floating point.
template <typename Tdst, typename Tsrc>
struct Convert;

template <typename Tid>
struct Convert<Tid, Tid>
{
	typedef Tid input_t;
	typedef Tid output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return val;
	}
};

template <>
struct Convert<uint8, int8>
{
	typedef int8 input_t;
	typedef uint8 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<uint8>(val+0x80);
	}
};

template <>
struct Convert<uint8, int16>
{
	typedef int16 input_t;
	typedef uint8 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<uint8>(static_cast<int8>(MPT_SC_RSHIFT_SIGNED(val, 8))+0x80);
	}
};

template <>
struct Convert<uint8, int24>
{
	typedef int24 input_t;
	typedef uint8 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<uint8>(static_cast<int8>(MPT_SC_RSHIFT_SIGNED(static_cast<int>(val), 16))+0x80);
	}
};

template <>
struct Convert<uint8, int32>
{
	typedef int32 input_t;
	typedef uint8 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<uint8>(static_cast<int8>(MPT_SC_RSHIFT_SIGNED(val, 24))+0x80);
	}
};

template <>
struct Convert<uint8, int64>
{
	typedef int64 input_t;
	typedef uint8 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<uint8>(static_cast<int8>(MPT_SC_RSHIFT_SIGNED(val, 56))+0x80);
	}
};

template <>
struct Convert<uint8, float32>
{
	typedef float32 input_t;
	typedef uint8 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		Limit(val, -1.0f, 1.0f);
		val *= 128.0f;
		return static_cast<uint8>(mpt::saturate_cast<int8>(static_cast<int>(MPT_SC_FASTROUND(val)))+0x80);
	}
};

template <>
struct Convert<uint8, double>
{
	typedef double input_t;
	typedef uint8 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		Limit(val, -1.0, 1.0);
		val *= 128.0;
		return static_cast<uint8>(mpt::saturate_cast<int8>(static_cast<int>(MPT_SC_FASTROUND(val)))+0x80);
	}
};

template <>
struct Convert<int8, uint8>
{
	typedef uint8 input_t;
	typedef int8 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<int8>(static_cast<int>(val)-0x80);
	}
};

template <>
struct Convert<int8, int16>
{
	typedef int16 input_t;
	typedef int8 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<int8>(MPT_SC_RSHIFT_SIGNED(val, 8));
	}
};

template <>
struct Convert<int8, int24>
{
	typedef int24 input_t;
	typedef int8 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<int8>(MPT_SC_RSHIFT_SIGNED(static_cast<int>(val), 16));
	}
};

template <>
struct Convert<int8, int32>
{
	typedef int32 input_t;
	typedef int8 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<int8>(MPT_SC_RSHIFT_SIGNED(val, 24));
	}
};

template <>
struct Convert<int8, int64>
{
	typedef int64 input_t;
	typedef int8 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<int8>(MPT_SC_RSHIFT_SIGNED(val, 56));
	}
};

template <>
struct Convert<int8, float32>
{
	typedef float32 input_t;
	typedef int8 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		Limit(val, -1.0f, 1.0f);
		val *= 128.0f;
		return mpt::saturate_cast<int8>(static_cast<int>(MPT_SC_FASTROUND(val)));
	}
};

template <>
struct Convert<int8, double>
{
	typedef double input_t;
	typedef int8 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		Limit(val, -1.0, 1.0);
		val *= 128.0;
		return mpt::saturate_cast<int8>(static_cast<int>(MPT_SC_FASTROUND(val)));
	}
};

template <>
struct Convert<int16, uint8>
{
	typedef uint8 input_t;
	typedef int16 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<int16>(MPT_SC_LSHIFT_SIGNED(static_cast<int>(val)-0x80, 8));
	}
};

template <>
struct Convert<int16, int8>
{
	typedef int8 input_t;
	typedef int16 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<int16>(MPT_SC_LSHIFT_SIGNED(val, 8));
	}
};

template <>
struct Convert<int16, int24>
{
	typedef int24 input_t;
	typedef int16 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<int16>(MPT_SC_RSHIFT_SIGNED(static_cast<int>(val), 8));
	}
};

template <>
struct Convert<int16, int32>
{
	typedef int32 input_t;
	typedef int16 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<int16>(MPT_SC_RSHIFT_SIGNED(val, 16));
	}
};

template <>
struct Convert<int16, int64>
{
	typedef int64 input_t;
	typedef int16 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<int16>(MPT_SC_RSHIFT_SIGNED(val, 48));
	}
};

template <>
struct Convert<int16, float32>
{
	typedef float32 input_t;
	typedef int16 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		Limit(val, -1.0f, 1.0f);
		val *= 32768.0f;
		return mpt::saturate_cast<int16>(static_cast<int>(MPT_SC_FASTROUND(val)));
	}
};

template <>
struct Convert<int16, double>
{
	typedef double input_t;
	typedef int16 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		Limit(val, -1.0, 1.0);
		val *= 32768.0;
		return mpt::saturate_cast<int16>(static_cast<int>(MPT_SC_FASTROUND(val)));
	}
};

template <>
struct Convert<int24, uint8>
{
	typedef uint8 input_t;
	typedef int24 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<int24>(MPT_SC_LSHIFT_SIGNED(static_cast<int>(val)-0x80, 16));
	}
};

template <>
struct Convert<int24, int8>
{
	typedef int8 input_t;
	typedef int24 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<int24>(MPT_SC_LSHIFT_SIGNED(val, 16));
	}
};

template <>
struct Convert<int24, int16>
{
	typedef int16 input_t;
	typedef int24 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<int24>(MPT_SC_LSHIFT_SIGNED(val, 8));
	}
};

template <>
struct Convert<int24, int32>
{
	typedef int32 input_t;
	typedef int24 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<int24>(MPT_SC_RSHIFT_SIGNED(val, 8));
	}
};

template <>
struct Convert<int24, int64>
{
	typedef int64 input_t;
	typedef int24 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<int24>(MPT_SC_RSHIFT_SIGNED(val, 40));
	}
};

template <>
struct Convert<int24, float32>
{
	typedef float32 input_t;
	typedef int24 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		Limit(val, -1.0f, 1.0f);
		val *= 2147483648.0f;
		return static_cast<int24>(MPT_SC_RSHIFT_SIGNED(mpt::saturate_cast<int32>(static_cast<int64>(MPT_SC_FASTROUND(val))), 8));
	}
};

template <>
struct Convert<int24, double>
{
	typedef double input_t;
	typedef int24 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		Limit(val, -1.0, 1.0);
		val *= 2147483648.0;
		return static_cast<int24>(MPT_SC_RSHIFT_SIGNED(mpt::saturate_cast<int32>(static_cast<int64>(MPT_SC_FASTROUND(val))), 8));
	}
};

template <>
struct Convert<int32, uint8>
{
	typedef uint8 input_t;
	typedef int32 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<int32>(MPT_SC_LSHIFT_SIGNED(static_cast<int>(val)-0x80, 24));
	}
};

template <>
struct Convert<int32, int8>
{
	typedef int8 input_t;
	typedef int32 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<int32>(MPT_SC_LSHIFT_SIGNED(val, 24));
	}
};

template <>
struct Convert<int32, int16>
{
	typedef int16 input_t;
	typedef int32 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<int32>(MPT_SC_LSHIFT_SIGNED(val, 16));
	}
};

template <>
struct Convert<int32, int24>
{
	typedef int24 input_t;
	typedef int32 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<int32>(MPT_SC_LSHIFT_SIGNED(static_cast<int>(val), 8));
	}
};

template <>
struct Convert<int32, int64>
{
	typedef int64 input_t;
	typedef int32 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<int32>(MPT_SC_RSHIFT_SIGNED(val, 32));
	}
};

template <>
struct Convert<int32, float32>
{
	typedef float32 input_t;
	typedef int32 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		Limit(val, -1.0f, 1.0f);
		val *= 2147483648.0f;
		return mpt::saturate_cast<int32>(static_cast<int64>(MPT_SC_FASTROUND(val)));
	}
};

template <>
struct Convert<int32, double>
{
	typedef double input_t;
	typedef int32 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		Limit(val, -1.0, 1.0);
		val *= 2147483648.0;
		return mpt::saturate_cast<int32>(static_cast<int64>(MPT_SC_FASTROUND(val)));
	}
};

template <>
struct Convert<int64, uint8>
{
	typedef uint8 input_t;
	typedef int64 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return MPT_SC_LSHIFT_SIGNED(static_cast<int64>(val)-0x80, 56);
	}
};

template <>
struct Convert<int64, int8>
{
	typedef int8 input_t;
	typedef int64 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return MPT_SC_LSHIFT_SIGNED(static_cast<int64>(val), 56);
	}
};

template <>
struct Convert<int64, int16>
{
	typedef int16 input_t;
	typedef int64 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return MPT_SC_LSHIFT_SIGNED(static_cast<int64>(val), 48);
	}
};

template <>
struct Convert<int64, int24>
{
	typedef int24 input_t;
	typedef int64 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return MPT_SC_LSHIFT_SIGNED(static_cast<int64>(val), 40);
	}
};

template <>
struct Convert<int64, int32>
{
	typedef int32 input_t;
	typedef int64 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return MPT_SC_LSHIFT_SIGNED(static_cast<int64>(val), 32);
	}
};

template <>
struct Convert<int64, float32>
{
	typedef float32 input_t;
	typedef int64 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		Limit(val, -1.0f, 1.0f);
		val *= static_cast<float>(uint64(1)<<63);
		return mpt::saturate_cast<int64>(MPT_SC_FASTROUND(val));
	}
};

template <>
struct Convert<int64, double>
{
	typedef double input_t;
	typedef int64 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		Limit(val, -1.0, 1.0);
		val *= static_cast<double>(uint64(1)<<63);
		return mpt::saturate_cast<int64>(MPT_SC_FASTROUND(val));
	}
};

template <>
struct Convert<float32, uint8>
{
	typedef uint8 input_t;
	typedef float32 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return (static_cast<int>(val)-0x80) * (1.0f / static_cast<float32>(static_cast<unsigned int>(1)<<7));
	}
};

template <>
struct Convert<float32, int8>
{
	typedef int8 input_t;
	typedef float32 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return val * (1.0f / static_cast<float>(static_cast<unsigned int>(1)<<7));
	}
};

template <>
struct Convert<float32, int16>
{
	typedef int16 input_t;
	typedef float32 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return val * (1.0f / static_cast<float>(static_cast<unsigned int>(1)<<15));
	}
};

template <>
struct Convert<float32, int24>
{
	typedef int24 input_t;
	typedef float32 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return val * (1.0f / static_cast<float>(static_cast<unsigned int>(1)<<23));
	}
};

template <>
struct Convert<float32, int32>
{
	typedef int32 input_t;
	typedef float32 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return val * (1.0f / static_cast<float>(static_cast<unsigned int>(1)<<31));
	}
};

template <>
struct Convert<float32, int64>
{
	typedef int64 input_t;
	typedef float32 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return val * (1.0f / static_cast<float>(static_cast<uint64>(1)<<63));
	}
};

template <>
struct Convert<double, uint8>
{
	typedef uint8 input_t;
	typedef double output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return (static_cast<int>(val)-0x80) * (1.0 / static_cast<double>(static_cast<unsigned int>(1)<<7));
	}
};

template <>
struct Convert<double, int8>
{
	typedef int8 input_t;
	typedef double output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return val * (1.0 / static_cast<double>(static_cast<unsigned int>(1)<<7));
	}
};

template <>
struct Convert<double, int16>
{
	typedef int16 input_t;
	typedef double output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return val * (1.0 / static_cast<double>(static_cast<unsigned int>(1)<<15));
	}
};

template <>
struct Convert<double, int24>
{
	typedef int24 input_t;
	typedef double output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return val * (1.0 / static_cast<double>(static_cast<unsigned int>(1)<<23));
	}
};

template <>
struct Convert<double, int32>
{
	typedef int32 input_t;
	typedef double output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return val * (1.0 / static_cast<double>(static_cast<unsigned int>(1)<<31));
	}
};

template <>
struct Convert<double, int64>
{
	typedef int64 input_t;
	typedef double output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return val * (1.0 / static_cast<double>(static_cast<uint64>(1)<<63));
	}
};

template <>
struct Convert<double, float>
{
	typedef float input_t;
	typedef double output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<double>(val);
	}
};

template <>
struct Convert<float, double>
{
	typedef double input_t;
	typedef float output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		return static_cast<float>(val);
	}
};


template <typename Tdst, typename Tsrc, int fractionalBits>
struct ConvertFixedPoint;

template <int fractionalBits>
struct ConvertFixedPoint<uint8, int32, fractionalBits>
{
	typedef int32 input_t;
	typedef uint8 output_t;
	static constexpr int shiftBits = fractionalBits + 1 - sizeof(output_t) * 8;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(input_t)*8-1);
		static_assert(shiftBits >= 1);
		val = MPT_SC_RSHIFT_SIGNED((val + (1<<(shiftBits-1))), shiftBits); // round
		if(val < int8_min) val = int8_min;
		if(val > int8_max) val = int8_max;
		return static_cast<uint8>(val+0x80); // unsigned
	}
};

template <int fractionalBits>
struct ConvertFixedPoint<int8, int32, fractionalBits>
{
	typedef int32 input_t;
	typedef int8 output_t;
	static constexpr int shiftBits = fractionalBits + 1 - sizeof(output_t) * 8;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(input_t)*8-1);
		static_assert(shiftBits >= 1);
		val = MPT_SC_RSHIFT_SIGNED((val + (1<<(shiftBits-1))), shiftBits); // round
		if(val < int8_min) val = int8_min;
		if(val > int8_max) val = int8_max;
		return static_cast<int8>(val);
	}
};

template <int fractionalBits>
struct ConvertFixedPoint<int16, int32, fractionalBits>
{
	typedef int32 input_t;
	typedef int16 output_t;
	static constexpr int shiftBits = fractionalBits + 1 - sizeof(output_t) * 8;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(input_t)*8-1);
		static_assert(shiftBits >= 1);
		val = MPT_SC_RSHIFT_SIGNED((val + (1<<(shiftBits-1))), shiftBits); // round
		if(val < int16_min) val = int16_min;
		if(val > int16_max) val = int16_max;
		return static_cast<int16>(val);
	}
};

template <int fractionalBits>
struct ConvertFixedPoint<int24, int32, fractionalBits>
{
	typedef int32 input_t;
	typedef int24 output_t;
	static constexpr int shiftBits = fractionalBits + 1 - sizeof(output_t) * 8;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(input_t)*8-1);
		static_assert(shiftBits >= 1);
		val = MPT_SC_RSHIFT_SIGNED((val + (1<<(shiftBits-1))), shiftBits); // round
		if(val < int24_min) val = int24_min;
		if(val > int24_max) val = int24_max;
		return static_cast<int24>(val);
	}
};

template <int fractionalBits>
struct ConvertFixedPoint<int32, int32, fractionalBits>
{
	typedef int32 input_t;
	typedef int32 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(input_t)*8-1);
		return static_cast<int32>(Clamp(val, static_cast<int>(-((1<<fractionalBits)-1)), static_cast<int>(1<<fractionalBits)-1)) << (sizeof(input_t)*8-1-fractionalBits);
	}
};

template <int fractionalBits>
struct ConvertFixedPoint<float32, int32, fractionalBits>
{
	typedef int32 input_t;
	typedef float32 output_t;
	const float factor;
	MPT_FORCEINLINE ConvertFixedPoint()
		: factor( 1.0f / static_cast<float>(1 << fractionalBits) )
	{
		return;
	}
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(input_t)*8-1);
		return val * factor;
	}
};

template <int fractionalBits>
struct ConvertFixedPoint<float64, int32, fractionalBits>
{
	typedef int32 input_t;
	typedef float64 output_t;
	const double factor;
	MPT_FORCEINLINE ConvertFixedPoint()
		: factor( 1.0 / static_cast<double>(1 << fractionalBits) )
	{
		return;
	}
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(input_t)*8-1);
		return val * factor;
	}
};


template <typename Tdst, typename Tsrc, int fractionalBits>
struct ConvertToFixedPoint;

template <int fractionalBits>
struct ConvertToFixedPoint<int32, uint8, fractionalBits>
{
	typedef uint8 input_t;
	typedef int32 output_t;
	static constexpr int shiftBits = fractionalBits + 1 - sizeof(input_t) * 8;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(output_t)*8-1);
		static_assert(shiftBits >= 1);
		return MPT_SC_LSHIFT_SIGNED(static_cast<output_t>(static_cast<int>(val)-0x80), shiftBits);
	}
};

template <int fractionalBits>
struct ConvertToFixedPoint<int32, int8, fractionalBits>
{
	typedef int8 input_t;
	typedef int32 output_t;
	static constexpr int shiftBits = fractionalBits + 1 - sizeof(input_t) * 8;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(output_t)*8-1);
		static_assert(shiftBits >= 1);
		return MPT_SC_LSHIFT_SIGNED(static_cast<output_t>(val), shiftBits);
	}
};

template <int fractionalBits>
struct ConvertToFixedPoint<int32, int16, fractionalBits>
{
	typedef int16 input_t;
	typedef int32 output_t;
	static constexpr int shiftBits = fractionalBits + 1 - sizeof(input_t) * 8;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(output_t)*8-1);
		static_assert(shiftBits >= 1);
		return MPT_SC_LSHIFT_SIGNED(static_cast<output_t>(val), shiftBits);
	}
};

template <int fractionalBits>
struct ConvertToFixedPoint<int32, int24, fractionalBits>
{
	typedef int24 input_t;
	typedef int32 output_t;
	static constexpr int shiftBits = fractionalBits + 1 - sizeof(input_t) * 8;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(output_t)*8-1);
		static_assert(shiftBits >= 1);
		return MPT_SC_LSHIFT_SIGNED(static_cast<output_t>(val), shiftBits);
	}
};

template <int fractionalBits>
struct ConvertToFixedPoint<int32, int32, fractionalBits>
{
	typedef int32 input_t;
	typedef int32 output_t;
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(output_t)*8-1);
		return MPT_SC_RSHIFT_SIGNED(static_cast<output_t>(val), (sizeof(input_t)*8-1-fractionalBits));
	}
};

template <int fractionalBits>
struct ConvertToFixedPoint<int32, float32, fractionalBits>
{
	typedef float32 input_t;
	typedef int32 output_t;
	const float factor;
	MPT_FORCEINLINE ConvertToFixedPoint()
		: factor( static_cast<float>(1 << fractionalBits) )
	{
		return;
	}
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(input_t)*8-1);
		return mpt::saturate_cast<output_t>(MPT_SC_FASTROUND(val * factor));
	}
};

template <int fractionalBits>
struct ConvertToFixedPoint<int32, float64, fractionalBits>
{
	typedef float64 input_t;
	typedef int32 output_t;
	const double factor;
	MPT_FORCEINLINE ConvertToFixedPoint()
		: factor( static_cast<double>(1 << fractionalBits) )
	{
		return;
	}
	MPT_FORCEINLINE output_t operator() (input_t val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(input_t)*8-1);
		return mpt::saturate_cast<output_t>(MPT_SC_FASTROUND(val * factor));
	}
};



// Reads sample data with Func and passes it directly to Func2.
// Func1::output_t and Func2::input_t must be identical
template <typename Func2, typename Func1>
struct ConversionChain
{
	typedef typename Func1::input_t input_t;
	typedef typename Func2::output_t output_t;
	static constexpr std::size_t input_inc = Func1::input_inc;
	Func1 func1;
	Func2 func2;
	MPT_FORCEINLINE output_t operator() (const input_t *inBuf)
	{
		return func2(func1(inBuf));
	}
	MPT_FORCEINLINE ConversionChain(Func2 f2 = Func2(), Func1 f1 = Func1())
		: func1(f1)
		, func2(f2)
	{
		return;
	}
};


template <typename Tfixed, int fractionalBits, bool clipOutput>
struct ClipFixed
{
	typedef Tfixed input_t;
	typedef Tfixed output_t;
	MPT_FORCEINLINE Tfixed operator() (Tfixed val)
	{
		static_assert(fractionalBits >= 0 && fractionalBits <= sizeof(output_t)*8-1);
		if constexpr(clipOutput)
		{
			constexpr Tfixed clip_max = (Tfixed(1) << fractionalBits) - Tfixed(1);
			constexpr Tfixed clip_min = Tfixed(0) - (Tfixed(1) << fractionalBits);
			if(val < clip_min) val = clip_min;
			if(val > clip_max) val = clip_max;
			return val;
		} else
		{
			return val;
		}
	}
};


template <typename Tfloat, bool clipOutput>
struct ClipFloat;

template <bool clipOutput>
struct ClipFloat<float, clipOutput>
{
	typedef float input_t;
	typedef float output_t;
	MPT_FORCEINLINE float operator() (float val)
	{
		if constexpr(clipOutput)
		{
			if(val < -1.0f) val = -1.0f;
			if(val > 1.0f) val = 1.0f;
			return val;
		} else
		{
			return val;
		}
	}
};

template <bool clipOutput>
struct ClipFloat<double, clipOutput>
{
	typedef double input_t;
	typedef double output_t;
	MPT_FORCEINLINE double operator() (double val)
	{
		if constexpr(clipOutput)
		{
			if(val < -1.0) val = -1.0;
			if(val > 1.0) val = 1.0;
			return val;
		} else
		{
			return val;
		}
	}
};


template <typename Tsample>
struct Normalize;

template <>
struct Normalize<int32>
{
	typedef int32 input_t;
	typedef int32 output_t;
	typedef uint32 peak_t;
	uint32 maxVal;
	MPT_FORCEINLINE Normalize() : maxVal(0) { }
	MPT_FORCEINLINE void FindMax(input_t val)
	{
		if(val < 0)
		{
			if(val == int32_min)
			{
				maxVal = static_cast<uint32>(-static_cast<int64>(int32_min));
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
	MPT_FORCEINLINE output_t operator() (input_t val)
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
	MPT_FORCEINLINE Normalize() : maxVal(0.0f), maxValInv(1.0f) { }
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
	MPT_FORCEINLINE output_t operator() (input_t val)
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
	MPT_FORCEINLINE Normalize() : maxVal(0.0), maxValInv(1.0) { }
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
	MPT_FORCEINLINE output_t operator() (input_t val)
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
	MPT_FORCEINLINE output_t operator() (const input_t *inBuf)
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



#undef MPT_SC_RSHIFT_SIGNED
#undef MPT_SC_LSHIFT_SIGNED
#undef MPT_SC_FASTROUND



} // namespace SC



OPENMPT_NAMESPACE_END
