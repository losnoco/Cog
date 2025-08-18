//
//  InputNode.h
//  Cog
//
//  Created by Vincent Spader on 8/2/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>
#import <CoreAudio/AudioHardware.h>

#import <CogAudio/AudioDecoder.h>
#import <CogAudio/Node.h>
#import <CogAudio/Plugin.h>

#define INPUT_NODE_SEEK

@interface InputNode : Node {
	id<CogDecoder> decoder;

	int bytesPerSample;
	int bytesPerFrame;
	BOOL floatingPoint;
	BOOL swapEndian;

	BOOL shouldSeek;
	long seekFrame;

	BOOL observersAdded;

	BOOL shouldResetBuffers;

	Semaphore *exitAtTheEndOfTheStream;
}
@property(readonly) Semaphore * _Nonnull exitAtTheEndOfTheStream;
@property(readonly) BOOL threadExited;

- (id _Nullable)initWithController:(id _Nonnull)c previous:(id _Nullable)p;

- (BOOL)openWithSource:(id<CogSource>_Nonnull)source;
- (BOOL)openWithDecoder:(id<CogDecoder>_Nonnull)d;

- (void)process;
- (NSDictionary *_Nonnull)properties;
- (void)seek:(long)frame;

- (void)registerObservers;

- (BOOL)setTrack:(NSURL *_Nonnull)track;

- (id<CogDecoder>_Nonnull)decoder;

- (void)setResetBuffers:(BOOL)resetBuffers;

@end
