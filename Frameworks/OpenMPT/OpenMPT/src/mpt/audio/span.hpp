/* SPDX-License-Identifier: BSL-1.0 OR BSD-3-Clause */

#ifndef MPT_AUDIO_SPAN_HPP
#define MPT_AUDIO_SPAN_HPP



#include "mpt/base/namespace.hpp"

#include <type_traits>

#include <cassert>
#include <cstddef>



namespace mpt {
inline namespace MPT_INLINE_NS {



// LxxxLxxxLxxxLxxxLxxx xRxxxRxxxRxxxRxxxRxx
template <typename SampleType>
struct audio_span_planar_strided {
public:
	using sample_type = SampleType;

private:
	SampleType * const * m_buffers;
	std::ptrdiff_t m_frame_stride;
	std::size_t m_channels;
	std::size_t m_frames;

public:
	constexpr audio_span_planar_strided(SampleType * const * buffers, std::size_t channels, std::size_t frames, std::ptrdiff_t frame_stride) noexcept
		: m_buffers(buffers)
		, m_frame_stride(frame_stride)
		, m_channels(channels)
		, m_frames(frames) {
		return;
	}
	SampleType * const * data_planar() const noexcept {
		return m_buffers;
	}
	SampleType * data() const noexcept {
		return nullptr;
	}
	SampleType & operator()(std::size_t channel, std::size_t frame) const {
		return m_buffers[channel][static_cast<std::ptrdiff_t>(frame) * m_frame_stride];
	}
	bool is_contiguous() const noexcept {
		return false;
	}
	bool channels_are_contiguous() const noexcept {
		return false;
	}
	bool frames_are_contiguous() const noexcept {
		return false;
	}
	std::size_t size_channels() const noexcept {
		return m_channels;
	}
	std::size_t size_frames() const noexcept {
		return m_frames;
	}
	std::size_t size_samples() const noexcept {
		return m_channels * m_frames;
	}
	std::ptrdiff_t frame_stride() const noexcept {
		return m_frame_stride;
	}
};


// LLLLL RRRRR
template <typename SampleType>
struct audio_span_planar {
public:
	using sample_type = SampleType;

private:
	SampleType * const * m_buffers;
	std::size_t m_channels;
	std::size_t m_frames;

public:
	constexpr audio_span_planar(SampleType * const * buffers, std::size_t channels, std::size_t frames) noexcept
		: m_buffers(buffers)
		, m_channels(channels)
		, m_frames(frames) {
		return;
	}
	SampleType * const * data_planar() const noexcept {
		return m_buffers;
	}
	SampleType * data() const noexcept {
		return nullptr;
	}
	SampleType & operator()(std::size_t channel, std::size_t frame) const {
		return m_buffers[channel][frame];
	}
	bool is_contiguous() const noexcept {
		return false;
	}
	bool channels_are_contiguous() const noexcept {
		return false;
	}
	bool frames_are_contiguous() const noexcept {
		return false;
	}
	std::size_t size_channels() const noexcept {
		return m_channels;
	}
	std::size_t size_frames() const noexcept {
		return m_frames;
	}
	std::size_t size_samples() const noexcept {
		return m_channels * m_frames;
	}
};


// LLLLLRRRRR
template <typename SampleType>
struct audio_span_contiguous {
public:
	using sample_type = SampleType;

private:
	SampleType * const m_buffer;
	std::size_t m_channels;
	std::size_t m_frames;

public:
	constexpr audio_span_contiguous(SampleType * buffer, std::size_t channels, std::size_t frames) noexcept
		: m_buffer(buffer)
		, m_channels(channels)
		, m_frames(frames) {
		return;
	}
	SampleType * const * data_planar() const noexcept {
		return nullptr;
	}
	SampleType * data() const noexcept {
		return m_buffer;
	}
	SampleType & operator()(std::size_t channel, std::size_t frame) const {
		return m_buffer[(m_frames * channel) + frame];
	}
	bool is_contiguous() const noexcept {
		return true;
	}
	bool channels_are_contiguous() const noexcept {
		return true;
	}
	bool frames_are_contiguous() const noexcept {
		return false;
	}
	std::size_t size_channels() const noexcept {
		return m_channels;
	}
	std::size_t size_frames() const noexcept {
		return m_frames;
	}
	std::size_t size_samples() const noexcept {
		return m_channels * m_frames;
	}
};


// LRLRLRLRLR
template <typename SampleType>
struct audio_span_interleaved {
public:
	using sample_type = SampleType;

private:
	SampleType * const m_buffer;
	std::size_t m_channels;
	std::size_t m_frames;

public:
	constexpr audio_span_interleaved(SampleType * buffer, std::size_t channels, std::size_t frames) noexcept
		: m_buffer(buffer)
		, m_channels(channels)
		, m_frames(frames) {
		return;
	}
	SampleType * const * data_planar() const noexcept {
		return nullptr;
	}
	SampleType * data() const noexcept {
		return m_buffer;
	}
	SampleType & operator()(std::size_t channel, std::size_t frame) const {
		return m_buffer[m_channels * frame + channel];
	}
	bool is_contiguous() const noexcept {
		return true;
	}
	bool channels_are_contiguous() const noexcept {
		return false;
	}
	bool frames_are_contiguous() const noexcept {
		return true;
	}
	std::size_t size_channels() const noexcept {
		return m_channels;
	}
	std::size_t size_frames() const noexcept {
		return m_frames;
	}
	std::size_t size_samples() const noexcept {
		return m_channels * m_frames;
	}
};


struct audio_span_frames_are_contiguous_t {
};

struct audio_span_channels_are_contiguous_t {
};

struct audio_span_channels_are_planar_t {
};

struct audio_span_channels_are_planar_and_strided_t {
};

// LRLRLRLRLR
inline constexpr audio_span_frames_are_contiguous_t audio_span_frames_are_contiguous;

// LLLLLRRRRR
inline constexpr audio_span_channels_are_contiguous_t audio_span_channels_are_contiguous;

// LLLLL RRRRR
inline constexpr audio_span_channels_are_planar_t audio_span_channels_are_planar;

// LxxxLxxxLxxxLxxxLxxx xRxxxRxxxRxxxRxxxRxx
inline constexpr audio_span_channels_are_planar_and_strided_t audio_span_channels_are_planar_and_strided;

template <typename SampleType>
struct audio_span {
public:
	using sample_type = SampleType;

private:
	union {
		SampleType * const contiguous;
		SampleType * const * const planes;
	} m_buffer;
	std::ptrdiff_t m_frame_stride;
	std::ptrdiff_t m_channel_stride;
	std::size_t m_channels;
	std::size_t m_frames;

public:
	constexpr audio_span(audio_span_interleaved<SampleType> buffer) noexcept
		: m_frame_stride(static_cast<std::ptrdiff_t>(buffer.size_channels()))
		, m_channel_stride(1)
		, m_channels(buffer.size_channels())
		, m_frames(buffer.size_frames()) {
		m_buffer.contiguous = buffer.data();
	}
	constexpr audio_span(SampleType * buffer, std::size_t channels, std::size_t frames, audio_span_frames_are_contiguous_t) noexcept
		: m_frame_stride(static_cast<std::ptrdiff_t>(channels))
		, m_channel_stride(1)
		, m_channels(channels)
		, m_frames(frames) {
		m_buffer.contiguous = buffer;
	}
	constexpr audio_span(audio_span_contiguous<SampleType> buffer) noexcept
		: m_frame_stride(1)
		, m_channel_stride(buffer.size_frames())
		, m_channels(buffer.size_channels())
		, m_frames(buffer.size_frames()) {
		m_buffer.contiguous = buffer.data();
	}
	constexpr audio_span(SampleType * buffer, std::size_t channels, std::size_t frames, audio_span_channels_are_contiguous_t) noexcept
		: m_frame_stride(1)
		, m_channel_stride(static_cast<std::ptrdiff_t>(frames))
		, m_channels(channels)
		, m_frames(frames) {
		m_buffer.contiguous = buffer;
	}
	constexpr audio_span(audio_span_planar<SampleType> buffer) noexcept
		: m_frame_stride(1)
		, m_channel_stride(0)
		, m_channels(buffer.size_channels())
		, m_frames(buffer.size_frames()) {
		m_buffer.planes = buffer.data_planar();
	}
	constexpr audio_span(SampleType * const * planes, std::size_t channels, std::size_t frames, audio_span_channels_are_planar_t) noexcept
		: m_frame_stride(1)
		, m_channel_stride(0)
		, m_channels(channels)
		, m_frames(frames) {
		m_buffer.planes = planes;
	}
	constexpr audio_span(audio_span_planar_strided<SampleType> buffer) noexcept
		: m_frame_stride(static_cast<std::ptrdiff_t>(buffer.frame_stride()))
		, m_channel_stride(0)
		, m_channels(buffer.size_channels())
		, m_frames(buffer.size_frames()) {
		m_buffer.planes = buffer.data_planar();
	}
	constexpr audio_span(SampleType * const * planes, std::size_t channels, std::size_t frames, std::ptrdiff_t frame_stride, audio_span_channels_are_planar_and_strided_t) noexcept
		: m_frame_stride(frame_stride)
		, m_channel_stride(0)
		, m_channels(channels)
		, m_frames(frames) {
		m_buffer.planes = planes;
	}
	bool is_contiguous() const noexcept {
		return (m_channel_stride != 0);
	}
	SampleType * const * data_planar() const noexcept {
		return (!is_contiguous()) ? m_buffer.planes : nullptr;
	}
	SampleType * data() const noexcept {
		return is_contiguous() ? m_buffer.contiguous : nullptr;
	}
	SampleType & operator()(std::size_t channel, std::size_t frame) const {
		return is_contiguous() ? m_buffer.contiguous[(m_channel_stride * static_cast<std::ptrdiff_t>(channel)) + (m_frame_stride * static_cast<std::ptrdiff_t>(frame))] : m_buffer.planes[channel][frame * static_cast<std::ptrdiff_t>(m_frame_stride)];
	}
	bool channels_are_contiguous() const noexcept {
		return (m_channel_stride == static_cast<std::ptrdiff_t>(m_frames));
	}
	bool frames_are_contiguous() const noexcept {
		return (m_frame_stride == static_cast<std::ptrdiff_t>(m_channels));
	}
	std::size_t size_channels() const noexcept {
		return m_channels;
	}
	std::size_t size_frames() const noexcept {
		return m_frames;
	}
	std::size_t size_samples() const noexcept {
		return m_channels * m_frames;
	}
};


template <typename Taudio_span>
struct audio_span_with_offset {
public:
	using sample_type = typename Taudio_span::sample_type;

private:
	Taudio_span m_buffer;
	std::size_t m_offset;

public:
	audio_span_with_offset(Taudio_span buffer, std::size_t offsetFrames) noexcept
		: m_buffer(buffer)
		, m_offset(offsetFrames) {
		return;
	}
	sample_type * data() const noexcept {
		if (!is_contiguous())
		{
			return nullptr;
		}
		return m_buffer.data() + (size_channels() * m_offset);
	}
	sample_type & operator()(std::size_t channel, std::size_t frame) const {
		return m_buffer(channel, m_offset + frame);
	}
	bool is_contiguous() const noexcept {
		return m_buffer.is_contiguous() && m_buffer.frames_are_contiguous();
	}
	bool channels_are_contiguous() const noexcept {
		return m_buffer.channels_are_contiguous();
	}
	bool frames_are_contiguous() const noexcept {
		return m_buffer.frames_are_contiguous();
	}
	std::size_t size_channels() const noexcept {
		return m_buffer.size_channels();
	}
	std::size_t size_frames() const noexcept {
		return m_buffer.size_frames() - m_offset;
	}
	std::size_t size_samples() const noexcept {
		return size_channels() * size_frames();
	}
};



template <typename BufferType>
inline audio_span_with_offset<BufferType> make_audio_span_with_offset(BufferType buf, std::size_t offsetFrames) noexcept {
	assert(offsetFrames <= buf.size_frames());
	return audio_span_with_offset<BufferType>{buf, offsetFrames};
}



} // namespace MPT_INLINE_NS
} // namespace mpt



#endif // MPT_AUDIO_SPAN_HPP
