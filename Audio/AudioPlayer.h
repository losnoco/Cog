//
//  AudioController.h
//  Cog
//
//  Created by Vincent Spader on 8/7/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <CogAudio/CogSemaphore.h>

#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>
#import <CoreAudio/CoreAudio.h>
#import <CoreAudio/CoreAudioTypes.h>

#import <stdatomic.h>

@class BufferChain;
@class OutputNode;

@interface AudioPlayer : NSObject {
	BufferChain *bufferChain;
	OutputNode *output;

	BOOL muted;
	double volume;
	double pitch;
	double tempo;

	NSMutableArray *chainQueue;

	NSURL *nextStream;
	id nextStreamUserInfo;
	NSDictionary *nextStreamRGInfo;

	id previousUserInfo; // Track currently last heard track for play counts

	id delegate;

	BOOL outputLaunched;
	BOOL endOfInputReached;
	BOOL startedPaused;
	BOOL initialBufferFilled;

	Semaphore *semaphore;

	atomic_bool resettingNow;
	atomic_int refCount;

	int currentPlaybackStatus;

	BOOL shouldContinue;
}

- (id)init;

- (void)setDelegate:(id)d;
- (id)delegate;

- (void)play:(NSURL *)url;
- (void)play:(NSURL *)url withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi;
- (void)play:(NSURL *)url withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi startPaused:(BOOL)paused;
- (void)play:(NSURL *)url withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi startPaused:(BOOL)paused andSeekTo:(double)time;
- (void)playBG:(NSURL *)url withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi startPaused:(NSNumber *)paused andSeekTo:(NSNumber *)time;

- (void)stop;
- (void)pause;
- (void)resume;

- (void)seekToTime:(double)time;
- (void)seekToTimeBG:(NSNumber *)time;
- (void)setVolume:(double)v;
- (double)volume;
- (double)volumeUp:(double)amount;
- (double)volumeDown:(double)amount;

- (void)mute;
- (void)unmute;

- (double)amountPlayed;
- (double)amountPlayedInterval;

- (void)setNextStream:(NSURL *)url;
- (void)setNextStream:(NSURL *)url withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi;
- (void)resetNextStreams;

- (void)restartPlaybackAtCurrentPosition;

- (void)pushInfo:(NSDictionary *)info toTrack:(id)userInfo;

+ (NSArray *)fileTypes;
+ (NSArray *)schemes;
+ (NSArray *)containerTypes;

@end

@interface AudioPlayer (Private) // Dont use this stuff!

- (OutputNode *)output;
- (BufferChain *)bufferChain;
- (id)initWithDelegate:(id)d;

- (void)setPlaybackStatus:(int)status waitUntilDone:(BOOL)wait;
- (void)setPlaybackStatus:(int)s;

- (void)requestNextStream:(id)userInfo;
- (void)requestNextStreamMainThread:(id)userInfo;

- (void)notifyStreamChanged:(id)userInfo;
- (void)notifyStreamChangedMainThread:(id)userInfo;

- (void)beginEqualizer:(AudioUnit)eq;
- (void)refreshEqualizer:(AudioUnit)eq;
- (void)endEqualizer:(AudioUnit)eq;

- (BOOL)endOfInputReached:(BufferChain *)sender;
- (void)setShouldContinue:(BOOL)s;
//- (BufferChain *)bufferChain;
- (void)launchOutputThread;
- (BOOL)selectNextBuffer;
- (void)endOfInputPlayed;
- (void)reportPlayCount;
- (void)sendDelegateMethod:(SEL)selector withVoid:(void *)obj waitUntilDone:(BOOL)wait;
- (void)sendDelegateMethod:(SEL)selector withObject:(id)obj waitUntilDone:(BOOL)wait;
- (void)sendDelegateMethod:(SEL)selector withObject:(id)obj withObject:(id)obj2 waitUntilDone:(BOOL)wait;

- (BOOL)chainQueueHasTracks;
@end

@protocol AudioPlayerDelegate
- (void)audioPlayer:(AudioPlayer *)player willEndStream:(id)userInfo; // You must use setNextStream in this method
- (void)audioPlayer:(AudioPlayer *)player didBeginStream:(id)userInfo;
- (void)audioPlayer:(AudioPlayer *)player didChangeStatus:(id)status userInfo:(id)userInfo;
- (void)audioPlayer:(AudioPlayer *)player didStopNaturally:(id)userInfo;
- (void)audioPlayer:(AudioPlayer *)player displayEqualizer:(AudioUnit)eq;
- (void)audioPlayer:(AudioPlayer *)player refreshEqualizer:(AudioUnit)eq;
- (void)audioPlayer:(AudioPlayer *)player removeEqualizer:(AudioUnit)eq;
- (void)audioPlayer:(AudioPlayer *)player sustainHDCD:(id)userInfo;
- (void)audioPlayer:(AudioPlayer *)player restartPlaybackAtCurrentPosition:(id)userInfo;
- (void)audioPlayer:(AudioPlayer *)player pushInfo:(NSDictionary *)info toTrack:(id)userInfo;
- (void)audioPlayer:(AudioPlayer *)player reportPlayCountForTrack:(id)userInfo;
- (void)audioPlayer:(AudioPlayer *)player updatePosition:(id)userInfo;
- (void)audioPlayer:(AudioPlayer *)player setError:(NSNumber *)status toTrack:(id)userInfo;
@end
