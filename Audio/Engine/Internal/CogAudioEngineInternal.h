//
//  CogAudioEngineInternal.h
//  CogAudio
//
//  C headers the engine's Swift uses but CogAudio does not publish. Swift
//  imports this through `module.modulemap` beside it, with
//  `@_implementationOnly`, so none of it leaks into the public module.
//

#ifndef CogAudioEngineInternal_h
#define CogAudioEngineInternal_h

#include <stddef.h>

#include "../../ThirdParty/lvqcl/lpc.h"
#include "../../../ThirdParty/rubberband/include/rubberband/rubberband-c.h"
#include "../Core/CogSignalsmith.h"

#endif /* CogAudioEngineInternal_h */
