/*
 * Dither.h
 * --------
 * Purpose: Dithering when converting to lower resolution sample formats.
 * Notes  : (currently none)
 * Authors: Olivier Lapicque
 *          OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"


#include "SampleTypes.h"
#include "SampleFormatConverters.h"
#include "../common/mptRandom.h"


OPENMPT_NAMESPACE_BEGIN


enum DitherMode
{
	DitherNone       = 0,
	DitherDefault    = 1, // chosen by OpenMPT code, might change
	DitherModPlug    = 2, // rectangular, 0.5 bit depth, no noise shaping (original ModPlug Tracker)
	DitherSimple     = 3, // rectangular, 1 bit depth, simple 1st order noise shaping
	NumDitherModes
};


struct Dither_None
{
public:
	template <uint32 targetbits, typename Trng>
	MPT_FORCEINLINE MixSampleInt process(MixSampleInt sample, Trng &)
	{
		return sample;
	}
	template <uint32 targetbits, typename Trng>
	MPT_FORCEINLINE MixSampleFloat process(MixSampleFloat sample, Trng &)
	{
		return sample;
	}
};


struct Dither_ModPlug
{
public:
	template <uint32 targetbits, typename Trng>
	MPT_FORCEINLINE MixSampleInt process(MixSampleInt sample, Trng &rng)
	{
		if constexpr(targetbits == 0)
		{
			MPT_UNREFERENCED_PARAMETER(rng);
			return sample;
		} else if constexpr(targetbits + MixSampleIntTraits::mix_headroom_bits() + 1 >= 32)
		{
			MPT_UNREFERENCED_PARAMETER(rng);
			return sample;
		} else
		{
			sample += mpt::rshift_signed(static_cast<int32>(mpt::random<uint32>(rng)), (targetbits + MixSampleIntTraits::mix_headroom_bits() + 1));
			return sample;
		}
	}
	template <uint32 targetbits, typename Trng>
	MPT_FORCEINLINE MixSampleFloat process(MixSampleFloat sample, Trng &prng)
	{
		SC::ConvertToFixedPoint<MixSampleInt, MixSampleFloat, MixSampleIntTraits::mix_fractional_bits()> conv1;
		SC::ConvertFixedPoint<MixSampleFloat, MixSampleInt, MixSampleIntTraits::mix_fractional_bits()> conv2;
		return conv2(process<targetbits>(conv1(sample), prng));
	}
};


template<int ditherdepth = 1, bool triangular = false, bool shaped = true>
struct Dither_SimpleImpl
{
private:
	int32 error = 0;
public:
	template <uint32 targetbits, typename Trng>
	MPT_FORCEINLINE MixSampleInt process(MixSampleInt sample, Trng &prng)
	{
		if constexpr(targetbits == 0)
		{
			MPT_UNREFERENCED_PARAMETER(prng);
			return sample;
		} else
		{
			static_assert(sizeof(MixSampleInt) == 4);
			constexpr int rshift = (32-targetbits) - MixSampleIntTraits::mix_headroom_bits();
			if constexpr(rshift <= 1)
			{
				MPT_UNREFERENCED_PARAMETER(prng);
				// nothing to dither
				return sample;
			} else
			{
				constexpr int rshiftpositive = (rshift > 1) ? rshift : 1; // work-around warnings about negative shift with C++14 compilers
				constexpr int round_mask = ~((1<<rshiftpositive)-1);
				constexpr int round_offset = 1<<(rshiftpositive-1);
				constexpr int noise_bits = rshiftpositive + (ditherdepth - 1);
				constexpr int noise_bias = (1<<(noise_bits-1));
				int32 e = error;
				unsigned int unoise = 0;
				if constexpr(triangular)
				{
					unoise = (mpt::random<unsigned int>(prng, noise_bits) + mpt::random<unsigned int>(prng, noise_bits)) >> 1;
				} else
				{
					unoise = mpt::random<unsigned int>(prng, noise_bits);
				}
				int noise = static_cast<int>(unoise) - noise_bias; // un-bias
				int val = sample;
				if constexpr(shaped)
				{
					val += (e >> 1);
				}
				int rounded = (val + noise + round_offset) & round_mask;;
				e = val - rounded;
				sample = rounded;
				error = e;
				return sample;
			}
		}
	}
	template <uint32 targetbits, typename Trng>
	MPT_FORCEINLINE MixSampleFloat process(MixSampleFloat sample, Trng &prng)
	{
		SC::ConvertToFixedPoint<MixSampleInt, MixSampleFloat, MixSampleIntTraits::mix_fractional_bits()> conv1;
		SC::ConvertFixedPoint<MixSampleFloat, MixSampleInt, MixSampleIntTraits::mix_fractional_bits()> conv2;
		return conv2(process<targetbits>(conv1(sample), prng));
	}
};

using Dither_Simple = Dither_SimpleImpl<>;


template <typename Tdither, std::size_t channels>
class MultiChannelDither
{
private:
	std::array<Tdither, channels> DitherChannels;
public:
	void Reset()
	{
		DitherChannels.fill(Tdither());
	}
	template <uint32 targetbits, typename Trng>
	MPT_FORCEINLINE MixSampleInt process(std::size_t channel, MixSampleInt sample, Trng &prng)
	{
		return DitherChannels[channel].template process<targetbits>(sample, prng);
	}
	template <uint32 targetbits, typename Trng>
	MPT_FORCEINLINE MixSampleFloat process(std::size_t channel, MixSampleFloat sample, Trng &prng)
	{
		return DitherChannels[channel].template process<targetbits>(sample, prng);
	}
};


template <typename Tdither, std::size_t channels>
class DitherTemplate;

template <std::size_t channels>
class DitherTemplate<Dither_None, channels>
	: public MultiChannelDither<Dither_None, channels>
{
	struct {} prng;
public:
	template <typename Trd>
	DitherTemplate(Trd &)
	{
		return;
	}
	template <uint32 targetbits>
	MPT_FORCEINLINE MixSampleInt process(std::size_t channel, MixSampleInt sample)
	{
		return MultiChannelDither<Dither_None, channels>::template process<targetbits>(channel, sample, prng);
	}
	template <uint32 targetbits>
	MPT_FORCEINLINE MixSampleFloat process(std::size_t channel, MixSampleFloat sample)
	{
		return MultiChannelDither<Dither_None, channels>::template process<targetbits>(channel, sample, prng);
	}
};

template <std::size_t channels>
class DitherTemplate<Dither_ModPlug, channels>
	: public MultiChannelDither<Dither_ModPlug, channels>
{
private:
	mpt::rng::modplug_dither prng;
public:
	template <typename Trd>
	DitherTemplate(Trd &)
		: prng(0, 0)
	{
		return;
	}
	template <uint32 targetbits>
	MPT_FORCEINLINE MixSampleInt process(std::size_t channel, MixSampleInt sample)
	{
		return MultiChannelDither<Dither_ModPlug, channels>::template process<targetbits>(channel, sample, prng);
	}
	template <uint32 targetbits>
	MPT_FORCEINLINE MixSampleFloat process(std::size_t channel, MixSampleFloat sample)
	{
		return MultiChannelDither<Dither_ModPlug, channels>::template process<targetbits>(channel, sample, prng);
	}
};

template <std::size_t channels>
class DitherTemplate<Dither_Simple, channels>
	: public MultiChannelDither<Dither_Simple, channels>
{
private:
	mpt::fast_prng prng;
public:
	template <typename Trd>
	DitherTemplate(Trd & rd)
		: prng(rd)
	{
		return;
	}
	template <uint32 targetbits>
	MPT_FORCEINLINE MixSampleInt process(std::size_t channel, MixSampleInt sample)
	{
		return MultiChannelDither<Dither_Simple, channels>::template process<targetbits>(channel, sample, prng);
	}
	template <uint32 targetbits>
	MPT_FORCEINLINE MixSampleFloat process(std::size_t channel, MixSampleFloat sample)
	{
		return MultiChannelDither<Dither_Simple, channels>::template process<targetbits>(channel, sample, prng);
	}
};


class DitherNames
{
public:
	static mpt::ustring GetModeName(DitherMode mode);
};


template <std::size_t channels>
class DitherChannels
	: public DitherNames
{

private:

	DitherTemplate<Dither_None, channels> ditherNone;
	DitherTemplate<Dither_ModPlug, channels> ditherModPlug;
	DitherTemplate<Dither_Simple, channels> ditherSimple;

	DitherMode mode = DitherDefault;

public:

	template <typename Trd>
	DitherChannels(Trd & rd)
		: ditherNone(rd)
		, ditherModPlug(rd)
		, ditherSimple(rd)
	{
		return;
	}
	void Reset()
	{
		ditherModPlug.Reset();
		ditherSimple.Reset();
	}

	DitherTemplate<Dither_None, channels> & NoDither()
	{
		MPT_ASSERT(mode == DitherNone);
		return ditherNone;
	}
	DitherTemplate<Dither_ModPlug, channels> & DefaultDither()
	{
		MPT_ASSERT(mode == DitherDefault);
		return ditherModPlug;
	}
	DitherTemplate<Dither_ModPlug, channels> & ModPlugDither()
	{
		MPT_ASSERT(mode == DitherModPlug);
		return ditherModPlug;
	}
	DitherTemplate<Dither_Simple, channels> & SimpleDither()
	{
		MPT_ASSERT(mode == DitherSimple);
		return ditherSimple;
	}

	template <typename Tfn>
	auto WithDither(Tfn fn)
	{
		switch(GetMode())
		{
		case DitherNone:    return fn(NoDither());      break;
		case DitherModPlug: return fn(ModPlugDither()); break;
		case DitherSimple:  return fn(SimpleDither());  break;
		case DitherDefault: return fn(DefaultDither()); break;
		default:            return fn(DefaultDither()); break;
		}
	}

	void SetMode(DitherMode mode_)
	{
		mode = mode_;
	}
	DitherMode GetMode() const
	{
		return mode;
	}

};


using Dither = DitherChannels<4>;


OPENMPT_NAMESPACE_END
