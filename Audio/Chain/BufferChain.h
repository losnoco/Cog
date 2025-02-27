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
#import <CogAudio/DSPRubberbandNode.h>
#import <CogAudio/DSPFSurroundNode.h>
#import <CogAudio/DSPHRTFNode.h>
#import <CogAudio/DSPEqualizerNode.h>
#import <CogAudio/VisualizationNode.h>
#import <CogAudio/DSPDownmixNode.h>
#import <CogAudio/InputNode.h>

@interface BufferChain : NSObject {
	InputNode *inputNode;
	ConverterNode *converterNode;
	DSPRubberbandNode *rubberbandNode;
	DSPFSurroundNode *fsurroundNode;
	DSPHRTFNode *hrtfNode;
	DSPEqualizerNode *equalizerNode;
	DSPDownmixNode *downmixNode;
	VisualizationNode *visualizationNode;

	NSURL *streamURL;
	id userInfo;
	NSDictionary *rgInfo;

	id finalNode; // Final buffer in the chain.

	id controller;
}

- (id)initWithController:(id)c;
- (BOOL)buildChain;

- (BOOL)open:(NSURL *)url withOutputFormat:(AudioStreamBasicDescription)outputFormat withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi;

// Used when changing tracks to reuse the same decoder
- (BOOL)openWithInput:(InputNode *)i withOutputFormat:(AudioStreamBasicDescription)outputFormat withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi;

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

- (DSPRubberbandNode *)rubberband;

- (DSPFSurroundNode *)fsurround;

- (DSPHRTFNode *)hrtf;

- (DSPEqualizerNode *)equalizer;

- (DSPDownmixNode *)downmix;

- (VisualizationNode *)visualization;

- (double)secondsBuffered;

- (void)sustainHDCD;

- (void)restartPlaybackAtCurrentPosition;

- (void)pushInfo:(NSDictionary *)info;

- (void)setError:(BOOL)status;

- (double)getPostVisLatency;

@end
