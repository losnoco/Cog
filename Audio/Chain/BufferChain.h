//
//  BufferChain.h
//  CogNew
//
//  Created by Vincent Spader on 1/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <CogAudio/AudioPlayer.h>
#import <CogAudio/ConverterNode.h>
#import <CogAudio/InputNode.h>

@interface BufferChain : NSObject {
	InputNode *inputNode;
	ConverterNode *converterNode;

	NSURL *streamURL;
	id userInfo;
	NSDictionary *rgInfo;

	id finalNode; // Final buffer in the chain.

	id controller;
}

- (id)initWithController:(id)c;
- (BOOL)buildChain:(BOOL)resetBuffers;

- (BOOL)open:(NSURL *)url withOutputFormat:(AudioStreamBasicDescription)outputFormat withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi;

- (BOOL)open:(NSURL *)url withOutputFormat:(AudioStreamBasicDescription)outputFormat withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi resetBuffers:(BOOL)resetBuffers;

// Used when changing tracks to reuse the same decoder
- (BOOL)openWithInput:(InputNode *)i withOutputFormat:(AudioStreamBasicDescription)outputFormat withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi;

- (BOOL)openWithInput:(InputNode *)i withOutputFormat:(AudioStreamBasicDescription)outputFormat withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi resetBuffers:(BOOL)resetBuffers;

// Used when resetting the decoder on seek
- (BOOL)openWithDecoder:(id<CogDecoder>)decoder
       withOutputFormat:(AudioStreamBasicDescription)outputFormat
           withUserInfo:(id)userInfo
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

- (BOOL)isRunning;

- (id)controller;

- (ConverterNode *)converter;
- (AudioStreamBasicDescription)inputFormat;
- (uint32_t)inputConfig;

- (double)secondsBuffered;

- (void)sustainHDCD;

- (void)restartPlaybackAtCurrentPosition;

- (void)pushInfo:(NSDictionary *)info;

- (void)setError:(BOOL)status;

@end
