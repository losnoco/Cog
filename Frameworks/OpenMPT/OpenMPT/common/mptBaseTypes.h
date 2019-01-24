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

#include <cstddef>
#include <cstdint>

#if MPT_GCC_BEFORE(4,9,0)
#include <stddef.h>
#endif
#include <stdint.h>



OPENMPT_NAMESPACE_BEGIN



typedef std::int8_t   int8;
typedef std::int16_t  int16;
typedef std::int32_t  int32;
typedef std::int64_t  int64;
typedef std::uint8_t  uint8;
typedef std::uint16_t uint16;
typedef std::uint32_t uint32;
typedef std::uint64_t uint64;

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


typedef float float32;
MPT_STATIC_ASSERT(sizeof(float32) == 4);

typedef double float64;
MPT_STATIC_ASSERT(sizeof(float64) == 8);


MPT_STATIC_ASSERT(sizeof(std::uintptr_t) == sizeof(void*));


MPT_STATIC_ASSERT(std::numeric_limits<unsigned char>::digits == 8);

MPT_STATIC_ASSERT(sizeof(char) == 1);

#if MPT_CXX_AT_LEAST(17)
namespace mpt {
using byte = std::byte;
} // namespace mpt
#define MPT_BYTE_IS_STD_BYTE 1
#else
// In C++11 and C++14, a C++17 compatible definition of byte would not be required to be allowed to alias other types,
// thus just use a typedef for unsigned char which is guaranteed to be allowed to alias.
//enum class byte : unsigned char { };
namespace mpt {
typedef unsigned char byte;
} // namespace mpt
#define MPT_BYTE_IS_STD_BYTE 0
#endif
MPT_STATIC_ASSERT(sizeof(mpt::byte) == 1);
MPT_STATIC_ASSERT(alignof(mpt::byte) == 1);


namespace mpt {
#if MPT_GCC_BEFORE(4,9,0)
typedef ::max_align_t max_align_t;
#else
typedef std::max_align_t max_align_t;
#endif
} // namespace mpt


namespace mpt {
constexpr int arch_bits = sizeof(void*) * 8;
constexpr std::size_t pointer_size = sizeof(void*);
} // namespace mpt

MPT_STATIC_ASSERT(mpt::arch_bits == static_cast<int>(mpt::pointer_size) * 8);



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

#define MPT_SOURCE_LOCATION_CURRENT() mpt::source_location::current( __FILE__ , __FUNCTION__ , __LINE__ , 0 )

} // namespace mpt



OPENMPT_NAMESPACE_END
