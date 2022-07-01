/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_BIT_HPP
#define MPT_BASE_BIT_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/integer.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/base/macros.hpp"

#if MPT_CXX_AT_LEAST(20)
#include <bit>
#else // !C++20
#include <array>
#include <limits>
#endif // C++20
#include <type_traits>

#include <cstddef>
#if MPT_CXX_BEFORE(20)
#include <cstring>
#endif // !C++20



namespace mpt {
inline namespace MPT_INLINE_NS {



#if MPT_CXX_AT_LEAST(20) && !MPT_CLANG_BEFORE(14, 0, 0)
using std::bit_cast;
#else  // !C++20
// C++2a compatible bit_cast.
// Not implementing constexpr because this is not easily possible pre C++20.
template <typename Tdst, typename Tsrc>
MPT_FORCEINLINE typename std::enable_if<(sizeof(Tdst) == sizeof(Tsrc)) && std::is_trivially_copyable<Tsrc>::value && std::is_trivially_copyable<Tdst>::value, Tdst>::type bit_cast(const Tsrc & src) noexcept {
	Tdst dst{};
	std::memcpy(&dst, &src, sizeof(Tdst));
	return dst;
}
#endif // C++20



#if MPT_CXX_AT_LEAST(20)

using std::endian;

static_assert(mpt::endian::big != mpt::endian::little, "platform with all scalar types having size 1 is not supported");

constexpr mpt::endian get_endian() noexcept {
	return mpt::endian::native;
}

constexpr bool endian_is_little() noexcept {
	return get_endian() == mpt::endian::little;
}

constexpr bool endian_is_big() noexcept {
	return get_endian() == mpt::endian::big;
}

constexpr bool endian_is_weird() noexcept {
	return !endian_is_little() && !endian_is_big();
}

#else // !C++20

#if !MPT_COMPILER_GENERIC

#if MPT_COMPILER_MSVC
#define MPT_PLATFORM_LITTLE_ENDIAN
#elif MPT_COMPILER_GCC || MPT_COMPILER_CLANG
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define MPT_PLATFORM_BIG_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define MPT_PLATFORM_LITTLE_ENDIAN
#endif
#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && defined(__ORDER_LITTLE_ENDIAN__)
#if __ORDER_BIG_ENDIAN__ != __ORDER_LITTLE_ENDIAN__
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define MPT_PLATFORM_BIG_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define MPT_PLATFORM_LITTLE_ENDIAN
#endif
#endif
#endif

// fallback:
#if !defined(MPT_PLATFORM_BIG_ENDIAN) && !defined(MPT_PLATFORM_LITTLE_ENDIAN)
#if (defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)) \
	|| (defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)) \
	|| (defined(_STLP_BIG_ENDIAN) && !defined(_STLP_LITTLE_ENDIAN))
#define MPT_PLATFORM_BIG_ENDIAN
#elif (defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)) \
	|| (defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)) \
	|| (defined(_STLP_LITTLE_ENDIAN) && !defined(_STLP_BIG_ENDIAN))
#define MPT_PLATFORM_LITTLE_ENDIAN
#elif defined(__hpux) || defined(__hppa) \
	|| defined(_MIPSEB) \
	|| defined(__s390__)
#define MPT_PLATFORM_BIG_ENDIAN
#elif defined(__i386__) || defined(_M_IX86) \
	|| defined(__amd64) || defined(__amd64__) || defined(_M_AMD64) || defined(__x86_64) || defined(__x86_64__) || defined(_M_X64) \
	|| defined(__bfin__)
#define MPT_PLATFORM_LITTLE_ENDIAN
#endif
#endif

#endif // !MPT_COMPILER_GENERIC

enum class endian {
	little = 0x78563412u,
	big = 0x12345678u,
	weird = 1u,
#if MPT_COMPILER_GENERIC
	native = 0u,
#elif defined(MPT_PLATFORM_LITTLE_ENDIAN)
	native = little,
#elif defined(MPT_PLATFORM_BIG_ENDIAN)
	native = big,
#else
	native = 0u,
#endif
};

static_assert(mpt::endian::big != mpt::endian::little, "platform with all scalar types having size 1 is not supported");

