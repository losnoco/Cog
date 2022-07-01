/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_TESTS_MATH_HPP
#define MPT_BASE_TESTS_MATH_HPP



#include "mpt/base/detect_compiler.hpp"
#include "mpt/base/math.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/test/test.hpp"
#include "mpt/test/test_macros.hpp"



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace tests {
namespace base {
namespace math {

#if MPT_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#endif
MPT_TEST_GROUP_INLINE("mpt/base/math")
#if MPT_COMPILER_CLANG
#pragma clang diagnostic pop
#endif
{
	MPT_TEST_EXPECT_EQUAL(mpt::round(1.99), 2.0);
	MPT_TEST_EXPECT_EQUAL(mpt::round(1.5), 2.0);
	MPT_TEST_EXPECT_EQUAL(mpt::round(1.1), 1.0);
	MPT_TEST_EXPECT_EQUAL(mpt::round(-0.1), 0.0);
	MPT_TEST_EXPECT_EQUAL(mpt::round(-0.5), -1.0);
	MPT_TEST_EXPECT_EQUAL(mpt::round(-0.9), -1.0);
	MPT_TEST_EXPECT_EQUAL(mpt::round(-1.4), -1.0);
	MPT_TEST_EXPECT_EQUAL(mpt::round(-1.7), -2.0);
}

} // namespace math
} // namespace base
} // namespace tests



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_BASE_TESTS_MATH_HPP
