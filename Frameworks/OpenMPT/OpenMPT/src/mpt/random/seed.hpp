/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_RANDOM_SEED_HPP
#define MPT_RANDOM_SEED_HPP



#include "mpt/base/namespace.hpp"

#include <array>
#include <random>

#include <cstddef>


namespace mpt {
inline namespace MPT_INLINE_NS {



template <std::size_t N>
class seed_seq_values {
private:
	std::array<unsigned int, N> seeds;

public:
	template <typename Trd>
	explicit seed_seq_values(Trd & rd) {
		std::uniform_int_distribution<unsigned int> random_int{};
		for (std::size_t i = 0; i < N; ++i) {
			seeds[i] = random_int(rd);
		}
	}
	const unsigned int * begin() const {
		return seeds.data();
	}
	const unsigned int * end() const {
		return seeds.data() + N;
	}
};



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_RANDOM_SEED_HPP
