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
#include "Dither.h"
#include "../soundbase/SampleFormat.h"
#include "../soundbase/SampleFormatConverters.h"
#include "../soundbase/SampleFormatCopy.h"
#include "MixerLoops.h"
#include "Mixer.h"


OPENMPT_NAMESPACE_BEGIN


template<typename Tsample, bool clipOutput = false>
class AudioReadTargetBuffer
	: public IAudioReadTarget
{
private:
	std::size_t countRendered;
	Dither &dither;
protected:
	Tsample *outputBuffer;
	Tsample * const *outputBuffers;
public:
	AudioReadTargetBuffer(Dither &dither_, Tsample *buffer, Tsample * const *buffers)
		: countRendered(0)
		, dither(dither_)
		, outputBuffer(buffer)
		, outputBuffers(buffers)
	{
		MPT_ASSERT(SampleFormat(SampleFormatTraits<Tsample>::sampleFormat).IsValid());
	}
	std::size_t GetRenderedCount() const { return countRendered; }
public:
	void DataCallback(int32 *MixSoundBuffer, std::size_t channels, std::size_t countChunk) override
	{
		// Convert to output sample format and optionally perform dithering and clipping if needed

		const SampleFormat sampleFormat = SampleFormatTraits<Tsample>::sampleFormat;

		if(sampleFormat.IsInt())
		{
			dither.Process(MixSoundBuffer, countChunk, channels, sampleFormat.GetBitsPerSample());
		}

		if(outputBuffer)
		{
			ConvertInterleavedFixedPointToInterleaved<MIXING_FRACTIONAL_BITS, clipOutput>(outputBuffer + (channels * countRendered), MixSoundBuffer, channels, countChunk);
		}
		if(outputBuffers)
		{
			Tsample *buffers[4] = { nullptr, nullptr, nullptr, nullptr };
			for(std::size_t channel = 0; channel < channels; ++channel)
			{
				buffers[channel] = outputBuffers[channel] + countRendered;
			}
			ConvertInterleavedFixedPointToNonInterleaved<MIXING_FRACTIONAL_BITS, clipOutput>(buffers, MixSoundBuffer, channels, countChunk);
		}

		countRendered += countChunk;
	}
};


#if defined(MODPLUG_TRACKER)


class AudioReadTargetBufferInterleavedDynamic
	: public IAudioReadTarget
{
private:
	const SampleFormat sampleFormat;
	bool clipFloat;
	Dither &dither;
	void *buffer;
public:
	AudioReadTargetBufferInterleavedDynamic(SampleFormat sampleFormat_, bool clipFloat_, Dither &dither_, void *buffer_)
		: sampleFormat(sampleFormat_)
		, clipFloat(clipFloat_)
		, dither(dither_)
		, buffer(buffer_)
	{
		MPT_ASSERT_ALWAYS(sampleFormat.IsValid());
	}
	void DataCallback(int32 *MixSoundBuffer, std::size_t channels, std::size_t countChunk) override
	{
		switch(sampleFormat.value)
		{
			case SampleFormatUnsigned8:
				{
					typedef SampleFormatToType<SampleFormatUnsigned8>::type Tsample;
					AudioReadTargetBuffer<Tsample> target(dither, reinterpret_cast<Tsample*>(buffer), nullptr);
					target.DataCallback(MixSoundBuffer, channels, countChunk);
				}
				break;
			case SampleFormatInt16:
				{
					typedef SampleFormatToType<SampleFormatInt16>::type Tsample;
					AudioReadTargetBuffer<Tsample> target(dither, reinterpret_cast<Tsample*>(buffer), nullptr);
					target.DataCallback(MixSoundBuffer, channels, countChunk);
				}
				break;
			case SampleFormatInt24:
				{
					typedef SampleFormatToType<SampleFormatInt24>::type Tsample;
					AudioReadTargetBuffer<Tsample> target(dither, reinterpret_cast<Tsample*>(buffer), nullptr);
					target.DataCallback(MixSoundBuffer, channels, countChunk);
				}
				break;
			case SampleFormatInt32:
				{
					typedef SampleFormatToType<SampleFormatInt32>::type Tsample;
					AudioReadTargetBuffer<Tsample> target(dither, reinterpret_cast<Tsample*>(buffer), nullptr);
					target.DataCallback(MixSoundBuffer, channels, countChunk);
				}
				break;
			case SampleFormatFloat32:
				if(clipFloat)
				{
					typedef SampleFormatToType<SampleFormatFloat32>::type Tsample;
					AudioReadTargetBuffer<Tsample, true> target(dither, reinterpret_cast<Tsample*>(buffer), nullptr);
					target.DataCallback(MixSoundBuffer, channels, countChunk);
				} else
				{
					typedef SampleFormatToType<SampleFormatFloat32>::type Tsample;
					AudioReadTargetBuffer<Tsample, false> target(dither, reinterpret_cast<Tsample*>(buffer), nullptr);
					target.DataCallback(MixSoundBuffer, channels, countChunk);
				}
				break;
		}
		// increment output buffer for potentially next callback
		buffer = reinterpret_cast<char*>(buffer) + (sampleFormat.GetBitsPerSample()/8) * channels * countChunk;
	}
};


