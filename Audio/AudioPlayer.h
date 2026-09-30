//
//  AudioController.h
//  Cog
//
//  Created by Vincent Spader on 8/7/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>
#import <CoreAudio/CoreAudio.h>
#import <CoreAudio/CoreAudioTypes.h>

@interface AudioPlayer : NSObject {
	double volume;

	NSURL *nextStream;
	id nextStreamUserInfo;
	NSDictionary *nextStreamRGInfo;

	id previousUserInfo; // Track currently last heard track for play counts

	id delegate;

	int currentPlaybackStatus;
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

- (double)amountPlayed;
- (double)amountPlayedInterval;

- (void)setScrobbleThreshold:(double)threshold;

- (void)setNextStream:(NSURL *)url;
- (void)setNextStream:(NSURL *)url withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi;
- (void)resetNextStreams;

- (void)restartPlaybackAtCurrentPosition;

- (void)pushInfo:(NSDictionary *)info toTrack:(id)userInfo;

// New ReplayGain info for a track that is playing or queued, for example
// once its tags have loaded after playback began.
- (void)setRGInfo:(NSDictionary *)rgi forTrack:(id)userInfo;

+ (NSArray *)fileTypes;
+ (NSArray *)schemes;
+ (NSArray *)containerTypes;

@end

@interface AudioPlayer (Private) // Dont use this stuff!

- (void)setPlaybackStatus:(int)status waitUntilDone:(BOOL)wait;
- (void)setPlaybackStatus:(int)s;

- (void)requestNextStream:(id)userInfo;

- (void)notifyStreamChanged:(id)userInfo;

- (void)beginEqualizer:(void *)eq;
- (void)refreshEqualizer:(void *)eq;
- (void)endEqualizer:(void *)eq;

- (void)reportPlayCount;
- (void)reportScrobble;
- (void)setError:(BOOL)status forTrack:(id)userInfo;
- (void)sendDelegateMethod:(SEL)selector withVoid:(void *)obj waitUntilDone:(BOOL)wait;
- (void)sendDelegateMethod:(SEL)selector withObject:(id)obj waitUntilDone:(BOOL)wait;
- (void)sendDelegateMethod:(SEL)selector withObject:(id)obj withObject:(id)obj2 waitUntilDone:(BOOL)wait;
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
- (void)audioPlayer:(AudioPlayer *)player reportScrobbleForTrack:(id)userInfo;
- (void)audioPlayer:(AudioPlayer *)player updatePosition:(id)userInfo;
- (void)audioPlayer:(AudioPlayer *)player setError:(NSNumber *)status toTrack:(id)userInfo;
@end
