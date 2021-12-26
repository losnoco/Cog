/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_CHECK_PLATFORM_HPP
#define MPT_BASE_CHECK_PLATFORM_HPP



#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/base/pointer.hpp"

#include <limits>

#include <cstddef>


namespace mpt {
inline namespace MPT_INLINE_NS {


static_assert(sizeof(std::uintptr_t) == sizeof(void *));
static_assert(std::numeric_limits<unsigned char>::digits == 8);

static_assert(sizeof(char) == 1);

static_assert(sizeof(std::byte) == 1);
static_assert(alignof(std::byte) == 1);

static_assert(mpt::arch_bits == static_cast<int>(mpt::pointer_size) * 8);


} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_BASE_CHECK_PLATFORM_HPP
