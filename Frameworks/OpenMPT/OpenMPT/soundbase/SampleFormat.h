/*
 * SampleFormat.h
 * ---------------
 * Purpose: Utility enum and funcion to describe sample formats.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "BuildSettings.h"


OPENMPT_NAMESPACE_BEGIN


struct int24;


enum SampleFormatEnum : uint8
{
	SampleFormatUnsigned8 =  9,       // do not change value (for compatibility with old configuration settings)
	SampleFormatInt8      =  8,       // do not change value (for compatibility with old configuration settings)
	SampleFormatInt16     = 16,       // do not change value (for compatibility with old configuration settings)
	SampleFormatInt24     = 24,       // do not change value (for compatibility with old configuration settings)
	SampleFormatInt32     = 32,       // do not change value (for compatibility with old configuration settings)
	SampleFormatFloat32   = 32 + 128, // do not change value (for compatibility with old configuration settings)
	SampleFormatFloat64   = 64 + 128, // do not change value (for compatibility with old configuration settings)
	SampleFormatInvalid   =  0
};

template <typename Container>
Container AllSampleFormats()
{
	return { SampleFormatFloat64, SampleFormatFloat32, SampleFormatInt32, SampleFormatInt24, SampleFormatInt16, SampleFormatInt8, SampleFormatUnsigned8 };
}

template <typename Container>
Container DefaultSampleFormats()
{
	return { SampleFormatFloat32, SampleFormatInt32, SampleFormatInt24, SampleFormatInt16, SampleFormatInt8 };
}

template<typename Tsample> struct SampleFormatTraits;
template<> struct SampleFormatTraits<uint8>  { static MPT_CONSTEXPR11_FUN SampleFormatEnum sampleFormat() { return SampleFormatUnsigned8; } };
template<> struct SampleFormatTraits<int8>   { static MPT_CONSTEXPR11_FUN SampleFormatEnum sampleFormat() { return SampleFormatInt8;      } };
template<> struct SampleFormatTraits<int16>  { static MPT_CONSTEXPR11_FUN SampleFormatEnum sampleFormat() { return SampleFormatInt16;     } };
template<> struct SampleFormatTraits<int24>  { static MPT_CONSTEXPR11_FUN SampleFormatEnum sampleFormat() { return SampleFormatInt24;     } };
template<> struct SampleFormatTraits<int32>  { static MPT_CONSTEXPR11_FUN SampleFormatEnum sampleFormat() { return SampleFormatInt32;     } };
template<> struct SampleFormatTraits<float>  { static MPT_CONSTEXPR11_FUN SampleFormatEnum sampleFormat() { return SampleFormatFloat32;   } };
template<> struct SampleFormatTraits<double> { static MPT_CONSTEXPR11_FUN SampleFormatEnum sampleFormat() { return SampleFormatFloat64;   } };

template<SampleFormatEnum sampleFormat> struct SampleFormatToType;
template<> struct SampleFormatToType<SampleFormatUnsigned8> { typedef uint8  type; };
template<> struct SampleFormatToType<SampleFormatInt8>      { typedef int8   type; };
template<> struct SampleFormatToType<SampleFormatInt16>     { typedef int16  type; };
template<> struct SampleFormatToType<SampleFormatInt24>     { typedef int24  type; };
template<> struct SampleFormatToType<SampleFormatInt32>     { typedef int32  type; };
template<> struct SampleFormatToType<SampleFormatFloat32>   { typedef float  type; };
template<> struct SampleFormatToType<SampleFormatFloat64>   { typedef double type; };


class SampleFormat
{

private:

	SampleFormatEnum value;

	template <typename T>
	static MPT_CONSTEXPR14_FUN SampleFormatEnum Sanitize(T x) noexcept
	{
		using uT = typename std::make_unsigned<T>::type;
		uT ux = x;
		if(ux == static_cast<uT>(-8))
		{
			ux = 8+1;
		}
		// float|64|32|16|8|?|?|unsigned
		ux &= 0b11111001;
		return static_cast<SampleFormatEnum>(ux);
	}

public:

	MPT_CONSTEXPR11_FUN SampleFormat() noexcept
		: value(SampleFormatInvalid)
	{
	}

	MPT_CONSTEXPR14_FUN SampleFormat(SampleFormatEnum v) noexcept
		: value(Sanitize(v))
	{
	}

	MPT_CONSTEXPR11_FUN bool IsValid() const noexcept
	{
		return value != SampleFormatInvalid;
	}

	MPT_CONSTEXPR11_FUN bool IsUnsigned() const noexcept
	{
		return IsValid() && (value == SampleFormatUnsigned8);
	}
	MPT_CONSTEXPR11_FUN bool IsFloat() const noexcept
	{
		return IsValid() && ((value == SampleFormatFloat32) || (value == SampleFormatFloat64));
	}
	MPT_CONSTEXPR11_FUN bool IsInt() const noexcept
	{
		return IsValid() && ((value != SampleFormatFloat32) && (value != SampleFormatFloat64));
	}
	MPT_CONSTEXPR11_FUN uint8 GetBitsPerSample() const noexcept
	{
		return
			!IsValid() ? 0 :
			(value == SampleFormatUnsigned8) ?  8 :
			(value == SampleFormatInt8)      ?  8 :
			(value == SampleFormatInt16)     ? 16 :
			(value == SampleFormatInt24)     ? 24 :
			(value == SampleFormatInt32)     ? 32 :
			(value == SampleFormatFloat32)   ? 32 :
			(value == SampleFormatFloat64)   ? 64 :
			0;
	}

	MPT_CONSTEXPR11_FUN operator SampleFormatEnum () const noexcept
	{
		return value;
	}

	// backward compatibility, conversion to/from integers
	static MPT_CONSTEXPR14_FUN SampleFormat FromInt(int x) noexcept
	{
		return SampleFormat(Sanitize(x));
	}
	static MPT_CONSTEXPR14_FUN int ToInt(SampleFormat x) noexcept
	{
		return x.value;
	}
	MPT_CONSTEXPR11_FUN int AsInt() const noexcept
	{
		return value;
	}

};


OPENMPT_NAMESPACE_END
