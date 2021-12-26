/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_ENDIAN_INTEGER_HPP
#define MPT_ENDIAN_INTEGER_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/bit.hpp"
#include "mpt/base/integer.hpp"
#include "mpt/base/macros.hpp"
#include "mpt/base/memory.hpp"
#include "mpt/base/namespace.hpp"

#include <array>
#include <limits>
#include <type_traits>

#include <cstddef>
#include <cstdint>
#include <cstring>

#if MPT_COMPILER_MSVC
#include <intrin.h>
#endif



namespace mpt {
inline namespace MPT_INLINE_NS {



struct BigEndian_tag {
	static constexpr mpt::endian endian = mpt::endian::big;
};

struct LittleEndian_tag {
	static constexpr mpt::endian endian = mpt::endian::little;
};



constexpr inline uint16 constexpr_bswap16(uint16 x) noexcept {
	return uint16(0)
		| ((x >> 8) & 0x00FFu)
		| ((x << 8) & 0xFF00u);
}

constexpr inline uint32 constexpr_bswap32(uint32 x) noexcept {
	return uint32(0)
		| ((x & 0x000000FFu) << 24)
		| ((x & 0x0000FF00u) << 8)
		| ((x & 0x00FF0000u) >> 8)
		| ((x & 0xFF000000u) >> 24);
}

constexpr inline uint64 constexpr_bswap64(uint64 x) noexcept {
	return uint64(0)
		| (((x >> 0) & 0xffull) << 56)
		| (((x >> 8) & 0xffull) << 48)
		| (((x >> 16) & 0xffull) << 40)
		| (((x >> 24) & 0xffull) << 32)
		| (((x >> 32) & 0xffull) << 24)
		| (((x >> 40) & 0xffull) << 16)
		| (((x >> 48) & 0xffull) << 8)
		| (((x >> 56) & 0xffull) << 0);
}

#if MPT_COMPILER_GCC
#define MPT_bswap16 __builtin_bswap16
#define MPT_bswap32 __builtin_bswap32
#define MPT_bswap64 __builtin_bswap64
#elif MPT_COMPILER_MSVC
#define MPT_bswap16 _byteswap_ushort
#define MPT_bswap32 _byteswap_ulong
#define MPT_bswap64 _byteswap_uint64
#endif

// No intrinsics available
#ifndef MPT_bswap16
#define MPT_bswap16(x) mpt::constexpr_bswap16(x)
#endif
#ifndef MPT_bswap32
#define MPT_bswap32(x) mpt::constexpr_bswap32(x)
#endif
#ifndef MPT_bswap64
#define MPT_bswap64(x) mpt::constexpr_bswap64(x)
#endif



template <typename T, typename Tendian, std::size_t size>
MPT_CONSTEXPRINLINE std::array<std::byte, size> EndianEncode(T val) noexcept {
	static_assert(Tendian::endian == mpt::endian::little || Tendian::endian == mpt::endian::big);
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(!std::numeric_limits<T>::is_signed);
	static_assert(sizeof(T) == size);
	using base_type = T;
	using unsigned_base_type = typename std::make_unsigned<base_type>::type;
	using endian_type = Tendian;
	unsigned_base_type uval = static_cast<unsigned_base_type>(val);
	std::array<std::byte, size> data{};
	if constexpr (endian_type::endian == mpt::endian::little) {
		for (std::size_t i = 0; i < sizeof(base_type); ++i) {
			data[i] = static_cast<std::byte>(static_cast<uint8>((uval >> (i * 8)) & 0xffu));
		}
	} else {
		for (std::size_t i = 0; i < sizeof(base_type); ++i) {
			data[(sizeof(base_type) - 1) - i] = static_cast<std::byte>(static_cast<uint8>((uval >> (i * 8)) & 0xffu));
		}
	}
	return data;
}

template <typename T, typename Tendian, std::size_t size>
MPT_CONSTEXPRINLINE T EndianDecode(std::array<std::byte, size> data) noexcept {
	static_assert(Tendian::endian == mpt::endian::little || Tendian::endian == mpt::endian::big);
	static_assert(std::numeric_limits<T>::is_integer);
	static_assert(!std::numeric_limits<T>::is_signed);
	static_assert(sizeof(T) == size);
	using base_type = T;
	using unsigned_base_type = typename std::make_unsigned<base_type>::type;
	using endian_type = Tendian;
	base_type val = base_type();
	unsigned_base_type uval = unsigned_base_type();
	if constexpr (endian_type::endian == mpt::endian::little) {
		for (std::size_t i = 0; i < sizeof(base_type); ++i) {
			uval |= static_cast<unsigned_base_type>(static_cast<uint8>(data[i])) << (i * 8);
		}
	} else {
		for (std::size_t i = 0; i < sizeof(base_type); ++i) {
			uval |= static_cast<unsigned_base_type>(static_cast<uint8>(data[(sizeof(base_type) - 1) - i])) << (i * 8);
		}
	}
	val = static_cast<base_type>(uval);
	return val;
}


MPT_CONSTEXPR20_FUN uint64 SwapBytesImpl(uint64 value) noexcept {
	MPT_MAYBE_CONSTANT_IF(MPT_IS_CONSTANT_EVALUATED20()) {
		return mpt::constexpr_bswap64(value);
	}
	else {
		return MPT_bswap64(value);
	}
}

MPT_CONSTEXPR20_FUN uint32 SwapBytesImpl(uint32 value) noexcept {
	MPT_MAYBE_CONSTANT_IF(MPT_IS_CONSTANT_EVALUATED20()) {
		return mpt::constexpr_bswap32(value);
	}
	else {
		return MPT_bswap32(value);
	}
}

MPT_CONSTEXPR20_FUN uint16 SwapBytesImpl(uint16 value) noexcept {
	MPT_MAYBE_CONSTANT_IF(MPT_IS_CONSTANT_EVALUATED20()) {
		return mpt::constexpr_bswap16(value);
	}
	else {
		return MPT_bswap16(value);
	}
}

MPT_CONSTEXPR20_FUN int64 SwapBytesImpl(int64 value) noexcept {
	MPT_MAYBE_CONSTANT_IF(MPT_IS_CONSTANT_EVALUATED20()) {
		return mpt::constexpr_bswap64(value);
	}
	else {
		return MPT_bswap64(value);
	}
}

MPT_CONSTEXPR20_FUN int32 SwapBytesImpl(int32 value) noexcept {
	MPT_MAYBE_CONSTANT_IF(MPT_IS_CONSTANT_EVALUATED20()) {
		return mpt::constexpr_bswap32(value);
	}
	else {
		return MPT_bswap32(value);
	}
}

MPT_CONSTEXPR20_FUN int16 SwapBytesImpl(int16 value) noexcept {
	MPT_MAYBE_CONSTANT_IF(MPT_IS_CONSTANT_EVALUATED20()) {
		return mpt::constexpr_bswap16(value);
	}
	else {
		return MPT_bswap16(value);
	}
}

// Do NOT remove these overloads, even if they seem useless.
// We do not want risking to extend 8bit integers to int and then
// endian-converting and casting back to int.
// Thus these overloads.

MPT_CONSTEXPR20_FUN uint8 SwapBytesImpl(uint8 value) noexcept {
	return value;
}

MPT_CONSTEXPR20_FUN int8 SwapBytesImpl(int8 value) noexcept {
	return value;
}

MPT_CONSTEXPR20_FUN char SwapBytesImpl(char value) noexcept {
	return value;
}

#undef MPT_bswap16
#undef MPT_bswap32
#undef MPT_bswap64



// On-disk integer types with defined endianness and no alignemnt requirements
// Note: To easily debug module loaders (and anything else that uses this
// wrapper struct), you can use the Debugger Visualizers available in
// build/vs/debug/ to conveniently view the wrapped contents.

template <typename T, typename Tendian>
struct packed {
public:
	using base_type = T;
	using endian_type = Tendian;

public:
	std::array<std::byte, sizeof(base_type)> data;

public:
	MPT_CONSTEXPR20_FUN void set(base_type val) noexcept {
		static_assert(std::numeric_limits<T>::is_integer);
		MPT_MAYBE_CONSTANT_IF(MPT_IS_CONSTANT_EVALUATED20()) {
			if constexpr (endian_type::endian == mpt::endian::big) {
				typename std::make_unsigned<base_type>::type uval = val;
				for (std::size_t i = 0; i < sizeof(base_type); ++i) {
					data[i] = static_cast<std::byte>((uval >> (8 * (sizeof(base_type) - 1 - i))) & 0xffu);
				}
			} else {
				typename std::make_unsigned<base_type>::type uval = val;
				for (std::size_t i = 0; i < sizeof(base_type); ++i) {
					data[i] = static_cast<std::byte>((uval >> (8 * i)) & 0xffu);
				}
			}
		}
		else {
			if constexpr (mpt::endian::native == mpt::endian::little || mpt::endian::native == mpt::endian::big) {
				if constexpr (mpt::endian::native != endian_type::endian) {
					val = mpt::SwapBytesImpl(val);
				}
				std::memcpy(data.data(), &val, sizeof(val));
			} else {
				using unsigned_base_type = typename std::make_unsigned<base_type>::type;
				data = EndianEncode<unsigned_base_type, Tendian, sizeof(T)>(val);
			}
		}
	}
	MPT_CONSTEXPR20_FUN base_type get() const noexcept {
		static_assert(std::numeric_limits<T>::is_integer);
		MPT_MAYBE_CONSTANT_IF(MPT_IS_CONSTANT_EVALUATED20()) {
			if constexpr (endian_type::endian == mpt::endian::big) {
				typename std::make_unsigned<base_type>::type uval = 0;
				for (std::size_t i = 0; i < sizeof(base_type); ++i) {
					uval |= static_cast<typename std::make_unsigned<base_type>::type>(data[i]) << (8 * (sizeof(base_type) - 1 - i));
				}
				return static_cast<base_type>(uval);
			} else {
				typename std::make_unsigned<base_type>::type uval = 0;
				for (std::size_t i = 0; i < sizeof(base_type); ++i) {
					uval |= static_cast<typename std::make_unsigned<base_type>::type>(data[i]) << (8 * i);
				}
				return static_cast<base_type>(uval);
			}
		}
		else {
			if constexpr (mpt::endian::native == mpt::endian::little || mpt::endian::native == mpt::endian::big) {
				base_type val = base_type();
				std::memcpy(&val, data.data(), sizeof(val));
				if constexpr (mpt::endian::native != endian_type::endian) {
					val = mpt::SwapBytesImpl(val);
				}
				return val;
			} else {
				using unsigned_base_type = typename std::make_unsigned<base_type>::type;
				return EndianDecode<unsigned_base_type, Tendian, sizeof(T)>(data);
			}
		}
	}
	MPT_CONSTEXPR20_FUN packed & operator=(const base_type & val) noexcept {
		set(val);
		return *this;
	}
	MPT_CONSTEXPR20_FUN operator base_type() const noexcept {
		return get();
	}

public:
	MPT_CONSTEXPR20_FUN packed & operator&=(base_type val) noexcept {
		set(get() & val);
		return *this;
	}
	MPT_CONSTEXPR20_FUN packed & operator|=(base_type val) noexcept {
		set(get() | val);
		return *this;
	}
	MPT_CONSTEXPR20_FUN packed & operator^=(base_type val) noexcept {
		set(get() ^ val);
		return *this;
	}
	MPT_CONSTEXPR20_FUN packed & operator+=(base_type val) noexcept {
		set(get() + val);
		return *this;
	}
	MPT_CONSTEXPR20_FUN packed & operator-=(base_type val) noexcept {
		set(get() - val);
		return *this;
	}
	MPT_CONSTEXPR20_FUN packed & operator*=(base_type val) noexcept {
		set(get() * val);
		return *this;
	}
	MPT_CONSTEXPR20_FUN packed & operator/=(base_type val) noexcept {
		set(get() / val);
		return *this;
	}
	MPT_CONSTEXPR20_FUN packed & operator%=(base_type val) noexcept {
		set(get() % val);
		return *this;
	}
	MPT_CONSTEXPR20_FUN packed & operator++() noexcept { // prefix
		set(get() + 1);
		return *this;
	}
	MPT_CONSTEXPR20_FUN packed & operator--() noexcept { // prefix
		set(get() - 1);
		return *this;
	}
	MPT_CONSTEXPR20_FUN base_type operator++(int) noexcept { // postfix
		base_type old = get();
		set(old + 1);
		return old;
	}
	MPT_CONSTEXPR20_FUN base_type operator--(int) noexcept { // postfix
		base_type old = get();
		set(old - 1);
		return old;
	}
};

using int64le = packed<int64, LittleEndian_tag>;
using int32le = packed<int32, LittleEndian_tag>;
using int16le = packed<int16, LittleEndian_tag>;
using int8le = packed<int8, LittleEndian_tag>;
using uint64le = packed<uint64, LittleEndian_tag>;
using uint32le = packed<uint32, LittleEndian_tag>;
using uint16le = packed<uint16, LittleEndian_tag>;
using uint8le = packed<uint8, LittleEndian_tag>;

using int64be = packed<int64, BigEndian_tag>;
using int32be = packed<int32, BigEndian_tag>;
using int16be = packed<int16, BigEndian_tag>;
using int8be = packed<int8, BigEndian_tag>;
using uint64be = packed<uint64, BigEndian_tag>;
using uint32be = packed<uint32, BigEndian_tag>;
using uint16be = packed<uint16, BigEndian_tag>;
using uint8be = packed<uint8, BigEndian_tag>;

constexpr bool declare_binary_safe(const int64le &) {
	return true;
}
constexpr bool declare_binary_safe(const int32le &) {
	return true;
}
constexpr bool declare_binary_safe(const int16le &) {
	return true;
}
constexpr bool declare_binary_safe(const int8le &) {
	return true;
}
constexpr bool declare_binary_safe(const uint64le &) {
	return true;
}
constexpr bool declare_binary_safe(const uint32le &) {
	return true;
}
constexpr bool declare_binary_safe(const uint16le &) {
	return true;
}
constexpr bool declare_binary_safe(const uint8le &) {
	return true;
}

constexpr bool declare_binary_safe(const int64be &) {
	return true;
}
constexpr bool declare_binary_safe(const int32be &) {
	return true;
}
constexpr bool declare_binary_safe(const int16be &) {
	return true;
}
constexpr bool declare_binary_safe(const int8be &) {
	return true;
}
constexpr bool declare_binary_safe(const uint64be &) {
	return true;
}
constexpr bool declare_binary_safe(const uint32be &) {
	return true;
}
constexpr bool declare_binary_safe(const uint16be &) {
	return true;
}
constexpr bool declare_binary_safe(const uint8be &) {
	return true;
}

static_assert(mpt::check_binary_size<int64le>(8));
static_assert(mpt::check_binary_size<int32le>(4));
static_assert(mpt::check_binary_size<int16le>(2));
static_assert(mpt::check_binary_size<int8le>(1));
static_assert(mpt::check_binary_size<uint64le>(8));
static_assert(mpt::check_binary_size<uint32le>(4));
static_assert(mpt::check_binary_size<uint16le>(2));
static_assert(mpt::check_binary_size<uint8le>(1));

static_assert(mpt::check_binary_size<int64be>(8));
static_assert(mpt::check_binary_size<int32be>(4));
static_assert(mpt::check_binary_size<int16be>(2));
static_assert(mpt::check_binary_size<int8be>(1));
static_assert(mpt::check_binary_size<uint64be>(8));
static_assert(mpt::check_binary_size<uint32be>(4));
static_assert(mpt::check_binary_size<uint16be>(2));
static_assert(mpt::check_binary_size<uint8be>(1));



template <typename T>
struct make_le {
	using type = packed<typename std::remove_const<T>::type, LittleEndian_tag>;
};

template <typename T>
struct make_be {
	using type = packed<typename std::remove_const<T>::type, BigEndian_tag>;
};

template <mpt::endian endian, typename T>
struct make_endian {
};

template <typename T>
struct make_endian<mpt::endian::little, T> {
	using type = packed<typename std::remove_const<T>::type, LittleEndian_tag>;
};

template <typename T>
struct make_endian<mpt::endian::big, T> {
	using type = packed<typename std::remove_const<T>::type, BigEndian_tag>;
};

template <typename T>
MPT_CONSTEXPR20_FUN auto as_le(T v) noexcept -> typename mpt::make_le<typename std::remove_const<T>::type>::type {
	typename mpt::make_le<typename std::remove_const<T>::type>::type res{};
	res = v;
	return res;
}

template <typename T>
MPT_CONSTEXPR20_FUN auto as_be(T v) noexcept -> typename mpt::make_be<typename std::remove_const<T>::type>::type {
	typename mpt::make_be<typename std::remove_const<T>::type>::type res{};
	res = v;
	return res;
}

template <typename Tpacked>
MPT_CONSTEXPR20_FUN Tpacked as_endian(typename Tpacked::base_type v) noexcept {
	Tpacked res{};
	res = v;
	return res;
}



} // namespace MPT_INLINE_NS
} // namespace mpt



namespace std {
template <typename T, typename Tendian>
class numeric_limits<mpt::packed<T, Tendian>> : public std::numeric_limits<T> { };
template <typename T, typename Tendian>
class numeric_limits<const mpt::packed<T, Tendian>> : public std::numeric_limits<const T> { };
} // namespace std



#endif // MPT_ENDIAN_INTEGER_HPP
