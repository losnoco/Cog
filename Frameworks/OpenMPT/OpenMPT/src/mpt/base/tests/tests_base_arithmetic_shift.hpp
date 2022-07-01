/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_TESTS_ARITHMETIC_SHIFT_HPP
#define MPT_BASE_TESTS_ARITHMETIC_SHIFT_HPP



#include "mpt/base/arithmetic_shift.hpp"
#include "mpt/base/detect.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/test/test.hpp"
#include "mpt/test/test_macros.hpp"

#include <limits>



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace tests {
namespace base {
namespace arithmetic_shift {

#if MPT_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#endif
MPT_TEST_GROUP_INLINE("mpt/base/arithmetic_shift")
#if MPT_COMPILER_CLANG
#pragma clang diagnostic pop
#endif
{
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-32768, 1), mpt::rshift_signed_standard<int16>(-32768, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-32767, 1), mpt::rshift_signed_standard<int16>(-32767, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-32766, 1), mpt::rshift_signed_standard<int16>(-32766, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-2, 1), mpt::rshift_signed_standard<int16>(-2, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-1, 1), mpt::rshift_signed_standard<int16>(-1, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(0, 1), mpt::rshift_signed_standard<int16>(0, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(1, 1), mpt::rshift_signed_standard<int16>(1, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(2, 1), mpt::rshift_signed_standard<int16>(2, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(32766, 1), mpt::rshift_signed_standard<int16>(32766, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(32767, 1), mpt::rshift_signed_standard<int16>(32767, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-32768, 14), mpt::rshift_signed_standard<int16>(-32768, 14));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-32767, 14), mpt::rshift_signed_standard<int16>(-32767, 14));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-32766, 14), mpt::rshift_signed_standard<int16>(-32766, 14));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-2, 14), mpt::rshift_signed_standard<int16>(-2, 14));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-1, 14), mpt::rshift_signed_standard<int16>(-1, 14));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(0, 14), mpt::rshift_signed_standard<int16>(0, 14));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(1, 14), mpt::rshift_signed_standard<int16>(1, 14));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(2, 14), mpt::rshift_signed_standard<int16>(2, 14));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(32766, 14), mpt::rshift_signed_standard<int16>(32766, 14));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(32767, 14), mpt::rshift_signed_standard<int16>(32767, 14));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-32768, 15), mpt::rshift_signed_standard<int16>(-32768, 15));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-32767, 15), mpt::rshift_signed_standard<int16>(-32767, 15));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-32766, 15), mpt::rshift_signed_standard<int16>(-32766, 15));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-2, 15), mpt::rshift_signed_standard<int16>(-2, 15));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-1, 15), mpt::rshift_signed_standard<int16>(-1, 15));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(0, 15), mpt::rshift_signed_standard<int16>(0, 15));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(1, 15), mpt::rshift_signed_standard<int16>(1, 15));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(2, 15), mpt::rshift_signed_standard<int16>(2, 15));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(32766, 15), mpt::rshift_signed_standard<int16>(32766, 15));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(32767, 15), mpt::rshift_signed_standard<int16>(32767, 15));

	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-32768, 1), mpt::lshift_signed_standard<int16>(-32768, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-32767, 1), mpt::lshift_signed_standard<int16>(-32767, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-32766, 1), mpt::lshift_signed_standard<int16>(-32766, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-2, 1), mpt::lshift_signed_standard<int16>(-2, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-1, 1), mpt::lshift_signed_standard<int16>(-1, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(0, 1), mpt::lshift_signed_standard<int16>(0, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(1, 1), mpt::lshift_signed_standard<int16>(1, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(2, 1), mpt::lshift_signed_standard<int16>(2, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(32766, 1), mpt::lshift_signed_standard<int16>(32766, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(32767, 1), mpt::lshift_signed_standard<int16>(32767, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-32768, 14), mpt::lshift_signed_standard<int16>(-32768, 14));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-32767, 14), mpt::lshift_signed_standard<int16>(-32767, 14));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-32766, 14), mpt::lshift_signed_standard<int16>(-32766, 14));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-2, 14), mpt::lshift_signed_standard<int16>(-2, 14));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-1, 14), mpt::lshift_signed_standard<int16>(-1, 14));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(0, 14), mpt::lshift_signed_standard<int16>(0, 14));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(1, 14), mpt::lshift_signed_standard<int16>(1, 14));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(2, 14), mpt::lshift_signed_standard<int16>(2, 14));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(32766, 14), mpt::lshift_signed_standard<int16>(32766, 14));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(32767, 14), mpt::lshift_signed_standard<int16>(32767, 14));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-32768, 15), mpt::lshift_signed_standard<int16>(-32768, 15));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-32767, 15), mpt::lshift_signed_standard<int16>(-32767, 15));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-32766, 15), mpt::lshift_signed_standard<int16>(-32766, 15));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-2, 15), mpt::lshift_signed_standard<int16>(-2, 15));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-1, 15), mpt::lshift_signed_standard<int16>(-1, 15));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(0, 15), mpt::lshift_signed_standard<int16>(0, 15));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(1, 15), mpt::lshift_signed_standard<int16>(1, 15));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(2, 15), mpt::lshift_signed_standard<int16>(2, 15));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(32766, 15), mpt::lshift_signed_standard<int16>(32766, 15));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(32767, 15), mpt::lshift_signed_standard<int16>(32767, 15));

