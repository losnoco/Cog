/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_FLOATINGPOINT_HPP
#define MPT_BASE_FLOATINGPOINT_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"

#include <limits>
#include <type_traits>



namespace mpt {
inline namespace MPT_INLINE_NS {


// fp half
// n/a

// fp single
using single = float;
namespace float_literals {
constexpr single operator"" _fs(long double lit) noexcept {
	return static_cast<single>(lit);
}
} // namespace float_literals

// fp double
namespace float_literals {
constexpr double operator"" _fd(long double lit) noexcept {
	return static_cast<double>(lit);
}
} // namespace float_literals

// fp extended
namespace float_literals {
constexpr long double operator"" _fe(long double lit) noexcept {
	return static_cast<long double>(lit);
}
} // namespace float_literals

// fp quad
// n/a

using float32 = std::conditional<sizeof(float) == 4, float, std::conditional<sizeof(double) == 4, double, std::conditional<sizeof(long double) == 4, long double, float>::type>::type>::type;
namespace float_literals {
constexpr float32 operator"" _f32(long double lit) noexcept {
	return static_cast<float32>(lit);
}
} // namespace float_literals

using float64 = std::conditional<sizeof(float) == 8, float, std::conditional<sizeof(double) == 8, double, std::conditional<sizeof(long double) == 8, long double, double>::type>::type>::type;
namespace float_literals {
constexpr float64 operator"" _f64(long double lit) noexcept {
	return static_cast<float64>(lit);
}
} // namespace float_literals

template <typename T>
struct float_traits {
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
	static constexpr bool is_preferred = is_float && ((is_float32 && MPT_COMPILER_QUIRK_FLOAT_PREFER32) || (is_float64 && MPT_COMPILER_QUIRK_FLOAT_PREFER64));
};

// prefer smaller floats, but try to use IEEE754 floats
using nativefloat =
	std::conditional<mpt::float_traits<float32>::is_preferred, float32, std::conditional<mpt::float_traits<float64>::is_preferred, float64, std::conditional<std::numeric_limits<float>::is_iec559, float, std::conditional<std::numeric_limits<double>::is_iec559, double, std::conditional<std::numeric_limits<long double>::is_iec559, long double, float>::type>::type>::type>::type>::type;
namespace float_literals {
constexpr nativefloat operator"" _nf(long double lit) noexcept {
	return static_cast<nativefloat>(lit);
}
} // namespace float_literals


} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_BASE_FLOATINGPOINT_HPP
