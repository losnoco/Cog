/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_UTILITY_HPP
#define MPT_BASE_UTILITY_HPP



#include "mpt/base/detect_compiler.hpp"
#include "mpt/base/namespace.hpp"

#if MPT_CXX_BEFORE(20)
#include "mpt/base/saturate_cast.hpp"
#endif

#include <type_traits>
#include <utility>

#include <cstddef>



namespace mpt {
inline namespace MPT_INLINE_NS {


#if MPT_CXX_AT_LEAST(20) && !MPT_CLANG_BEFORE(13, 0, 0)

using std::in_range;

#else

// Returns true iff Tdst can represent the value val.
// Use as if(mpt::in_range<uint8>(-1)).
template <typename Tdst, typename Tsrc>
constexpr bool in_range(Tsrc val) {
	return (static_cast<Tsrc>(mpt::saturate_cast<Tdst>(val)) == val);
}

#endif


#if MPT_CXX_AT_LEAST(23)

using std::to_underlying;

#else

template <typename T>
constexpr std::underlying_type_t<T> to_underlying(T value) noexcept {
	return static_cast<typename std::underlying_type<T>::type>(value);
}

#endif



template <typename T>
struct value_initializer {
	inline void operator()(T & x) {
		x = T{};
	}
};

template <typename T, std::size_t N>
struct value_initializer<T[N]> {
	inline void operator()(T (&a)[N]) {
		for (auto & e : a)
		{
			value_initializer<T>{}(e);
		}
	}
};

template <typename T>
inline void reset(T & x) {
	value_initializer<T>{}(x);
}



#if MPT_CXX_AT_LEAST(20) && !MPT_CLANG_BEFORE(13, 0, 0)

using std::cmp_equal;
using std::cmp_greater;
using std::cmp_greater_equal;
using std::cmp_less;
using std::cmp_less_equal;
using std::cmp_not_equal;

#else

template <typename Ta, typename Tb>
constexpr bool cmp_equal(Ta a, Tb b) noexcept {
	using UTa = typename std::make_unsigned<Ta>::type;
	using UTb = typename std::make_unsigned<Tb>::type;
	if constexpr (std::is_signed<Ta>::value == std::is_signed<Tb>::value) {
		return a == b;
	} else if constexpr (std::is_signed<Ta>::value) {
		return (a < 0) ? false : static_cast<UTa>(a) == b;
	} else {
		return (b < 0) ? false : a == static_cast<UTb>(b);
	}
}

template <typename Ta, typename Tb>
constexpr bool cmp_not_equal(Ta a, Tb b) noexcept {
	using UTa = typename std::make_unsigned<Ta>::type;
	using UTb = typename std::make_unsigned<Tb>::type;
	if constexpr (std::is_signed<Ta>::value == std::is_signed<Tb>::value) {
		return a != b;
	} else if constexpr (std::is_signed<Ta>::value) {
		return (a < 0) ? true : static_cast<UTa>(a) != b;
	} else {
		return (b < 0) ? true : a != static_cast<UTb>(b);
	}
}

template <typename Ta, typename Tb>
constexpr bool cmp_less(Ta a, Tb b) noexcept {
	using UTa = typename std::make_unsigned<Ta>::type;
	using UTb = typename std::make_unsigned<Tb>::type;
	if constexpr (std::is_signed<Ta>::value == std::is_signed<Tb>::value) {
		return a < b;
	} else if constexpr (std::is_signed<Ta>::value) {
		return (a < 0) ? true : static_cast<UTa>(a) < b;
	} else {
		return (b < 0) ? false : a < static_cast<UTb>(b);
	}
}

template <typename Ta, typename Tb>
constexpr bool cmp_greater(Ta a, Tb b) noexcept {
	using UTa = typename std::make_unsigned<Ta>::type;
	using UTb = typename std::make_unsigned<Tb>::type;
	if constexpr (std::is_signed<Ta>::value == std::is_signed<Tb>::value) {
		return a > b;
	} else if constexpr (std::is_signed<Ta>::value) {
		return (a < 0) ? false : static_cast<UTa>(a) > b;
	} else {
		return (b < 0) ? true : a > static_cast<UTb>(b);
	}
}

template <typename Ta, typename Tb>
constexpr bool cmp_less_equal(Ta a, Tb b) noexcept {
	using UTa = typename std::make_unsigned<Ta>::type;
	using UTb = typename std::make_unsigned<Tb>::type;
	if constexpr (std::is_signed<Ta>::value == std::is_signed<Tb>::value) {
		return a <= b;
	} else if constexpr (std::is_signed<Ta>::value) {
		return (a < 0) ? true : static_cast<UTa>(a) <= b;
	} else {
		return (b < 0) ? false : a <= static_cast<UTb>(b);
	}
}

template <typename Ta, typename Tb>
constexpr bool cmp_greater_equal(Ta a, Tb b) noexcept {
	using UTa = typename std::make_unsigned<Ta>::type;
	using UTb = typename std::make_unsigned<Tb>::type;
	if constexpr (std::is_signed<Ta>::value == std::is_signed<Tb>::value) {
		return a >= b;
	} else if constexpr (std::is_signed<Ta>::value) {
		return (a < 0) ? false : static_cast<UTa>(a) >= b;
	} else {
		return (b < 0) ? true : a >= static_cast<UTb>(b);
	}
}

#endif



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_BASE_UTILITY_HPP
