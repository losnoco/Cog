/*
 * modsmp_ctrl.h
 * -------------
 * Purpose: Basic sample editing code
 * Notes  : (currently none)
 * Authors: OpenMPT Devs
 * The OpenMPT source code is released under the BSD license. Read LICENSE for more details.
 */


#pragma once

#include "BuildSettings.h"

#include "Snd_defs.h"

OPENMPT_NAMESPACE_BEGIN

class CSoundFile;
struct ModSample;
struct ModChannel;

namespace ctrlSmp
{

// Replaces sample in 'smp' with given sample and frees the old sample.
void ReplaceSample(ModSample &smp, void *pNewSample,  const SmpLength newLength, CSoundFile &sndFile);

// Propagate loop point changes to player
bool UpdateLoopPoints(const ModSample &smp, CSoundFile &sndFile);

// Invert sample data (flip by 180 degrees)
bool InvertSample(ModSample &smp, SmpLength start, SmpLength end, CSoundFile &sndFile);

// Crossfade sample data to create smooth loops
bool XFadeSample(ModSample &smp, SmpLength fadeLength, int fadeLaw, bool afterloopFade, bool useSustainLoop, CSoundFile &sndFile);

enum StereoToMonoMode
{
	mixChannels,
	onlyLeft,
	onlyRight,
	splitSample,
};

// Convert a sample with any number of channels to mono
bool ConvertToMono(ModSample &smp, CSoundFile &sndFile, StereoToMonoMode conversionMode);

// Convert a mono sample to stereo
bool ConvertToStereo(ModSample &smp, CSoundFile &sndFile);

} // Namespace ctrlSmp

namespace ctrlChn
{

// Replaces sample from sound channels by given sample.
void ReplaceSample( CSoundFile &sndFile,
					const ModSample &sample,
					const void * const pNewSample,
					const SmpLength newLength,
					FlagSet<ChannelFlags> setFlags,
					FlagSet<ChannelFlags> resetFlags);

} // namespace ctrlChn


OPENMPT_NAMESPACE_END
