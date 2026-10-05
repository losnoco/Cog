//
//  CogRender.h
//  CogAudio
//
//  Created by Christopher Snowhill on 9/29/26.
//

#ifndef CogRender_h
#define CogRender_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <AudioToolbox/AudioToolbox.h>

#include <CogAudio/CogRing.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma clang assume_nonnull begin

// MARK: - DoP

/// Whether `count` interleaved frames are all DSD over PCM: the same 0x05 or
/// 0xFA marker byte in every channel of a frame, alternating frame to frame.
/// On success `nextMarker` (if given) is the marker the next frame must carry.
bool cog_dop_validate(const float *frames, size_t channels, size_t count, uint8_t *_Nullable nextMarker);

/// Fills `count` frames with DoP silence (the 0x69 idle pattern), continuing
/// from `marker` and leaving it at the marker for the frame after.
void cog_dop_fill_silence(float *frames, size_t channels, size_t count, uint8_t *marker);

/// A gain applied on the render thread and steered from any other thread.
///
/// The controlling thread asks for a target and a ramp length; the render
/// thread moves toward it one frame at a time, so volume changes and
/// transport fades never step. Only one thread may apply the gain.
typedef struct CogGain CogGain;

CogGain *_Nullable cog_gain_create(float initial);
void cog_gain_destroy(CogGain *_Nullable gain);

/// Ramps linearly from wherever the gain is to `target` over `frames`
/// frames; zero jumps on the next render.
void cog_gain_ramp_to(CogGain *gain, float target, uint32_t frames);

/// Ramps from `from` to `target` over `frames` frames, as one request: the
/// render thread applies only the latest request, so "jump to 0" followed by
/// "ramp to 1" before a render would otherwise lose the jump. NaN for
/// `from` means wherever the gain is.
///
/// Requests come from one controlling thread at a time.
void cog_gain_ramp(CogGain *gain, float from, float target, uint32_t frames);

/// The most recently requested target.
float cog_gain_target(const CogGain *gain);

/// The gain as of the last render.
float cog_gain_current(const CogGain *gain);

/// True once the render thread has reached the most recent target.
bool cog_gain_settled(const CogGain *gain);

/// Render thread: scales `count` interleaved frames in place.
void cog_gain_apply(CogGain *gain, float *frames, size_t count, uint32_t channels);

/// The render callback's inner loop: pulls interleaved float frames from a
/// ring, fills any shortfall with silence, and applies the volume and
/// transport gains. It never allocates, locks or blocks.
typedef struct CogRenderer CogRenderer;

/// `ring` is borrowed and must outlive the renderer.
CogRenderer *_Nullable cog_renderer_create(CogRing *ring);
void cog_renderer_destroy(CogRenderer *_Nullable renderer);

CogRing *cog_renderer_ring(const CogRenderer *renderer);

/// The user's volume. Ramp it briefly to avoid zipper noise.
CogGain *cog_renderer_volume(const CogRenderer *renderer);

/// Pause, resume and stop fades.
CogGain *cog_renderer_transport(const CogRenderer *renderer);

/// Seek crossfades. When the renderer honours a flush of its ring while the
/// transport is audible, it keeps up to `frames` of the frames it discards
/// (the ones about to be heard) and fades them out on an equal-power curve
/// over the new audio, which fades in on the matching curve from its first
/// frame. Zero turns this off. Allocates, so call only while the renderer is
/// not running.
bool cog_renderer_set_crossfade_frames(CogRenderer *renderer, size_t frames);

/// Paused with the device still running: renders silence (DoP silence while
/// DoP is playing, so the DAC stays locked) without reading the ring, which
/// keeps its audio for resuming. Flushes are still honoured. From any
/// thread.
void cog_renderer_set_held(CogRenderer *renderer, bool held);

/// Whether the next flushes crossfade or cut; from any thread. A cut is for
/// when fades are turned off, and for DoP, which cannot be mixed.
void cog_renderer_set_crossfade_enabled(CogRenderer *renderer, bool enabled);

/// The sample words the renderer hands the device: float, or one of the
/// integer layouts a device held exclusively runs at (all signed, native
/// endian, interleaved).
typedef CF_ENUM(uint32_t, CogSampleFormat) {
	CogSampleFormatFloat32 = 0,
	/// 32 significant bits.
	CogSampleFormatInt32,
	/// 24 bits in the high three bytes of a 32-bit word, the low byte zero.
	CogSampleFormatInt24High,
	/// 24 bits in the low three bytes of a 32-bit word, sign-extended.
	CogSampleFormatInt24Low,
	/// 24 bits in three bytes.
	CogSampleFormatInt24Packed,
	CogSampleFormatInt16,
};

/// Bytes one sample takes in `format`.
size_t cog_sample_format_bytes(CogSampleFormat format);

