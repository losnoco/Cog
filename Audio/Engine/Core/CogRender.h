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

#include <CogAudio/CogRing.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma clang assume_nonnull begin

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

/// Render thread: fills `frames` frames of `out` (in the ring's channel
/// count) and returns how many came from the ring; the rest are silence.
size_t cog_renderer_render(CogRenderer *renderer, float *out, size_t frames);

/// Frames delivered to the device, audio and silence alike.
uint64_t cog_renderer_frames_rendered(const CogRenderer *renderer);

/// Silent frames delivered because the ring had nothing to give, whether
/// playback had not started, had ended, or had underrun.
uint64_t cog_renderer_silent_frames(const CogRenderer *renderer);

/// Times the ring ran dry after having had audio.
uint64_t cog_renderer_underrun_events(const CogRenderer *renderer);

#pragma clang assume_nonnull end

#ifdef __cplusplus
}
#endif

#endif /* CogRender_h */
