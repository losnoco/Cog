/*
 * SampleTypes.h
 * -------------
 * Purpose: Basic audio sample types.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "BuildSettings.h"

#include "../common/mptBaseMacros.h"
#include "../common/mptBaseTypes.h"

#include <limits>
#include <type_traits>


OPENMPT_NAMESPACE_BEGIN


using AudioSampleInt = int16;
using AudioSampleFloat = nativefloat;

using AudioSample = mpt::select_type<mpt::float_traits<nativefloat>::is_hard, AudioSampleFloat, AudioSampleInt>::type;


template <typename Tsample, std::size_t MIX_HEADROOM_BITS, std::size_t FILTER_HEADROOM_BITS>
struct FixedPointSampleTraits
{
	static_assert(std::is_integral<Tsample>::value);
	static_assert(std::is_signed<Tsample>::value);
	static_assert((sizeof(Tsample) * 8u) - 1 > MIX_HEADROOM_BITS);
	static_assert((sizeof(Tsample) * 8u) - 1 > FILTER_HEADROOM_BITS);
	using sample_type = Tsample;
	enum class sample_type_strong : sample_type {};
	static constexpr int mix_headroom_bits() noexcept { return static_cast<int>(MIX_HEADROOM_BITS); }
	static constexpr int mix_precision_bits() noexcept { return static_cast<int>((sizeof(Tsample) * 8) - MIX_HEADROOM_BITS); }       // including sign bit
	static constexpr int mix_fractional_bits() noexcept { return static_cast<int>((sizeof(Tsample) * 8) - 1 - MIX_HEADROOM_BITS); }  // excluding sign bit
	static constexpr sample_type mix_clip_max() noexcept { return ((sample_type(1) << mix_fractional_bits()) - sample_type(1)); }
	static constexpr sample_type mix_clip_min() noexcept { return -((sample_type(1) << mix_fractional_bits()) - sample_type(1)); }
	static constexpr int filter_headroom_bits() noexcept { return static_cast<int>(FILTER_HEADROOM_BITS); }
	static constexpr int filter_precision_bits() noexcept { return static_cast<int>((sizeof(Tsample) * 8) - FILTER_HEADROOM_BITS); }       // including sign bit
	static constexpr int filter_fractional_bits() noexcept { return static_cast<int>((sizeof(Tsample) * 8) - 1 - FILTER_HEADROOM_BITS); }  // excluding sign bit
	template <typename Tfloat>
	static constexpr Tfloat mix_scale() noexcept { return static_cast<Tfloat>(sample_type(1) << mix_fractional_bits()); }
};

using MixSampleIntTraits = FixedPointSampleTraits<int32, 4, 8>;

using MixSampleInt = MixSampleIntTraits::sample_type;
using MixSampleFloat = AudioSampleFloat;

using MixSample = mpt::select_type<mpt::float_traits<nativefloat>::is_hard, MixSampleFloat, MixSampleInt>::type;


OPENMPT_NAMESPACE_END
