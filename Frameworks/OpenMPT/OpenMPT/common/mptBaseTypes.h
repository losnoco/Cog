/*
 * mptBaseTypes.h
 * --------------
 * Purpose: Basic data type definitions.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"



#include "mptBaseMacros.h"

#include <array>
#include <limits>
#if MPT_CXX_AT_LEAST(20)
#include <source_location>
#endif // C++20

#include <cstddef>
#include <cstdint>

#include <stdint.h>



OPENMPT_NAMESPACE_BEGIN



namespace mpt
{
template <bool cond, typename Ta, typename Tb>
struct select_type
{
};
template <typename Ta, typename Tb>
struct select_type<true, Ta, Tb>
{
	using type = Ta;
};
template <typename Ta, typename Tb>
struct select_type<false, Ta, Tb>
{
	using type = Tb;
};
} // namespace mpt



using int8   = std::int8_t;
using int16  = std::int16_t;
using int32  = std::int32_t;
using int64  = std::int64_t;
using uint8  = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;

constexpr int8 int8_min     = std::numeric_limits<int8>::min();
constexpr int16 int16_min   = std::numeric_limits<int16>::min();
constexpr int32 int32_min   = std::numeric_limits<int32>::min();
constexpr int64 int64_min   = std::numeric_limits<int64>::min();

constexpr int8 int8_max     = std::numeric_limits<int8>::max();
constexpr int16 int16_max   = std::numeric_limits<int16>::max();
constexpr int32 int32_max   = std::numeric_limits<int32>::max();
constexpr int64 int64_max   = std::numeric_limits<int64>::max();

constexpr uint8 uint8_max   = std::numeric_limits<uint8>::max();
constexpr uint16 uint16_max = std::numeric_limits<uint16>::max();
constexpr uint32 uint32_max = std::numeric_limits<uint32>::max();
constexpr uint64 uint64_max = std::numeric_limits<uint64>::max();



// fp half
// n/a

// fp single
using single = float;
constexpr single operator"" _fs(long double lit)
{
	return static_cast<single>(lit);
}

// fp double
constexpr double operator"" _fd(long double lit)
{
	return static_cast<double>(lit);
}

// fp extended
constexpr long double operator"" _fe(long double lit)
{
	return static_cast<long double>(lit);
}

// fp quad
// n/a

using float32 = mpt::select_type<sizeof(float) == 4,
		float
	,
		mpt::select_type<sizeof(double) == 4,
			double
		,
      mpt::select_type<sizeof(long double) == 4,
				long double
			,
				float
			>::type
		>::type
	>::type;
constexpr float32 operator"" _f32(long double lit)
{
	return static_cast<float32>(lit);
}

using float64 = mpt::select_type<sizeof(float) == 8,
		float
	,
		mpt::select_type<sizeof(double) == 8,
			double
		,
      mpt::select_type<sizeof(long double) == 8,
				long double
			,
				double
			>::type
		>::type
	>::type;
constexpr float64 operator"" _f64(long double lit)
{
	return static_cast<float64>(lit);
}

namespace mpt
{
template <typename T>
struct float_traits
{
	static constexpr bool is_float = !std::numeric_limits<T>::is_integer;
	static constexpr bool is_hard = is_float && !MPT_COMPILER_QUIRK_FLOAT_EMULATED;
	static constexpr bool is_soft = is_float && MPT_COMPILER_QUIRK_FLOAT_EMULATED;
	static constexpr bool is_float32 = is_float && (sizeof(T) == 4);
	static constexpr bool is_float64 = is_float && (sizeof(T) == 8);
	static constexpr bool is_native_endian = is_float && !MPT_COMPILER_QUIRK_FLOAT_NOTNATIVEENDIAN;
	static constexpr bool is_ieee754_binary = is_float && std::numeric_limits<T>::is_iec559 && !MPT_COMPILER_QUIRK_FLOAT_NOTIEEE754;
	static constexpr bool is_ieee754_binary32 = is_float && is_ieee754_binary && is_float32;
	static constexpr bool is_ieee754_binary64 = is_float && is_ieee754_binary && is_float64;
	static constexpr bool is_ieee754_binary32ne = is_float && is_ieee754_binary && is_float32 && is_native_endian;
	static constexpr bool is_ieee754_binary64ne = is_float && is_ieee754_binary && is_float64 && is_native_endian;
};
}  // namespace mpt

#if MPT_COMPILER_QUIRK_FLOAT_PREFER32
using nativefloat = float32;
#elif MPT_COMPILER_QUIRK_FLOAT_PREFER64
using nativefloat = float64;
#else
// prefer smaller floats, but try to use IEEE754 floats
using nativefloat = mpt::select_type<std::numeric_limits<float>::is_iec559,
		float
	,
		mpt::select_type<std::numeric_limits<double>::is_iec559,
			double
		,
			mpt::select_type<std::numeric_limits<long double>::is_iec559,
				long double
			,
				float
			>::type
		>::type
	>::type;	
#endif
constexpr nativefloat operator"" _nf(long double lit)
{
	return static_cast<nativefloat>(lit);
}



static_assert(sizeof(std::uintptr_t) == sizeof(void*));



static_assert(std::numeric_limits<unsigned char>::digits == 8);

static_assert(sizeof(char) == 1);

static_assert(sizeof(std::byte) == 1);
static_assert(alignof(std::byte) == 1);


namespace mpt {
constexpr int arch_bits = sizeof(void*) * 8;
constexpr std::size_t pointer_size = sizeof(void*);
} // namespace mpt

static_assert(mpt::arch_bits == static_cast<int>(mpt::pointer_size) * 8);



namespace mpt {

template <typename T>
struct limits
{
	static constexpr typename std::remove_cv<T>::type min() noexcept { return std::numeric_limits<typename std::remove_cv<T>::type>::min(); }
	static constexpr typename std::remove_cv<T>::type max() noexcept { return std::numeric_limits<typename std::remove_cv<T>::type>::max(); }
};

} // namespace mpt



namespace mpt
{

#if MPT_CXX_AT_LEAST(20)

using std::source_location;

#define MPT_SOURCE_LOCATION_CURRENT() std::source_location::current()

#else // !C++20

// compatible with std::experimental::source_location from Library Fundamentals TS v2.
struct source_location
{
private:
	const char* m_file_name;
	const char* m_function_name;
	uint32 m_line;
	uint32 m_column;
public:
	constexpr source_location() noexcept
		: m_file_name("")
		, m_function_name("")
		, m_line(0)
		, m_column(0)
	{
	}
	constexpr source_location(const char* file, const char* function, uint32 line, uint32 column) noexcept
		: m_file_name(file)
		, m_function_name(function)
		, m_line(line)
		, m_column(column)
	{
	}
	source_location(const source_location&) = default;
	source_location(source_location&&) = default;
	//static constexpr current() noexcept;  // use MPT_SOURCE_LOCATION_CURRENT()
	static constexpr source_location current(const char* file, const char* function, uint32 line, uint32 column) noexcept
	{
		return source_location(file, function, line, column);
	}
	constexpr uint32 line() const noexcept
	{
		return m_line;
	}
	constexpr uint32 column() const noexcept
	{
		return m_column;
	}
	constexpr const char* file_name() const noexcept
	{
		return m_file_name;
	}
	constexpr const char* function_name() const noexcept
	{
		return m_function_name;
	}
};

#define MPT_SOURCE_LOCATION_CURRENT() mpt::source_location::current( __FILE__ , __func__ , __LINE__ , 0 )

#endif // C++20

} // namespace mpt



OPENMPT_NAMESPACE_END
