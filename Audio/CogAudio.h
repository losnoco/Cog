//
//  CogAudio.h
//  CogAudio
//
//  Umbrella header for the CogAudio framework module. Every public header
//  must be listed here, so `import CogAudio` (Swift) and `@import CogAudio;`
//  (Objective-C) see the whole public interface.
//

#import <Foundation/Foundation.h>

FOUNDATION_EXPORT double CogAudioVersionNumber;
FOUNDATION_EXPORT const unsigned char CogAudioVersionString[];

// Plugin contract and player
#import <CogAudio/Plugin.h>
#import <CogAudio/PluginController.h>
#import <CogAudio/Status.h>
#import <CogAudio/Helper.h>
#import <CogAudio/AudioPlayer.h>
#import <CogAudio/AudioSource.h>
#import <CogAudio/AudioContainer.h>
#import <CogAudio/AudioDecoder.h>
#import <CogAudio/AudioMetadataReader.h>
#import <CogAudio/AudioPropertiesReader.h>

// Utilities
#import <CogAudio/CogSemaphore.h>
#import <CogAudio/CoreAudioUtils.h>
#import <CogAudio/soxr.h>

// Chain engine
#import <CogAudio/AudioChunk.h>
#import <CogAudio/ChunkList.h>
#import <CogAudio/Node.h>
#import <CogAudio/InputNode.h>
#import <CogAudio/ConverterNode.h>
#import <CogAudio/BufferChain.h>
#import <CogAudio/DSPNode.h>
#import <CogAudio/Downmix.h>
#import <CogAudio/DSPDownmixNode.h>
#import <CogAudio/DSPEqualizerNode.h>
#import <CogAudio/DSPFaderNode.h>
#import <CogAudio/DSPFSurroundNode.h>
#import <CogAudio/DSPHRTFNode.h>
#import <CogAudio/DSPRubberbandNode.h>
#import <CogAudio/FSurroundFilter.h>
#import <CogAudio/HeadphoneFilter.h>
#import <CogAudio/SimpleBuffer.h>
#import <CogAudio/VisualizationNode.h>
#import <CogAudio/OutputCoreAudio.h>
#import <CogAudio/OutputNode.h>

// Visualization
#import <CogAudio/MIDIVisualizationController.h>
#import <CogAudio/VisualizationController.h>

// Engine real-time core
#import <CogAudio/CogRing.h>
#import <CogAudio/CogRender.h>