#if MPT_COMPILER_SHIFT_SIGNED

	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-32768, 1), (-32768) >> 1);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-32767, 1), (-32767) >> 1);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-32766, 1), (-32766) >> 1);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-2, 1), (-2) >> 1);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-1, 1), (-1) >> 1);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(0, 1), (0) >> 1);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(1, 1), (1) >> 1);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(2, 1), (2) >> 1);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(32766, 1), (32766) >> 1);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(32767, 1), (32767) >> 1);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-32768, 14), (-32768) >> 14);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-32767, 14), (-32767) >> 14);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-32766, 14), (-32766) >> 14);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-2, 14), (-2) >> 14);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-1, 14), (-1) >> 14);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(0, 14), (0) >> 14);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(1, 14), (1) >> 14);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(2, 14), (2) >> 14);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(32766, 14), (32766) >> 14);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(32767, 14), (32767) >> 14);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-32768, 15), (-32768) >> 15);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-32767, 15), (-32767) >> 15);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-32766, 15), (-32766) >> 15);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-2, 15), (-2) >> 15);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(-1, 15), (-1) >> 15);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(0, 15), (0) >> 15);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(1, 15), (1) >> 15);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(2, 15), (2) >> 15);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(32766, 15), (32766) >> 15);
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int16>(32767, 15), (32767) >> 15);

	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-32768, 1), (-32768) << 1);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-32767, 1), (-32767) << 1);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-32766, 1), (-32766) << 1);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-2, 1), (-2) << 1);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-1, 1), (-1) << 1);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(0, 1), (0) << 1);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(1, 1), (1) << 1);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(2, 1), (2) << 1);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(32766, 1), (32766) << 1);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(32767, 1), (32767) << 1);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-32768, 14), (-32768) << 14);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-32767, 14), (-32767) << 14);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-32766, 14), (-32766) << 14);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-2, 14), (-2) << 14);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-1, 14), (-1) << 14);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(0, 14), (0) << 14);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(1, 14), (1) << 14);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(2, 14), (2) << 14);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(32766, 14), (32766) << 14);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(32767, 14), (32767) << 14);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-32768, 15), (-32768) << 15);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-32767, 15), (-32767) << 15);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-32766, 15), (-32766) << 15);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-2, 15), (-2) << 15);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(-1, 15), (-1) << 15);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(0, 15), (0) << 15);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(1, 15), (1) << 15);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(2, 15), (2) << 15);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(32766, 15), (32766) << 15);
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int16>(32767, 15), (32767) << 15);

