/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_BASE_TESTS_RANDOM_HPP
#define MPT_BASE_TESTS_RANDOM_HPP



#include "mpt/base/algorithm.hpp"
#include "mpt/base/detect_compiler.hpp"
#include "mpt/base/integer.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/random/default_engines.hpp"
#include "mpt/random/device.hpp"
#include "mpt/random/random.hpp"
#include "mpt/test/test.hpp"
#include "mpt/test/test_macros.hpp"

#include <cstddef>



namespace mpt {
inline namespace MPT_INLINE_NS {



namespace tests {
namespace random {

#if MPT_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
#endif
MPT_TEST_GROUP_INLINE("mpt/random")
#if MPT_COMPILER_CLANG
#pragma clang diagnostic pop
#endif
{
	mpt::sane_random_device rd;
	mpt::good_engine prng = mpt::make_prng<mpt::good_engine>(rd);

	bool failed = false;

	for (std::size_t i = 0; i < 10000; ++i) {

		failed = failed || !mpt::is_in_range(mpt::random<uint16, 7>(prng), 0u, 127u);
		failed = failed || !mpt::is_in_range(mpt::random<uint16, 8>(prng), 0u, 255u);
		failed = failed || !mpt::is_in_range(mpt::random<uint16, 9>(prng), 0u, 511u);
		failed = failed || !mpt::is_in_range(mpt::random<uint64, 1>(prng), 0u, 1u);
		failed = failed || !mpt::is_in_range(mpt::random<uint16>(prng, 7), 0u, 127u);
		failed = failed || !mpt::is_in_range(mpt::random<uint16>(prng, 8), 0u, 255u);
		failed = failed || !mpt::is_in_range(mpt::random<uint16>(prng, 9), 0u, 511u);
		failed = failed || !mpt::is_in_range(mpt::random<uint64>(prng, 1), 0u, 1u);

		failed = failed || !mpt::is_in_range(mpt::random<int16, 7>(prng), 0, 127);
		failed = failed || !mpt::is_in_range(mpt::random<int16, 8>(prng), 0, 255);
		failed = failed || !mpt::is_in_range(mpt::random<int16, 9>(prng), 0, 511);
		failed = failed || !mpt::is_in_range(mpt::random<int64, 1>(prng), 0, 1);
		failed = failed || !mpt::is_in_range(mpt::random<int16>(prng, 7), 0, 127);
		failed = failed || !mpt::is_in_range(mpt::random<int16>(prng, 8), 0, 255);
		failed = failed || !mpt::is_in_range(mpt::random<int16>(prng, 9), 0, 511);
		failed = failed || !mpt::is_in_range(mpt::random<int64>(prng, 1), 0, 1);

		failed = failed || !mpt::is_in_range(mpt::random<float>(prng, 0.0f, 1.0f), 0.0f, 1.0f);
		failed = failed || !mpt::is_in_range(mpt::random<double>(prng, 0.0, 1.0), 0.0, 1.0);
		failed = failed || !mpt::is_in_range(mpt::random<double>(prng, -1.0, 1.0), -1.0, 1.0);
		failed = failed || !mpt::is_in_range(mpt::random<double>(prng, -1.0, 0.0), -1.0, 0.0);
		failed = failed || !mpt::is_in_range(mpt::random<double>(prng, 1.0, 2.0), 1.0, 2.0);
		failed = failed || !mpt::is_in_range(mpt::random<double>(prng, 1.0, 3.0), 1.0, 3.0);
	}

	MPT_TEST_EXPECT_EXPR(!failed);
}

} // namespace random
} // namespace tests



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_BASE_TESTS_RANDOM_HPP
