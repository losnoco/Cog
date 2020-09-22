/*
 * SampleFormatCopy.h
 * ------------------
 * Purpose: Functions for copying sample data.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"


#include "../common/Endianness.h"
#include "SampleFormatConverters.h"
#include "SampleFormat.h"


OPENMPT_NAMESPACE_BEGIN


//////////////////////////////////////////////////////
// Actual sample conversion functions

// Copy a sample data buffer.
// targetBuffer: Buffer in which the sample should be copied into.
// numSamples: Number of samples of size T that should be copied. targetBuffer is expected to be able to hold "numSamples * incTarget" samples.
// incTarget: Number of samples by which the target data pointer is increased each time.
// sourceBuffer: Buffer from which the samples should be read.
// sourceSize: Size of source buffer, in bytes.
// incSource: Number of samples by which the source data pointer is increased each time.
//
// Template arguments:
// SampleConversion: Functor of type SampleConversionFunctor to apply sample conversion (see above for existing functors).
template <typename SampleConversion>
size_t CopySample(typename SampleConversion::output_t * MPT_RESTRICT outBuf, size_t numSamples, size_t incTarget, const typename SampleConversion::input_t * MPT_RESTRICT inBuf, size_t sourceSize, size_t incSource, SampleConversion conv = SampleConversion())
{
	const size_t sampleSize = incSource * SampleConversion::input_inc * sizeof(typename SampleConversion::input_t);
	LimitMax(numSamples, sourceSize / sampleSize);
	const size_t copySize = numSamples * sampleSize;

	SampleConversion sampleConv(conv);
	while(numSamples--)
	{
		*outBuf = sampleConv(inBuf);
		outBuf += incTarget;
		inBuf += incSource * SampleConversion::input_inc;
	}

	return copySize;
}


// Copy numChannels interleaved sample streams.
template <typename SampleConversion>
void CopyInterleavedSampleStreams(typename SampleConversion::output_t * MPT_RESTRICT outBuf, const typename SampleConversion::input_t * MPT_RESTRICT inBuf, size_t numFrames, size_t numChannels, SampleConversion *conv)
{
	while(numFrames--)
	{
		for(size_t channel = 0; channel < numChannels; ++channel)
		{
			*outBuf = conv[channel](*inBuf);
			inBuf++;
			outBuf++;
		}
	}
}


// Copy numChannels interleaved sample streams.
template <typename SampleConversion>
void CopyInterleavedSampleStreams(typename SampleConversion::output_t * MPT_RESTRICT outBuf, const typename SampleConversion::input_t * MPT_RESTRICT inBuf, size_t numFrames, size_t numChannels, std::vector<SampleConversion> &conv)
{
	MPT_ASSERT(conv.size() >= numChannels);
	CopyInterleavedSampleStreams(outBuf, inBuf, numFrames, numChannels, &(conv[0]));
}


template<int fractionalBits, bool clipOutput, typename TOutBuf, typename TInBuf, typename Tdither>
void ConvertBufferMixFixedToBuffer(TOutBuf outBuf, TInBuf inBuf, Tdither & dither, std::size_t channels, std::size_t count)
{
	using TOutSample = typename std::remove_const<typename TOutBuf::sample_type>::type;
	using TInSample = typename std::remove_const<typename TInBuf::sample_type>::type;
	MPT_ASSERT(inBuf.size_channels() >= channels);
	MPT_ASSERT(outBuf.size_channels() >= channels);
	MPT_ASSERT(inBuf.size_frames() >= count);
	MPT_ASSERT(outBuf.size_frames() >= count);
	constexpr int ditherBits = SampleFormat(SampleFormatTraits<TOutSample>::sampleFormat()).IsInt()
		? SampleFormat(SampleFormatTraits<TOutSample>::sampleFormat()).GetBitsPerSample()
		: 0;
	SC::ClipFixed<int32, fractionalBits, clipOutput> clip;
	SC::ConvertFixedPoint<TOutSample, TInSample, fractionalBits> conv;
	for(std::size_t i = 0; i < count; ++i)
	{
		for(std::size_t channel = 0; channel < channels; ++channel)
		{
			outBuf(channel, i) = conv(clip(dither.template process<ditherBits>(channel, inBuf(channel, i))));
		}
	}
}


template<int fractionalBits, typename TOutBuf, typename TInBuf>
void ConvertBufferToBufferMixFixed(TOutBuf outBuf, TInBuf inBuf, std::size_t channels, std::size_t count)
{
	using TOutSample = typename std::remove_const<typename TOutBuf::sample_type>::type;
	using TInSample = typename std::remove_const<typename TInBuf::sample_type>::type;
	MPT_ASSERT(inBuf.size_channels() >= channels);
	MPT_ASSERT(outBuf.size_channels() >= channels);
	MPT_ASSERT(inBuf.size_frames() >= count);
	MPT_ASSERT(outBuf.size_frames() >= count);
	SC::ConvertToFixedPoint<TOutSample, TInSample, fractionalBits> conv;
	for(std::size_t i = 0; i < count; ++i)
	{
		for(std::size_t channel = 0; channel < channels; ++channel)
		{
			outBuf(channel, i) = conv(inBuf(channel, i));
		}
	}
}


template<bool clipOutput, typename TOutBuf, typename TInBuf, typename Tdither>
void ConvertBufferMixFloatToBuffer(TOutBuf outBuf, TInBuf inBuf, Tdither & dither, std::size_t channels, std::size_t count)
{
	using TOutSample = typename std::remove_const<typename TOutBuf::sample_type>::type;
	using TInSample = typename std::remove_const<typename TInBuf::sample_type>::type;
	MPT_ASSERT(inBuf.size_channels() >= channels);
	MPT_ASSERT(outBuf.size_channels() >= channels);
	MPT_ASSERT(inBuf.size_frames() >= count);
	MPT_ASSERT(outBuf.size_frames() >= count);
	constexpr int ditherBits = SampleFormat(SampleFormatTraits<TOutSample>::sampleFormat()).IsInt()
		? SampleFormat(SampleFormatTraits<TOutSample>::sampleFormat()).GetBitsPerSample()
		: 0;
	SC::ClipFloat<TInSample, clipOutput> clip;
	SC::Convert<TOutSample, TInSample> conv;
	for(std::size_t i = 0; i < count; ++i)
	{
		for(std::size_t channel = 0; channel < channels; ++channel)
		{
			outBuf(channel, i) = conv(clip(dither.template process<ditherBits>(channel, inBuf(channel, i))));
		}
	}
}


template<typename TOutBuf, typename TInBuf>
void ConvertBufferToBufferMixFloat(TOutBuf outBuf, TInBuf inBuf, std::size_t channels, std::size_t count)
{
	using TOutSample = typename std::remove_const<typename TOutBuf::sample_type>::type;
	using TInSample = typename std::remove_const<typename TInBuf::sample_type>::type;
	MPT_ASSERT(inBuf.size_channels() >= channels);
	MPT_ASSERT(outBuf.size_channels() >= channels);
	MPT_ASSERT(inBuf.size_frames() >= count);
	MPT_ASSERT(outBuf.size_frames() >= count);
	SC::Convert<TOutSample, TInSample> conv;
	for(std::size_t i = 0; i < count; ++i)
	{
		for(std::size_t channel = 0; channel < channels; ++channel)
		{
			outBuf(channel, i) = conv(inBuf(channel, i));
		}
	}
}


template<typename TOutBuf, typename TInBuf>
void ConvertBufferToBuffer(TOutBuf outBuf, TInBuf inBuf, std::size_t channels, std::size_t count)
{
	using TOutSample = typename std::remove_const<typename TOutBuf::sample_type>::type;
	using TInSample = typename std::remove_const<typename TInBuf::sample_type>::type;
	MPT_ASSERT(inBuf.size_channels() >= channels);
	MPT_ASSERT(outBuf.size_channels() >= channels);
	MPT_ASSERT(inBuf.size_frames() >= count);
	MPT_ASSERT(outBuf.size_frames() >= count);
	SC::Convert<TOutSample, TInSample> conv;
	for(std::size_t i = 0; i < count; ++i)
	{
		for(std::size_t channel = 0; channel < channels; ++channel)
		{
			outBuf(channel, i) = conv(inBuf(channel, i));
		}
	}
}


// Copy from an interleaed buffer of #channels.
template <typename SampleConversion>
void CopyInterleavedToChannel(typename SampleConversion::output_t * MPT_RESTRICT dst, const typename SampleConversion::input_t * MPT_RESTRICT src, std::size_t channels, std::size_t countChunk, std::size_t channel, SampleConversion conv = SampleConversion())
{
	SampleConversion sampleConv(conv);
	src += channel;
	for(std::size_t i = 0; i < countChunk; ++i)
	{
		*dst = sampleConv(*src);
		src += channels;
		dst++;
	}
}


// Copy buffer to an interleaed buffer of #channels.
template <typename SampleConversion>
void CopyChannelToInterleaved(typename SampleConversion::output_t * MPT_RESTRICT dst, const typename SampleConversion::input_t * MPT_RESTRICT src, std::size_t channels, std::size_t countChunk, std::size_t channel, SampleConversion conv = SampleConversion())
{
	SampleConversion sampleConv(conv);
	dst += channel;
	for(std::size_t i = 0; i < countChunk; ++i)
	{
		*dst = sampleConv(*src);
		src++;
		dst += channels;
	}
}


OPENMPT_NAMESPACE_END