#endif

	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(0 - 0x80000000, 1), mpt::rshift_signed_standard<int32>(0 - 0x80000000, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(-0x7fffffff, 1), mpt::rshift_signed_standard<int32>(-0x7fffffff, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(-0x7ffffffe, 1), mpt::rshift_signed_standard<int32>(-0x7ffffffe, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(-1, 1), mpt::rshift_signed_standard<int32>(-1, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(0, 1), mpt::rshift_signed_standard<int32>(0, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(1, 1), mpt::rshift_signed_standard<int32>(1, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(0x7ffffffe, 1), mpt::rshift_signed_standard<int32>(0x7ffffffe, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(0x7fffffff, 1), mpt::rshift_signed_standard<int32>(0x7fffffff, 1));

	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(0 - 0x80000000, 31), mpt::rshift_signed_standard<int32>(0 - 0x80000000, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(-0x7fffffff, 31), mpt::rshift_signed_standard<int32>(-0x7fffffff, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(-0x7ffffffe, 31), mpt::rshift_signed_standard<int32>(-0x7ffffffe, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(-1, 31), mpt::rshift_signed_standard<int32>(-1, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(0, 31), mpt::rshift_signed_standard<int32>(0, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(1, 31), mpt::rshift_signed_standard<int32>(1, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(0x7ffffffe, 31), mpt::rshift_signed_standard<int32>(0x7ffffffe, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(0x7fffffff, 31), mpt::rshift_signed_standard<int32>(0x7fffffff, 31));

	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(0 - 0x80000000, 1), mpt::lshift_signed_standard<int32>(0 - 0x80000000, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(-0x7fffffff, 1), mpt::lshift_signed_standard<int32>(-0x7fffffff, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(-0x7ffffffe, 1), mpt::lshift_signed_standard<int32>(-0x7ffffffe, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(-1, 1), mpt::lshift_signed_standard<int32>(-1, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(0, 1), mpt::lshift_signed_standard<int32>(0, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(1, 1), mpt::lshift_signed_standard<int32>(1, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(0x7ffffffe, 1), mpt::lshift_signed_standard<int32>(0x7ffffffe, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(0x7fffffff, 1), mpt::lshift_signed_standard<int32>(0x7fffffff, 1));

	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(0 - 0x80000000, 31), mpt::lshift_signed_standard<int32>(0 - 0x80000000, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(-0x7fffffff, 31), mpt::lshift_signed_standard<int32>(-0x7fffffff, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(-0x7ffffffe, 31), mpt::lshift_signed_standard<int32>(-0x7ffffffe, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(-1, 31), mpt::lshift_signed_standard<int32>(-1, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(0, 31), mpt::lshift_signed_standard<int32>(0, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(1, 31), mpt::lshift_signed_standard<int32>(1, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(0x7ffffffe, 31), mpt::lshift_signed_standard<int32>(0x7ffffffe, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(0x7fffffff, 31), mpt::lshift_signed_standard<int32>(0x7fffffff, 31));

#if MPT_COMPILER_SHIFT_SIGNED

	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(0 - 0x80000000, 1), mpt::rshift_signed_undefined<int32>(0 - 0x80000000, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(-0x7fffffff, 1), mpt::rshift_signed_undefined<int32>(-0x7fffffff, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(-0x7ffffffe, 1), mpt::rshift_signed_undefined<int32>(-0x7ffffffe, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(-1, 1), mpt::rshift_signed_undefined<int32>(-1, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(0, 1), mpt::rshift_signed_undefined<int32>(0, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(1, 1), mpt::rshift_signed_undefined<int32>(1, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(0x7ffffffe, 1), mpt::rshift_signed_undefined<int32>(0x7ffffffe, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(0x7fffffff, 1), mpt::rshift_signed_undefined<int32>(0x7fffffff, 1));

	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(0 - 0x80000000, 31), mpt::rshift_signed_undefined<int32>(0 - 0x80000000, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(-0x7fffffff, 31), mpt::rshift_signed_undefined<int32>(-0x7fffffff, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(-0x7ffffffe, 31), mpt::rshift_signed_undefined<int32>(-0x7ffffffe, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(-1, 31), mpt::rshift_signed_undefined<int32>(-1, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(0, 31), mpt::rshift_signed_undefined<int32>(0, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(1, 31), mpt::rshift_signed_undefined<int32>(1, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(0x7ffffffe, 31), mpt::rshift_signed_undefined<int32>(0x7ffffffe, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int32>(0x7fffffff, 31), mpt::rshift_signed_undefined<int32>(0x7fffffff, 31));

	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(0 - 0x80000000, 1), mpt::lshift_signed_undefined<int32>(0 - 0x80000000, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(-0x7fffffff, 1), mpt::lshift_signed_undefined<int32>(-0x7fffffff, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(-0x7ffffffe, 1), mpt::lshift_signed_undefined<int32>(-0x7ffffffe, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(-1, 1), mpt::lshift_signed_undefined<int32>(-1, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(0, 1), mpt::lshift_signed_undefined<int32>(0, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(1, 1), mpt::lshift_signed_undefined<int32>(1, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(0x7ffffffe, 1), mpt::lshift_signed_undefined<int32>(0x7ffffffe, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(0x7fffffff, 1), mpt::lshift_signed_undefined<int32>(0x7fffffff, 1));

	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(0 - 0x80000000, 31), mpt::lshift_signed_undefined<int32>(0 - 0x80000000, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(-0x7fffffff, 31), mpt::lshift_signed_undefined<int32>(-0x7fffffff, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(-0x7ffffffe, 31), mpt::lshift_signed_undefined<int32>(-0x7ffffffe, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(-1, 31), mpt::lshift_signed_undefined<int32>(-1, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(0, 31), mpt::lshift_signed_undefined<int32>(0, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(1, 31), mpt::lshift_signed_undefined<int32>(1, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(0x7ffffffe, 31), mpt::lshift_signed_undefined<int32>(0x7ffffffe, 31));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int32>(0x7fffffff, 31), mpt::lshift_signed_undefined<int32>(0x7fffffff, 31));

#endif

	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(0ull - 0x8000000000000000ull, 1), mpt::rshift_signed_standard<int64>(0ull - 0x8000000000000000ull, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(-0x7fffffffffffffffll, 1), mpt::rshift_signed_standard<int64>(-0x7fffffffffffffffll, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(-0x7ffffffffffffffell, 1), mpt::rshift_signed_standard<int64>(-0x7ffffffffffffffell, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(-1ll, 1), mpt::rshift_signed_standard<int64>(-1ll, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(0ll, 1), mpt::rshift_signed_standard<int64>(0ll, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(1ll, 1), mpt::rshift_signed_standard<int64>(1ll, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(0x7ffffffffffffffell, 1), mpt::rshift_signed_standard<int64>(0x7ffffffffffffffell, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(0x7fffffffffffffffll, 1), mpt::rshift_signed_standard<int64>(0x7fffffffffffffffll, 1));

	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(0ull - 0x8000000000000000ull, 63), mpt::rshift_signed_standard<int64>(0ull - 0x8000000000000000ull, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(-0x7fffffffffffffffll, 63), mpt::rshift_signed_standard<int64>(-0x7fffffffffffffffll, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(-0x7ffffffffffffffell, 63), mpt::rshift_signed_standard<int64>(-0x7ffffffffffffffell, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(-1ll, 63), mpt::rshift_signed_standard<int64>(-1ll, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(0ll, 63), mpt::rshift_signed_standard<int64>(0ll, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(1ll, 63), mpt::rshift_signed_standard<int64>(1ll, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(0x7ffffffffffffffell, 63), mpt::rshift_signed_standard<int64>(0x7ffffffffffffffell, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(0x7fffffffffffffffll, 63), mpt::rshift_signed_standard<int64>(0x7fffffffffffffffll, 63));

	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(0ull - 0x8000000000000000ull, 1), mpt::lshift_signed_standard<int64>(0ull - 0x8000000000000000ull, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(-0x7fffffffffffffffll, 1), mpt::lshift_signed_standard<int64>(-0x7fffffffffffffffll, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(-0x7ffffffffffffffell, 1), mpt::lshift_signed_standard<int64>(-0x7ffffffffffffffell, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(-1ll, 1), mpt::lshift_signed_standard<int64>(-1ll, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(0ll, 1), mpt::lshift_signed_standard<int64>(0ll, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(1ll, 1), mpt::lshift_signed_standard<int64>(1ll, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(0x7ffffffffffffffell, 1), mpt::lshift_signed_standard<int64>(0x7ffffffffffffffell, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(0x7fffffffffffffffll, 1), mpt::lshift_signed_standard<int64>(0x7fffffffffffffffll, 1));

	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(0ull - 0x8000000000000000ull, 63), mpt::lshift_signed_standard<int64>(0ull - 0x8000000000000000ull, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(-0x7fffffffffffffffll, 63), mpt::lshift_signed_standard<int64>(-0x7fffffffffffffffll, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(-0x7ffffffffffffffell, 63), mpt::lshift_signed_standard<int64>(-0x7ffffffffffffffell, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(-1ll, 63), mpt::lshift_signed_standard<int64>(-1ll, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(0ll, 63), mpt::lshift_signed_standard<int64>(0ll, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(1ll, 63), mpt::lshift_signed_standard<int64>(1ll, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(0x7ffffffffffffffell, 63), mpt::lshift_signed_standard<int64>(0x7ffffffffffffffell, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(0x7fffffffffffffffll, 63), mpt::lshift_signed_standard<int64>(0x7fffffffffffffffll, 63));

#if MPT_COMPILER_SHIFT_SIGNED

	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(0ull - 0x8000000000000000ull, 1), mpt::rshift_signed_undefined<int64>(0ull - 0x8000000000000000ull, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(-0x7fffffffffffffffll, 1), mpt::rshift_signed_undefined<int64>(-0x7fffffffffffffffll, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(-0x7ffffffffffffffell, 1), mpt::rshift_signed_undefined<int64>(-0x7ffffffffffffffell, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(-1ll, 1), mpt::rshift_signed_undefined<int64>(-1ll, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(0ll, 1), mpt::rshift_signed_undefined<int64>(0ll, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(1ll, 1), mpt::rshift_signed_undefined<int64>(1ll, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(0x7ffffffffffffffell, 1), mpt::rshift_signed_undefined<int64>(0x7ffffffffffffffell, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(0x7fffffffffffffffll, 1), mpt::rshift_signed_undefined<int64>(0x7fffffffffffffffll, 1));

	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(0ull - 0x8000000000000000ull, 63), mpt::rshift_signed_undefined<int64>(0ull - 0x8000000000000000ull, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(-0x7fffffffffffffffll, 63), mpt::rshift_signed_undefined<int64>(-0x7fffffffffffffffll, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(-0x7ffffffffffffffell, 63), mpt::rshift_signed_undefined<int64>(-0x7ffffffffffffffell, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(-1ll, 63), mpt::rshift_signed_undefined<int64>(-1ll, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(0ll, 63), mpt::rshift_signed_undefined<int64>(0ll, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(1ll, 63), mpt::rshift_signed_undefined<int64>(1ll, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(0x7ffffffffffffffell, 63), mpt::rshift_signed_undefined<int64>(0x7ffffffffffffffell, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::rshift_signed<int64>(0x7fffffffffffffffll, 63), mpt::rshift_signed_undefined<int64>(0x7fffffffffffffffll, 63));

	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(0ull - 0x8000000000000000ull, 1), mpt::lshift_signed_undefined<int64>(0ull - 0x8000000000000000ull, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(-0x7fffffffffffffffll, 1), mpt::lshift_signed_undefined<int64>(-0x7fffffffffffffffll, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(-0x7ffffffffffffffell, 1), mpt::lshift_signed_undefined<int64>(-0x7ffffffffffffffell, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(-1ll, 1), mpt::lshift_signed_undefined<int64>(-1ll, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(0ll, 1), mpt::lshift_signed_undefined<int64>(0ll, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(1ll, 1), mpt::lshift_signed_undefined<int64>(1ll, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(0x7ffffffffffffffell, 1), mpt::lshift_signed_undefined<int64>(0x7ffffffffffffffell, 1));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(0x7fffffffffffffffll, 1), mpt::lshift_signed_undefined<int64>(0x7fffffffffffffffll, 1));

	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(0ull - 0x8000000000000000ull, 63), mpt::lshift_signed_undefined<int64>(0ull - 0x8000000000000000ull, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(-0x7fffffffffffffffll, 63), mpt::lshift_signed_undefined<int64>(-0x7fffffffffffffffll, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(-0x7ffffffffffffffell, 63), mpt::lshift_signed_undefined<int64>(-0x7ffffffffffffffell, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(-1ll, 63), mpt::lshift_signed_undefined<int64>(-1ll, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(0ll, 63), mpt::lshift_signed_undefined<int64>(0ll, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(1ll, 63), mpt::lshift_signed_undefined<int64>(1ll, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(0x7ffffffffffffffell, 63), mpt::lshift_signed_undefined<int64>(0x7ffffffffffffffell, 63));
	MPT_TEST_EXPECT_EQUAL(mpt::lshift_signed<int64>(0x7fffffffffffffffll, 63), mpt::lshift_signed_undefined<int64>(0x7fffffffffffffffll, 63));

#endif
}

} // namespace arithmetic_shift
} // namespace base
} // namespace tests



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_BASE_TESTS_ARITHMETIC_SHIFT_HPP
