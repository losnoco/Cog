//
//  CogSignalsmith.h
//  CogAudio
//
//  Created by Christopher Snowhill on 9/30/26.
//

#ifndef CogSignalsmith_h
#define CogSignalsmith_h

#ifdef __cplusplus
extern "C" {
#endif

#pragma clang assume_nonnull begin

/// A C face on Signalsmith Stretch (header-only C++), so the engine's Swift
/// can drive it. Buffers are arrays of per-channel sample pointers.
typedef struct CogSignalsmith CogSignalsmith;

CogSignalsmith *_Nullable cog_signalsmith_create(int channels, float sampleRate);
void cog_signalsmith_destroy(CogSignalsmith *_Nullable stretch);

/// Transposes by `factor` (1 = none), as DSPSignalsmithStretchNode did.
void cog_signalsmith_set_transpose(CogSignalsmith *stretch, float factor, float sampleRate);

void cog_signalsmith_reset(CogSignalsmith *stretch);
int cog_signalsmith_input_latency(const CogSignalsmith *stretch);
int cog_signalsmith_output_latency(const CogSignalsmith *stretch);

/// Input frames `output_seek` wants to start at `rate` (tempo) aligned.
int cog_signalsmith_output_seek_length(const CogSignalsmith *stretch, float rate);
void cog_signalsmith_output_seek(CogSignalsmith *stretch, const float *const *inputs, int frames);

void cog_signalsmith_process(CogSignalsmith *stretch, const float *const *inputs, int inputFrames, float *const *outputs, int outputFrames);
void cog_signalsmith_flush(CogSignalsmith *stretch, float *const *outputs, int outputFrames);

#pragma clang assume_nonnull end

#ifdef __cplusplus
}
#endif

#endif /* CogSignalsmith_h */
