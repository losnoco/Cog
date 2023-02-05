/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_ALGORITHM_HPP
#define MPT_BASE_ALGORITHM_HPP



#include "mpt/base/detect_compiler.hpp"
#include "mpt/base/macros.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/base/saturate_cast.hpp"

#include <algorithm>
#include <limits>



namespace mpt {
inline namespace MPT_INLINE_NS {


// Grows x with an exponential factor suitable for increasing buffer sizes.
// Clamps the result at limit.
// And avoids integer overflows while doing its business.
// The growth factor is 1.5, rounding down, execpt for the initial x==1 case.
template <typename T, typename Tlimit>
inline T exponential_grow(const T & x, const Tlimit & limit) {
	if (x <= 1) {
		return 2;
	}
	T add = std::min(x >> 1, std::numeric_limits<T>::max() - x);
	return std::min(x + add, mpt::saturate_cast<T>(limit));
}

template <typename T>
inline T exponential_grow(const T & x) {
	return mpt::exponential_grow(x, std::numeric_limits<T>::max());
}


// Check if val is in [lo,hi] without causing compiler warnings
// if theses checks are always true due to the domain of T.
// GCC does not warn if the type is templated.
template <typename T, typename C>
constexpr bool is_in_range(const T & val, const C & lo, const C & hi) {
	return lo <= val && val <= hi;
}


template <typename Tcontainer, typename Tval>
MPT_CONSTEXPR20_FUN bool contains(const Tcontainer & container, const Tval & value) noexcept(noexcept(std::find(std::begin(container), std::end(container), value))) {
	return std::find(std::begin(container), std::end(container), value) != std::end(container);
}


} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_BASE_ALGORITHM_HPP