MPT_FORCEINLINE mpt::endian endian_probe() noexcept {
	using endian_probe_type = uint32;
	static_assert(sizeof(endian_probe_type) == 4);
	constexpr endian_probe_type endian_probe_big = 0x12345678u;
	constexpr endian_probe_type endian_probe_little = 0x78563412u;
	const std::array<std::byte, sizeof(endian_probe_type)> probe{
		{std::byte{0x12}, std::byte{0x34}, std::byte{0x56}, std::byte{0x78}}
    };
	const endian_probe_type test = mpt::bit_cast<endian_probe_type>(probe);
	mpt::endian result = mpt::endian::native;
	switch (test) {
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

MPT_FORCEINLINE mpt::endian get_endian() noexcept {
#if MPT_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 6285) // false-positive: (<non-zero constant> || <non-zero constant>) is always a non-zero constant.
#endif                          // MPT_COMPILER_MSVC
	if constexpr ((mpt::endian::native == mpt::endian::little) || (mpt::endian::native == mpt::endian::big)) {
		return mpt::endian::native;
	} else {
		return mpt::endian_probe();
	}
#if MPT_COMPILER_MSVC
#pragma warning(pop)
#endif // MPT_COMPILER_MSVC
}

MPT_FORCEINLINE bool endian_is_little() noexcept {
	return get_endian() == mpt::endian::little;
}

MPT_FORCEINLINE bool endian_is_big() noexcept {
	return get_endian() == mpt::endian::big;
}

MPT_FORCEINLINE bool endian_is_weird() noexcept {
	return !endian_is_little() && !endian_is_big();
}

#endif // C++20



#if MPT_CXX_AT_LEAST(20) && MPT_MSVC_AT_LEAST(2022, 1) && !MPT_CLANG_BEFORE(12, 0, 0)

// Disabled for VS2022.0 because of
// <https://developercommunity.visualstudio.com/t/vs2022-cl-193030705-generates-non-universally-avai/1578571>
// / <https://github.com/microsoft/STL/issues/2330> with fix already queued
// (<https://github.com/microsoft/STL/pull/2333>).

using std::bit_ceil;
using std::bit_floor;
using std::bit_width;
using std::countl_one;
using std::countl_zero;
using std::countr_one;
using std::countr_zero;
using std::has_single_bit;
using std::popcount;
using std::rotl;
using std::rotr;

#else // !C++20

// C++20 <bit> header.
// Note that we do not use SFINAE here but instead rely on static_assert.

template <typename T>
constexpr int popcount(T val) noexcept {
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::is_unsigned<T>::value);
	int result = 0;
	while (val > 0) {
		if (val & 0x1) {
			result++;
		}
		val >>= 1;
	}
	return result;
}

template <typename T>
constexpr bool has_single_bit(T x) noexcept {
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::is_unsigned<T>::value);
	return mpt::popcount(x) == 1;
}

template <typename T>
constexpr T bit_ceil(T x) noexcept {
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::is_unsigned<T>::value);
	T result = 1;
	while (result < x) {
		T newresult = result << 1;
		if (newresult < result) {
			return 0;
		}
		result = newresult;
	}
	return result;
}

template <typename T>
constexpr T bit_floor(T x) noexcept {
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::is_unsigned<T>::value);
	if (x == 0) {
		return 0;
	}
	T result = 1;
	do {
		T newresult = result << 1;
		if (newresult < result) {
			return result;
		}
		result = newresult;
	} while (result <= x);
	return result >> 1;
}

template <typename T>
constexpr T bit_width(T x) noexcept {
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::is_unsigned<T>::value);
	T result = 0;
	while (x > 0) {
		x >>= 1;
		result += 1;
	}
	return result;
}

template <typename T>
constexpr int countl_zero(T x) noexcept {
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::is_unsigned<T>::value);
	int count = 0;
	for (int bit = std::numeric_limits<T>::digits - 1; bit >= 0; --bit) {
		if ((x & (1u << bit)) == 0u) {
			count++;
		} else {
			break;
		}
	}
	return count;
}

template <typename T>
constexpr int countl_one(T x) noexcept {
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::is_unsigned<T>::value);
	int count = 0;
	for (int bit = std::numeric_limits<T>::digits - 1; bit >= 0; --bit) {
		if ((x & (1u << bit)) != 0u) {
			count++;
		} else {
			break;
		}
	}
	return count;
}

template <typename T>
constexpr int countr_zero(T x) noexcept {
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::is_unsigned<T>::value);
	int count = 0;
	for (int bit = 0; bit < std::numeric_limits<T>::digits; ++bit) {
		if ((x & (1u << bit)) == 0u) {
			count++;
		} else {
			break;
		}
	}
	return count;
}

template <typename T>
constexpr int countr_one(T x) noexcept {
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::is_unsigned<T>::value);
	int count = 0;
	for (int bit = 0; bit < std::numeric_limits<T>::digits; ++bit) {
		if ((x & (1u << bit)) != 0u) {
			count++;
		} else {
			break;
		}
	}
	return count;
}

template <typename T>
constexpr T rotl_impl(T x, int r) noexcept {
	auto N = std::numeric_limits<T>::digits;
	return (x >> (N - r)) | (x << r);
}

template <typename T>
constexpr T rotr_impl(T x, int r) noexcept {
	auto N = std::numeric_limits<T>::digits;
	return (x << (N - r)) | (x >> r);
}

template <typename T>
constexpr T rotl(T x, int s) noexcept {
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::is_unsigned<T>::value);
	auto N = std::numeric_limits<T>::digits;
	auto r = s % N;
	return (s < 0) ? mpt::rotr_impl(x, -s) : ((x >> (N - r)) | (x << r));
}

template <typename T>
constexpr T rotr(T x, int s) noexcept {
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(std::is_unsigned<T>::value);
	auto N = std::numeric_limits<T>::digits;
	auto r = s % N;
	return (s < 0) ? mpt::rotl_impl(x, -s) : ((x << (N - r)) | (x >> r));
}

#endif // C++20



template <typename T>
constexpr int lower_bound_entropy_bits(T x_) {
	typename std::make_unsigned<T>::type x = static_cast<typename std::make_unsigned<T>::type>(x_);
	return mpt::bit_width(x) == static_cast<typename std::make_unsigned<T>::type>(mpt::popcount(x)) ? mpt::bit_width(x) : mpt::bit_width(x) - 1;
}


template <typename T>
constexpr bool is_mask(T x) {
	static_assert(std::is_integral<T>::value);
	typedef typename std::make_unsigned<T>::type unsigned_T;
	unsigned_T ux = static_cast<unsigned_T>(x);
	unsigned_T mask = 0;
	for (std::size_t bits = 0; bits <= (sizeof(unsigned_T) * 8); ++bits) {
		mask = (mask << 1) | 1u;
		if (ux == mask) {
			return true;
		}
	}
	return false;
}



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_BASE_BIT_HPP
