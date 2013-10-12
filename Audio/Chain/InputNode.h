//
//  InputNode.h
//  Cog
//
//  Created by Vincent Spader on 8/2/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <CoreAudio/AudioHardware.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>

#import "AudioDecoder.h"
#import "Node.h"
#import "Plugin.h"

#define INPUT_NODE_SEEK

@interface InputNode : Node {
	id<CogDecoder> decoder;
	
    int bytesPerSample;
	int bytesPerFrame;
    BOOL floatingPoint;
    BOOL swapEndian;
	
	BOOL shouldSeek;
	long seekFrame;

    Semaphore *exitAtTheEndOfTheStream;
}

- (BOOL)openWithSource:(id<CogSource>)source;
- (BOOL)openWithDecoder:(id<CogDecoder>) d;

- (void)process;
- (NSDictionary *) properties;
- (void)seek:(long)frame;

- (void)registerObservers;

- (BOOL)setTrack:(NSURL *)track;

- (id<CogDecoder>) decoder;

@end
