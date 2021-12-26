/*
 * SampleBuffer.h
 * --------------
 * Purpose: C++2b audio_buffer-like thingy.
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */

#pragma once

#include "BuildSettings.h"

#include "../common/mptBaseMacros.h"
#include "../common/mptBaseTypes.h"


OPENMPT_NAMESPACE_BEGIN


template <typename SampleType>
struct audio_buffer_planar
{
public:
	using sample_type = SampleType;
private:
	SampleType * const * m_buffers;
	std::size_t m_channels;
	std::size_t m_frames;
	std::size_t m_offset;
public:
	constexpr audio_buffer_planar(SampleType * const * buffers, std::size_t channels, std::size_t frames)
		: m_buffers(buffers)
		, m_channels(channels)
		, m_frames(frames)
		, m_offset(0)
	{
		return;
	}
	SampleType & operator()(std::size_t channel, std::size_t frame)
	{
		return m_buffers[channel][m_offset + frame];
	}
	const SampleType & operator()(std::size_t channel, std::size_t frame) const
	{
		return m_buffers[channel][m_offset + frame];
	}
	std::size_t size_channels() const noexcept
	{
		return m_channels;
	}
	std::size_t size_frames() const noexcept
	{
		return m_frames;
	}
	audio_buffer_planar & advance(std::size_t numFrames)
	{
		m_offset += numFrames;
		m_frames -= numFrames;
		return *this;
	}
};

template <typename SampleType>
struct audio_buffer_interleaved
{
public:
	using sample_type = SampleType;
private:
	SampleType * m_buffer;
	std::size_t m_channels;
	std::size_t m_frames;
public:
	constexpr audio_buffer_interleaved(SampleType* buffer, std::size_t channels, std::size_t frames)
		: m_buffer(buffer)
		, m_channels(channels)
		, m_frames(frames)
	{
		return;
	}
	SampleType * data()
	{
		return m_buffer;
	}
	const SampleType * data() const
	{
		return m_buffer;
	}
	SampleType & operator()(std::size_t channel, std::size_t frame)
	{
		return m_buffer[m_channels * frame + channel];
	}
	const SampleType & operator()(std::size_t channel, std::size_t frame) const
	{
		return m_buffer[m_channels * frame + channel];
	}
	std::size_t size_channels() const noexcept
	{
		return m_channels;
	}
	std::size_t size_frames() const noexcept
	{
		return m_frames;
	}
	audio_buffer_interleaved & advance(std::size_t numFrames)
	{
		m_buffer += size_channels() * numFrames;
		m_frames -= numFrames;
		return *this;
	}
};

template <typename SampleType>
std::size_t planar_audio_buffer_valid_channels(SampleType * const * buffers, std::size_t maxChannels)
{
	std::size_t channel;
	for(channel = 0; channel < maxChannels; ++channel)
	{
		if(!buffers[channel])
		{
			break;
		}
	}
	return channel;
}

template <typename BufferType>
BufferType advance_audio_buffer(BufferType buf, std::size_t numFrames)
{
	MPT_ASSERT(numFrames <= buf.size_frames());
	return buf.advance(numFrames);
}


OPENMPT_NAMESPACE_END