/// Renders `format` instead of float. The conversion undoes the engine's
/// integer-to-float scaling exactly (a sample of n bits, n up to 24, became
/// s / 2^(n-1)), so integer audio nothing changed comes out as the integers
/// it went in as, and DoP carrier words pass exactly in any 24- or 32-bit
/// layout; anything else is rounded to nearest and clipped. Int16 output is
/// dithered where samples fall between its steps, as processing leaves them.
/// `maximumFrames`
/// is the most frames the device asks for at once. Allocates, so call only
/// while the renderer is not running.
bool cog_renderer_set_output_format(CogRenderer *renderer, CogSampleFormat format, size_t maximumFrames);

CogSampleFormat cog_renderer_output_format(const CogRenderer *renderer);

/// Render thread: fills `frames` frames of `out` (in the ring's channel
/// count) and returns how many came from the ring; the rest are silence.
///
/// DoP from the ring is passed bit-exact: no volume, transport ramp or
/// crossfade touches it (the transport can only let it through or replace
/// it with DoP silence), its marker phase is kept across calls, and while
/// DoP is playing, any shortfall is DoP silence rather than zeroes, which
/// would make a DAC drop out of DSD.
size_t cog_renderer_render(CogRenderer *renderer, float *out, size_t frames);

/// An AURenderCallback for an output unit whose input format is interleaved
/// in the ring's channel count, in the renderer's output format;
/// `inRefCon` is the CogRenderer. Plain C
/// on purpose: the device's I/O thread must run no Objective-C or Swift, whose
/// runtime locks another thread can hold (loading a bundle does) for tens of
/// milliseconds.
OSStatus cog_renderer_audio_unit_render(void *inRefCon,
                                        AudioUnitRenderActionFlags *ioActionFlags,
                                        const AudioTimeStamp *inTimeStamp,
                                        UInt32 inBusNumber,
                                        UInt32 inNumberFrames,
                                        AudioBufferList *_Nullable ioData);

/// An AudioDeviceIOProc for a device held exclusively, rendering straight
/// into its one output stream, which must run in the renderer's output
/// format with the ring's channel count; `inClientData` is the CogRenderer.
/// AUHAL will not drive a device whose stream is non-mixable (it moves to
/// another device instead), so exclusive output bypasses it; with no unit in
/// the way, nothing converts the samples after the renderer.
OSStatus cog_renderer_device_io_proc(AudioObjectID inDevice,
                                     const AudioTimeStamp *inNow,
                                     const AudioBufferList *inInputData,
                                     const AudioTimeStamp *inInputTime,
                                     AudioBufferList *outOutputData,
                                     const AudioTimeStamp *inOutputTime,
                                     void *_Nullable inClientData);

/// Converts `count` rendered float samples to `format` in `output`, as
/// `cog_renderer_set_output_format` describes.
void cog_convert_samples(void *output, CogSampleFormat format, const float *input, size_t count);

/// Frames delivered to the device, audio and silence alike.
uint64_t cog_renderer_frames_rendered(const CogRenderer *renderer);

/// Silent frames delivered because the ring had nothing to give, whether
/// playback had not started, had ended, or had underrun.
uint64_t cog_renderer_silent_frames(const CogRenderer *renderer);

/// Times the device's sample time did not continue from the previous
/// render: a cycle the device skipped or repeated, heard as a click even when
/// the audio was on time. Counted by `cog_renderer_audio_unit_render`.
uint64_t cog_renderer_device_discontinuities(const CogRenderer *renderer);

/// How many frames the last discontinuity jumped (negative: backwards).
int64_t cog_renderer_last_device_jump(const CogRenderer *renderer);

/// Forgets the device's sample time, as when the unit starts again and the
/// time legitimately starts over. Call while the unit is stopped.
void cog_renderer_forget_device_time(CogRenderer *renderer);

/// Times the ring ran dry after having had audio.
uint64_t cog_renderer_underrun_events(const CogRenderer *renderer);

/// The largest absolute sample delivered to the device since the last call,
/// after all gains; above 1.0 is louder than full scale. Resets it.
float cog_renderer_take_peak(CogRenderer *renderer);

// MARK: - Metering

/// Channels metered separately; any beyond share the last.
#define COG_METER_CHANNELS 8

/// The audio delivered to the device since the last take, per channel and
/// after all gains, as `cog_renderer_take_meter` reports it.
typedef struct CogMeterSnapshot {
	/// Largest absolute sample; above 1.0 is louder than full scale.
	float peak[COG_METER_CHANNELS];
	/// Sum of squared samples, for RMS over `frames`.
	double sumOfSquares[COG_METER_CHANNELS];
	/// Frames metered: audio only, not silence while held, dry or DoP.
	uint64_t frames;
	/// Samples beyond full scale, which integer output clips.
	uint64_t clippedSamples;
} CogMeterSnapshot;

/// Starts or stops metering. Off, the render callback does no metering work.
void cog_renderer_set_metering(CogRenderer *renderer, bool enabled);

/// What was metered since the last take; resets it. Lock-free, for any
/// thread while the renderer runs.
void cog_renderer_take_meter(CogRenderer *renderer, CogMeterSnapshot *snapshot);

#pragma clang assume_nonnull end

#ifdef __cplusplus
}
#endif

#endif /* CogRender_h */
