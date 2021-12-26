/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_RANDOM_ENGINE_HPP
#define MPT_RANDOM_ENGINE_HPP



#include "mpt/base/macros.hpp"
#include "mpt/base/namespace.hpp"
#include "mpt/random/seed.hpp"

#include <memory>
#include <random>

#include <cstddef>


namespace mpt {
inline namespace MPT_INLINE_NS {


template <typename Trng>
struct engine_traits {
	typedef typename Trng::result_type result_type;
	static MPT_CONSTEXPRINLINE int result_bits() {
		return Trng::result_bits();
	}
	template <typename Trd>
	static inline Trng make(Trd & rd) {
		return Trng(rd);
	}
};

// C++11 random does not provide any sane way to determine the amount of entropy
// required to seed a particular engine. VERY STUPID.
// List the ones we are likely to use.

template <>
struct engine_traits<std::mt19937> {
	enum : std::size_t
	{
		seed_bits = sizeof(std::mt19937::result_type) * 8 * std::mt19937::state_size
	};
	typedef std::mt19937 rng_type;
	typedef rng_type::result_type result_type;
	static MPT_CONSTEXPRINLINE int result_bits() {
		return rng_type::word_size;
	}
	template <typename Trd>
	static inline rng_type make(Trd & rd) {
		std::unique_ptr<mpt::seed_seq_values<seed_bits / sizeof(unsigned int)>> values = std::make_unique<mpt::seed_seq_values<seed_bits / sizeof(unsigned int)>>(rd);
		std::seed_seq seed(values->begin(), values->end());
		return rng_type(seed);
	}
};

template <>
struct engine_traits<std::mt19937_64> {
	enum : std::size_t
	{
		seed_bits = sizeof(std::mt19937_64::result_type) * 8 * std::mt19937_64::state_size
	};
	typedef std::mt19937_64 rng_type;
	typedef rng_type::result_type result_type;
	static MPT_CONSTEXPRINLINE int result_bits() {
		return rng_type::word_size;
	}
	template <typename Trd>
	static inline rng_type make(Trd & rd) {
		std::unique_ptr<mpt::seed_seq_values<seed_bits / sizeof(unsigned int)>> values = std::make_unique<mpt::seed_seq_values<seed_bits / sizeof(unsigned int)>>(rd);
		std::seed_seq seed(values->begin(), values->end());
		return rng_type(seed);
	}
};

template <>
struct engine_traits<std::ranlux24_base> {
	enum : std::size_t
	{
		seed_bits = std::ranlux24_base::word_size
	};
	typedef std::ranlux24_base rng_type;
	typedef rng_type::result_type result_type;
	static MPT_CONSTEXPRINLINE int result_bits() {
		return rng_type::word_size;
	}
	template <typename Trd>
	static inline rng_type make(Trd & rd) {
		mpt::seed_seq_values<seed_bits / sizeof(unsigned int)> values(rd);
		std::seed_seq seed(values.begin(), values.end());
		return rng_type(seed);
	}
};

template <>
struct engine_traits<std::ranlux48_base> {
	enum : std::size_t
	{
		seed_bits = std::ranlux48_base::word_size
	};
	typedef std::ranlux48_base rng_type;
	typedef rng_type::result_type result_type;
	static MPT_CONSTEXPRINLINE int result_bits() {
		return rng_type::word_size;
	}
	template <typename Trd>
	static inline rng_type make(Trd & rd) {
		mpt::seed_seq_values<seed_bits / sizeof(unsigned int)> values(rd);
		std::seed_seq seed(values.begin(), values.end());
		return rng_type(seed);
	}
};

template <>
struct engine_traits<std::ranlux24> {
	enum : std::size_t
	{
		seed_bits = std::ranlux24_base::word_size
	};
	typedef std::ranlux24 rng_type;
	typedef rng_type::result_type result_type;
	static MPT_CONSTEXPRINLINE int result_bits() {
		return std::ranlux24_base::word_size;
	}
	template <typename Trd>
	static inline rng_type make(Trd & rd) {
		mpt::seed_seq_values<seed_bits / sizeof(unsigned int)> values(rd);
		std::seed_seq seed(values.begin(), values.end());
		return rng_type(seed);
	}
};

template <>
struct engine_traits<std::ranlux48> {
	enum : std::size_t
	{
		seed_bits = std::ranlux48_base::word_size
	};
	typedef std::ranlux48 rng_type;
	typedef rng_type::result_type result_type;
	static MPT_CONSTEXPRINLINE int result_bits() {
		return std::ranlux48_base::word_size;
	}
	template <typename Trd>
	static inline rng_type make(Trd & rd) {
		mpt::seed_seq_values<seed_bits / sizeof(unsigned int)> values(rd);
		std::seed_seq seed(values.begin(), values.end());
		return rng_type(seed);
	}
};



template <typename Trng, typename Trd>
inline Trng make_prng(Trd & rd) {
	return mpt::engine_traits<Trng>::make(rd);
}



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_RANDOM_ENGINE_HPP
