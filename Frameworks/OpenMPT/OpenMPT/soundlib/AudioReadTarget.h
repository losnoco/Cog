/*
 * AudioReadTarget.h
 * -----------------
 * Purpose: Callback class implementations for audio data read via CSoundFile::Read.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "BuildSettings.h"

#include "Sndfile.h"
#include "../soundbase/SampleFormat.h"
#include "../soundbase/SampleFormatConverters.h"
#include "../soundbase/SampleFormatCopy.h"
#include "../soundbase/SampleBuffer.h"
#include "../soundbase/Dither.h"
#include "MixerLoops.h"
#include "Mixer.h"


OPENMPT_NAMESPACE_BEGIN


template<typename Tbuffer>
class AudioReadTargetBuffer
	: public IAudioReadTarget
{
private:
	std::size_t countRendered;
	Dither &dither;
protected:
	Tbuffer outputBuffer;
public:
	AudioReadTargetBuffer(Tbuffer buf, Dither &dither_)
		: countRendered(0)
		, dither(dither_)
		, outputBuffer(buf)
	{
		MPT_ASSERT(SampleFormat(SampleFormatTraits<typename Tbuffer::sample_type>::sampleFormat()).IsValid());
	}
	std::size_t GetRenderedCount() const { return countRendered; }
public:
	void DataCallback(MixSampleInt *MixSoundBuffer, std::size_t channels, std::size_t countChunk) override
	{
		dither.WithDither(
			[&](auto &ditherInstance)
			{
				ConvertBufferMixFixedToBuffer<MixSampleIntTraits::mix_fractional_bits(), false>(advance_audio_buffer(outputBuffer, countRendered), audio_buffer_interleaved<MixSampleInt>(MixSoundBuffer, channels, countChunk), ditherInstance, channels, countChunk);
			}
		);
		countRendered += countChunk;
	}
	void DataCallback(MixSampleFloat *MixSoundBuffer, std::size_t channels, std::size_t countChunk) override
	{
		dither.WithDither(
			[&](auto &ditherInstance)
			{
				ConvertBufferMixFloatToBuffer<false>(advance_audio_buffer(outputBuffer, countRendered), audio_buffer_interleaved<MixSampleFloat>(MixSoundBuffer, channels, countChunk), ditherInstance, channels, countChunk);
			}
		);
		countRendered += countChunk;
	}
};


#if defined(LIBOPENMPT_BUILD)


template<typename Tsample>
void ApplyGainBeforeConversionIfAppropriateFixed(MixSampleInt *MixSoundBuffer, std::size_t channels, std::size_t countChunk, float gainFactor)
{
	// Apply final output gain for non floating point output
	ApplyGain(MixSoundBuffer, channels, countChunk, mpt::saturate_round<int32>(gainFactor * (1<<16)));
}
template<>
void ApplyGainBeforeConversionIfAppropriateFixed<float>(MixSampleInt * /*MixSoundBuffer*/, std::size_t /*channels*/, std::size_t /*countChunk*/, float /*gainFactor*/)
{
	// nothing
}

template<typename Tsample>
void ApplyGainAfterConversionIfAppropriateFixed(audio_buffer_interleaved<Tsample> /*buffer*/, std::size_t /*countRendered*/, std::size_t /*channels*/, std::size_t /*countChunk*/, float /*gainFactor*/)
{
	// nothing
}
template<typename Tsample>
void ApplyGainAfterConversionIfAppropriateFixed(audio_buffer_planar<Tsample> /*buffer*/, std::size_t /*countRendered*/, std::size_t /*channels*/, std::size_t /*countChunk*/, float /*gainFactor*/)
{
	// nothing
}
template<>
void ApplyGainAfterConversionIfAppropriateFixed<float>(audio_buffer_interleaved<float> buffer, std::size_t countRendered, std::size_t channels, std::size_t countChunk, float gainFactor)
{
	// Apply final output gain for floating point output after conversion so we do not suffer underflow or clipping
	ApplyGain(buffer, countRendered, channels, countChunk, gainFactor);
}
template<>
void ApplyGainAfterConversionIfAppropriateFixed<float>(audio_buffer_planar<float> buffer, std::size_t countRendered, std::size_t channels, std::size_t countChunk, float gainFactor)
{
	// Apply final output gain for floating point output after conversion so we do not suffer underflow or clipping
	ApplyGain(buffer, countRendered, channels, countChunk, gainFactor);
}

inline void ApplyGainBeforeConversionIfAppropriateFloat(MixSampleFloat *MixSoundBuffer, std::size_t channels, std::size_t countChunk, float gainFactor)
{
	// Apply final output gain for non floating point output
	ApplyGain(MixSoundBuffer, channels, countChunk, gainFactor);
}

template<typename Tbuffer>
class AudioReadTargetGainBuffer
	: public AudioReadTargetBuffer<Tbuffer>
{
private:
	typedef AudioReadTargetBuffer<Tbuffer> Tbase;
private:
	const float gainFactor;
public:
	AudioReadTargetGainBuffer(Tbuffer buf, Dither &dither, float gainFactor_)
		: Tbase(buf, dither)
		, gainFactor(gainFactor_)
	{
		return;
	}
public:
	void DataCallback(MixSampleInt *MixSoundBuffer, std::size_t channels, std::size_t countChunk) override
	{
		const std::size_t countRendered_ = Tbase::GetRenderedCount();
		ApplyGainBeforeConversionIfAppropriateFixed<typename Tbuffer::sample_type>(MixSoundBuffer, channels, countChunk, gainFactor);
		Tbase::DataCallback(MixSoundBuffer, channels, countChunk);
		ApplyGainAfterConversionIfAppropriateFixed<typename Tbuffer::sample_type>(Tbase::outputBuffer, countRendered_, channels, countChunk, gainFactor);
	}
	void DataCallback(MixSampleFloat *MixSoundBuffer, std::size_t channels, std::size_t countChunk) override
	{
		ApplyGainBeforeConversionIfAppropriateFloat(MixSoundBuffer, channels, countChunk, gainFactor);
		Tbase::DataCallback(MixSoundBuffer, channels, countChunk);
	}
};


#endif // LIBOPENMPT_BUILD


OPENMPT_NAMESPACE_END
