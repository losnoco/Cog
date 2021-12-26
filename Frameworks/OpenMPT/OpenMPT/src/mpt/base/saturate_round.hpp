/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_SATURATE_ROUND_HPP
#define MPT_BASE_SATURATE_ROUND_HPP



#include "mpt/base/namespace.hpp"

#include "mpt/base/math.hpp"
#include "mpt/base/saturate_cast.hpp"

#include <type_traits>



namespace mpt {
inline namespace MPT_INLINE_NS {


// Rounds given double value to nearest integer value of type T.
// Out-of-range values are saturated to the specified integer type's limits.

template <typename T>
inline T saturate_round(float val) {
	static_assert(std::numeric_limits<T>::is_integer);
	return mpt::saturate_cast<T>(mpt::round(val));
}

template <typename T>
inline T saturate_round(double val) {
	static_assert(std::numeric_limits<T>::is_integer);
	return mpt::saturate_cast<T>(mpt::round(val));
}

template <typename T>
inline T saturate_round(long double val) {
	static_assert(std::numeric_limits<T>::is_integer);
	return mpt::saturate_cast<T>(mpt::round(val));
}


} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_BASE_SATURATE_ROUND_HPP