class AudioSourceBuffer
	: public IAudioSource
{
private:
	std::size_t countRendered;
protected:
	SampleFormat sampleFormat;
	const void *inputBuffer;
public:
	AudioSourceBuffer(SampleFormat sampleFormat, const void *buffer)
		: countRendered(0)
		, sampleFormat(sampleFormat)
		, inputBuffer(buffer)
	{
		MPT_ASSERT(sampleFormat.IsValid());
	}
	virtual ~AudioSourceBuffer() { }
	std::size_t GetRenderedCount() const { return countRendered; }
private:
	template<typename Tsample>
	void Fill(const Tsample *inputBuffer, int32 * const *MixInputBuffers, std::size_t channels, std::size_t countChunk)
	{
		for(std::size_t channel = 0; channel < channels; ++channel)
		{
			SC::ConvertToFixedPoint<int32, Tsample, MIXING_FRACTIONAL_BITS> conv;
			for(std::size_t frame = 0; frame < countChunk; ++frame)
			{
				MixInputBuffers[channel][frame] = conv(inputBuffer[channel + ((countRendered + frame) * channels)]);
			}
		}
		countRendered += countChunk;
	}
public:
	virtual void FillCallback(int32 * const *MixInputBuffers, std::size_t channels, std::size_t countChunk)
	{
		switch(sampleFormat.value)
		{
		case SampleFormatUnsigned8:
			{
				typedef SampleFormatToType<SampleFormatUnsigned8>::type Tsample;
				Fill(reinterpret_cast<const Tsample*>(inputBuffer), MixInputBuffers, channels, countChunk);
			}
			break;
		case SampleFormatInt16:
			{
				typedef SampleFormatToType<SampleFormatInt16>::type Tsample;
				Fill(reinterpret_cast<const Tsample*>(inputBuffer), MixInputBuffers, channels, countChunk);
			}
			break;
		case SampleFormatInt24:
			{
				typedef SampleFormatToType<SampleFormatInt24>::type Tsample;
				Fill(reinterpret_cast<const Tsample*>(inputBuffer), MixInputBuffers, channels, countChunk);
			}
			break;
		case SampleFormatInt32:
			{
				typedef SampleFormatToType<SampleFormatInt32>::type Tsample;
				Fill(reinterpret_cast<const Tsample*>(inputBuffer), MixInputBuffers, channels, countChunk);
			}
			break;
		case SampleFormatFloat32:
			{
				typedef SampleFormatToType<SampleFormatFloat32>::type Tsample;
				Fill(reinterpret_cast<const Tsample*>(inputBuffer), MixInputBuffers, channels, countChunk);
			}
			break;
		}
	}
};


#else // !MODPLUG_TRACKER


template<typename Tsample>
void ApplyGainBeforeConversionIfAppropriate(int32 *MixSoundBuffer, std::size_t channels, std::size_t countChunk, float gainFactor)
{
	// Apply final output gain for non floating point output
	ApplyGain(MixSoundBuffer, channels, countChunk, mpt::saturate_round<int32>(gainFactor * (1<<16)));
}
template<>
void ApplyGainBeforeConversionIfAppropriate<float>(int32 * /*MixSoundBuffer*/, std::size_t /*channels*/, std::size_t /*countChunk*/, float /*gainFactor*/)
{
	// nothing
}

template<typename Tsample>
void ApplyGainAfterConversionIfAppropriate(Tsample * /*buffer*/, Tsample * const * /*buffers*/, std::size_t /*countRendered*/, std::size_t /*channels*/, std::size_t /*countChunk*/, float /*gainFactor*/)
{
	// nothing
}
template<>
void ApplyGainAfterConversionIfAppropriate<float>(float *buffer, float * const *buffers, std::size_t countRendered, std::size_t channels, std::size_t countChunk, float gainFactor)
{
	// Apply final output gain for floating point output after conversion so we do not suffer underflow or clipping
	ApplyGain(buffer, buffers, countRendered, channels, countChunk, gainFactor);
}

template<typename Tsample>
class AudioReadTargetGainBuffer
	: public AudioReadTargetBuffer<Tsample>
{
private:
	typedef AudioReadTargetBuffer<Tsample> Tbase;
private:
	const float gainFactor;
public:
	AudioReadTargetGainBuffer(Dither &dither, Tsample *buffer, Tsample * const *buffers, float gainFactor_)
		: Tbase(dither, buffer, buffers)
		, gainFactor(gainFactor_)
	{
		return;
	}
public:
	void DataCallback(int32 *MixSoundBuffer, std::size_t channels, std::size_t countChunk) override
	{
		const std::size_t countRendered_ = Tbase::GetRenderedCount();

		ApplyGainBeforeConversionIfAppropriate<Tsample>(MixSoundBuffer, channels, countChunk, gainFactor);

		Tbase::DataCallback(MixSoundBuffer, channels, countChunk);

		ApplyGainAfterConversionIfAppropriate<Tsample>(Tbase::outputBuffer, Tbase::outputBuffers, countRendered_, channels, countChunk, gainFactor);

	}
};


#endif // MODPLUG_TRACKER


OPENMPT_NAMESPACE_END
