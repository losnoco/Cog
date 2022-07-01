/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_POINTER_HPP
#define MPT_BASE_POINTER_HPP



#include "mpt/base/namespace.hpp"

#include <cstddef>



namespace mpt {
inline namespace MPT_INLINE_NS {


inline constexpr int arch_bits = sizeof(void *) * 8;
inline constexpr std::size_t pointer_size = sizeof(void *);


template <typename Tdst, typename Tsrc>
struct pointer_cast_helper {
	static constexpr Tdst cast(const Tsrc & src) noexcept {
		return src;
	}
};

template <typename Tdst, typename Tptr>
struct pointer_cast_helper<Tdst, const Tptr *> {
	static constexpr Tdst cast(const Tptr * const & src) noexcept {
		return reinterpret_cast<const Tdst>(src);
	}
};
template <typename Tdst, typename Tptr>
struct pointer_cast_helper<Tdst, Tptr *> {
	static constexpr Tdst cast(const Tptr * const & src) noexcept {
		return reinterpret_cast<const Tdst>(src);
	}
};


template <typename Tdst, typename Tsrc>
constexpr Tdst pointer_cast(const Tsrc & src) noexcept {
	return pointer_cast_helper<Tdst, Tsrc>::cast(src);
}


} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_BASE_POINTER_HPP
