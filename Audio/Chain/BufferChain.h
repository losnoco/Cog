//
//  BufferChain.h
//  CogNew
//
//  Created by Vincent Spader on 1/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "AudioPlayer.h"
#import "ConverterNode.h"
#import "InputNode.h"

@interface BufferChain : NSObject {
	InputNode *inputNode;
	ConverterNode *converterNode;

	AudioStreamBasicDescription inputFormat;

	NSURL *streamURL;
	id userInfo;
	NSDictionary *rgInfo;

	id finalNode; // Final buffer in the chain.

	id controller;
}

- (id)initWithController:(id)c;
- (void)buildChain;

- (BOOL)open:(NSURL *)url withOutputFormat:(AudioStreamBasicDescription)outputFormat withRGInfo:(NSDictionary *)rgi;

// Used when changing tracks to reuse the same decoder
- (BOOL)openWithInput:(InputNode *)i withOutputFormat:(AudioStreamBasicDescription)outputFormat withRGInfo:(NSDictionary *)rgi;

// Used when resetting the decoder on seek
- (BOOL)openWithDecoder:(id<CogDecoder>)decoder
       withOutputFormat:(AudioStreamBasicDescription)outputFormat
             withRGInfo:(NSDictionary *)rgi;

- (void)seek:(double)time;

- (void)launchThreads;

- (InputNode *)inputNode;

- (id)finalNode;

- (id)userInfo;
- (void)setUserInfo:(id)i;

- (NSDictionary *)rgInfo;
- (void)setRGInfo:(NSDictionary *)rgi;

- (NSURL *)streamURL;
- (void)setStreamURL:(NSURL *)url;

- (void)setShouldContinue:(BOOL)s;

- (void)initialBufferFilled:(id)sender;

- (BOOL)endOfInputReached;
- (BOOL)setTrack:(NSURL *)track;

- (void)inputFormatDidChange:(AudioStreamBasicDescription)format;

- (BOOL)isRunning;

- (id)controller;

- (ConverterNode *)converter;
- (AudioStreamBasicDescription)inputFormat;

- (double)secondsBuffered;

- (void)sustainHDCD;

@end
