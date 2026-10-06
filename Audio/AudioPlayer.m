//  AudioController.m
//  Cog
//
//  Created by Vincent Spader on 8/7/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import "AudioPlayer.h"
#import "Helper.h"
#import "PluginController.h"
#import "Status.h"

#import "Logging.h"

#import "CogAudio-Swift.h"

NSNotificationName const CogAudioOutputStatusDidChangeNotification = @"CogAudioOutputStatusDidChangeNotification";

NSString *const CogAudioOutputSourceFormatKey = @"sourceFormat";
NSString *const CogAudioOutputSourceCodecKey = @"sourceCodec";
NSString *const CogAudioOutputSourceEncodingKey = @"sourceEncoding";
NSString *const CogAudioOutputRenderFormatKey = @"renderFormat";
NSString *const CogAudioOutputDoPKey = @"dop";
NSString *const CogAudioOutputDeviceNameKey = @"deviceName";
NSString *const CogAudioOutputSystemDefaultKey = @"systemDefault";
NSString *const CogAudioOutputExclusiveKey = @"exclusive";
NSString *const CogAudioOutputVirtualFormatsKey = @"virtualFormats";
NSString *const CogAudioOutputPhysicalFormatsKey = @"physicalFormats";
NSString *const CogAudioOutputModificationsKey = @"modifications";
NSString *const CogAudioOutputTrackGainKey = @"trackGain";
NSString *const CogAudioOutputFittedChannelsKey = @"fittedChannels";
NSString *const CogAudioOutputVolumeKey = @"volume";

NSString *const CogAudioOutputModificationDSDToPCM = @"dsdToPCM";
NSString *const CogAudioOutputModificationHDCD = @"hdcd";
NSString *const CogAudioOutputModificationPrecision = @"precision";
NSString *const CogAudioOutputModificationResampling = @"resampling";
NSString *const CogAudioOutputModificationTrackGain = @"trackGain";
NSString *const CogAudioOutputModificationTimeStretch = @"timeStretch";
NSString *const CogAudioOutputModificationFreeSurround = @"freeSurround";
NSString *const CogAudioOutputModificationEqualizer = @"equalizer";
NSString *const CogAudioOutputModificationSpatialAudio = @"spatialAudio";
NSString *const CogAudioOutputModificationChannelLayout = @"channelLayout";
NSString *const CogAudioOutputModificationVolume = @"volume";

// Playback runs in the engine (Audio/Engine). It reports back through
// PlaybackEngineHost, which this class turns into the delegate messages the
// playlist side has always received.
@interface AudioPlayer () <PlaybackEngineHost>
@end

@implementation AudioPlayer {
	PlaybackEngine *engine;
}

- (id)init {
	self = [super init];
	if(self) {
		// Created at launch: a device a crash left held is put back now,
		// not on the first play.
		[PlaybackEngine recoverAbandonedExclusiveOutput];
	}
	return self;
}

- (void)setDelegate:(id)d {
	delegate = d;
}

- (id)delegate {
	return delegate;
}

- (void)play:(NSURL *)url {
	[self play:url withUserInfo:nil withRGInfo:nil startPaused:NO andSeekTo:0.0];
}

- (void)play:(NSURL *)url withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi {
	[self play:url withUserInfo:userInfo withRGInfo:rgi startPaused:NO andSeekTo:0.0];
}

- (void)play:(NSURL *)url withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi startPaused:(BOOL)paused {
	[self play:url withUserInfo:userInfo withRGInfo:rgi startPaused:paused andSeekTo:0.0];
}

- (void)playBG:(NSURL *)url withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi startPaused:(NSNumber *)paused andSeekTo:(NSNumber *)time {
	@synchronized (self) {
		[self play:url withUserInfo:userInfo withRGInfo:rgi startPaused:[paused boolValue] andSeekTo:[time doubleValue]];
	}
}

- (void)play:(NSURL *)url withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi startPaused:(BOOL)paused andSeekTo:(double)time {
	ALog(@"Opening file for playback: %@ at seek offset %f%@", url, time, (paused) ? @", starting paused" : @"");

	if(!engine) {
		engine = [PlaybackEngine new];
		engine.host = self;
	}
	engine.volume = volume;

	[self notifyStreamChanged:userInfo];
	previousUserInfo = userInfo;

	if(![engine play:url userInfo:userInfo rgInfo:rgi startPaused:paused seekTo:time]) {
		ALog(@"The audio engine could not open the output device");
		[self setError:YES forTrack:userInfo];
		engine = nil;
		[self setPlaybackStatus:CogStatusStopped waitUntilDone:YES];
	}
}

- (void)stop {
	[engine stop];
}

- (void)pause {
	[engine pause];
}

- (void)resume {
	[engine resume];
}

- (void)seekToTimeBG:(NSNumber *)time {
	[self seekToTime:[time doubleValue]];
}

- (void)seekToTime:(double)time {
	[engine seekTo:time];
	[self updatePosition:previousUserInfo];
}

- (void)setVolume:(double)v {
	volume = v;
	engine.volume = v;
}

- (double)volume {
	return volume;
}

// This is called by the delegate DURING a requestNextStream request.
- (void)setNextStream:(NSURL *)url {
	[self setNextStream:url withUserInfo:nil withRGInfo:nil];
}

- (void)setNextStream:(NSURL *)url withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi {
	nextStream = url;
	nextStreamUserInfo = userInfo;
	nextStreamRGInfo = rgi;
}

// Called when the playlist changed before we actually started playing a requested stream. We will re-request.
- (void)resetNextStreams {
	[engine resetNextStreams];
}

- (void)restartPlaybackAtCurrentPosition {
	[self sendDelegateMethod:@selector(audioPlayer:restartPlaybackAtCurrentPosition:) withObject:previousUserInfo waitUntilDone:NO];
}

- (void)updatePosition:(id)userInfo {
	[self sendDelegateMethod:@selector(audioPlayer:updatePosition:) withObject:userInfo waitUntilDone:NO];
}

- (void)setRGInfo:(NSDictionary *)rgi forTrack:(id)userInfo {
	if(!userInfo) return;
	[engine updateReplayGain:rgi forTrack:userInfo];
	if(nextStreamUserInfo == userInfo) {
		nextStreamRGInfo = rgi;
	}
}

- (void)pushInfo:(NSDictionary *)info toTrack:(id)userInfo {
	[self sendDelegateMethod:@selector(audioPlayer:pushInfo:toTrack:) withObject:info withObject:userInfo waitUntilDone:NO];
}

- (void)reportPlayCountForTrack:(id)userInfo {
	[self sendDelegateMethod:@selector(audioPlayer:reportPlayCountForTrack:) withObject:userInfo waitUntilDone:NO];
}

- (double)amountPlayed {
	return engine.amountPlayed;
}

- (double)amountPlayedInterval {
	return engine.amountPlayedInterval;
}

- (void)setScrobbleThreshold:(double)threshold {
	[engine setScrobbleThreshold:threshold];
}

- (void)requestNextStream:(id)userInfo {
	[self sendDelegateMethod:@selector(audioPlayer:willEndStream:) withObject:userInfo waitUntilDone:YES];
}

- (void)notifyStreamChanged:(id)userInfo {
	[self sendDelegateMethod:@selector(audioPlayer:didBeginStream:) withObject:userInfo waitUntilDone:YES];
}

- (void)notifyPlaybackStopped:(id)userInfo {
	[self sendDelegateMethod:@selector(audioPlayer:didStopNaturally:) withObject:userInfo waitUntilDone:NO];
}

- (void)beginEqualizer:(void *)eq {
	[self sendDelegateMethod:@selector(audioPlayer:displayEqualizer:) withVoid:eq waitUntilDone:YES];
}

- (void)refreshEqualizer:(void *)eq {
	[self sendDelegateMethod:@selector(audioPlayer:refreshEqualizer:) withVoid:eq waitUntilDone:YES];
}

- (void)endEqualizer:(void *)eq {
	[self sendDelegateMethod:@selector(audioPlayer:removeEqualizer:) withVoid:eq waitUntilDone:YES];
}

- (void)reportPlayCount {
	[self reportPlayCountForTrack:previousUserInfo];
}

- (void)reportScrobble {
	[self reportScrobbleForTrack:previousUserInfo];
}

- (void)reportScrobbleForTrack:(id)userInfo {
	[self sendDelegateMethod:@selector(audioPlayer:reportScrobbleForTrack:) withObject:userInfo waitUntilDone:NO];
}

- (void)sendDelegateMethod:(SEL)selector withVoid:(void *)obj waitUntilDone:(BOOL)wait {
	NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:[delegate methodSignatureForSelector:selector]];
	[invocation setTarget:delegate];
	[invocation setSelector:selector];
	[invocation setArgument:(void *)&self atIndex:2];
	[invocation setArgument:&obj atIndex:3];
	[invocation retainArguments];

	[invocation performSelectorOnMainThread:@selector(invoke) withObject:nil waitUntilDone:wait];
}

- (void)sendDelegateMethod:(SEL)selector withObject:(id)obj waitUntilDone:(BOOL)wait {
	NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:[delegate methodSignatureForSelector:selector]];
	[invocation setTarget:delegate];
	[invocation setSelector:selector];
	[invocation setArgument:(void *)&self atIndex:2];
	[invocation setArgument:&obj atIndex:3];
	[invocation retainArguments];

	[invocation performSelectorOnMainThread:@selector(invoke) withObject:nil waitUntilDone:wait];
}

- (void)sendDelegateMethod:(SEL)selector withObject:(id)obj withObject:(id)obj2 waitUntilDone:(BOOL)wait {
	NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:[delegate methodSignatureForSelector:selector]];
	[invocation setTarget:delegate];
	[invocation setSelector:selector];
	[invocation setArgument:(void *)&self atIndex:2];
	[invocation setArgument:&obj atIndex:3];
	[invocation setArgument:&obj2 atIndex:4];
	[invocation retainArguments];

	[invocation performSelectorOnMainThread:@selector(invoke) withObject:nil waitUntilDone:wait];
}

- (void)setPlaybackStatus:(CogStatus)status waitUntilDone:(BOOL)wait {
	currentPlaybackStatus = status;

	[self sendDelegateMethod:@selector(audioPlayer:didChangeStatus:userInfo:) withObject:@(status) withObject:previousUserInfo waitUntilDone:wait];
}

- (void)setError:(BOOL)status forTrack:(id)userInfo {
	[self sendDelegateMethod:@selector(audioPlayer:setError:toTrack:) withObject:@(status) withObject:userInfo waitUntilDone:NO];
}

- (void)setPlaybackStatus:(CogStatus)status {
	[self setPlaybackStatus:status waitUntilDone:NO];
}

+ (NSArray *)containerTypes {
	return [[[PluginController sharedPluginController] containers] allKeys];
}

+ (NSArray *)fileTypes {
	PluginController *pluginController = [PluginController sharedPluginController];

	NSArray *containerTypes = [[pluginController containers] allKeys];
	NSArray *decoderTypes = [[pluginController decodersByExtension] allKeys];
	NSArray *metdataReaderTypes = [[pluginController metadataReaders] allKeys];
	NSArray *propertiesReaderTypes = [[pluginController propertiesReadersByExtension] allKeys];

	NSMutableSet *types = [NSMutableSet set];

	[types addObjectsFromArray:containerTypes];
	[types addObjectsFromArray:decoderTypes];
	[types addObjectsFromArray:metdataReaderTypes];
	[types addObjectsFromArray:propertiesReaderTypes];

	return [types allObjects];
}

+ (NSArray *)schemes {
	PluginController *pluginController = [PluginController sharedPluginController];

	return [[pluginController sources] allKeys];
}

- (double)volumeUp:(double)amount {
	BOOL volumeLimit = [[NSUserDefaults standardUserDefaults] boolForKey:@"volumeLimit"];
	const double MAX_VOLUME = (volumeLimit) ? 100.0 : 800.0;

	double newVolume = linearToLogarithmic(logarithmicToLinear(volume + amount, MAX_VOLUME), MAX_VOLUME);
	if(newVolume > MAX_VOLUME)
		newVolume = MAX_VOLUME;

	[self setVolume:newVolume];

	// the playbackController needs to know the new volume, so it can update the
	// volumeSlider accordingly.
	return newVolume;
}

- (double)volumeDown:(double)amount {
	BOOL volumeLimit = [[NSUserDefaults standardUserDefaults] boolForKey:@"volumeLimit"];
	const double MAX_VOLUME = (volumeLimit) ? 100.0 : 800.0;

	double newVolume;
	if(amount > volume)
		newVolume = 0.0;
	else
		newVolume = linearToLogarithmic(logarithmicToLinear(volume - amount, MAX_VOLUME), MAX_VOLUME);

	[self setVolume:newVolume];
	return newVolume;
}

#pragma mark - PlaybackEngineHost

- (EngineTrack *)playbackEngineNextTrackAfter:(id)userInfo {
	[self requestNextStream:userInfo];
	if(!nextStream) {
		return nil;
	}
	return [[EngineTrack alloc] initWithUrl:nextStream userInfo:nextStreamUserInfo rgInfo:nextStreamRGInfo];
}

- (void)playbackEngineDidBeginTrack:(id)userInfo {
	previousUserInfo = userInfo;
	[self notifyStreamChanged:userInfo];
}

- (void)playbackEngineDidChangeStatus:(CogStatus)status userInfo:(id)userInfo {
	currentPlaybackStatus = status;
	[self sendDelegateMethod:@selector(audioPlayer:didChangeStatus:userInfo:) withObject:@(status) withObject:userInfo waitUntilDone:YES];
}

- (void)playbackEngineDidStopNaturally:(id)userInfo {
	[self notifyPlaybackStopped:userInfo];
}

- (void)playbackEngineReportPlayCount:(id)userInfo {
	[self reportPlayCountForTrack:userInfo];
}

- (void)playbackEngineReportScrobble:(id)userInfo {
	[self reportScrobbleForTrack:userInfo];
}

- (void)playbackEnginePushInfo:(NSDictionary *)info toTrack:(id)userInfo {
	[self pushInfo:info toTrack:userInfo];
}

- (void)playbackEngineSetError:(BOOL)error forTrack:(id)userInfo {
	[self setError:error forTrack:userInfo];
}

- (void)playbackEngineBeginEqualizer:(id<CogEqualizer>)equalizer {
	[self beginEqualizer:(__bridge void *)equalizer];
}

- (void)playbackEngineEndEqualizer:(id<CogEqualizer>)equalizer {
	[self endEqualizer:(__bridge void *)equalizer];
}

- (void)playbackEngineRestartAtCurrentPosition:(id)userInfo {
	[self sendDelegateMethod:@selector(audioPlayer:restartPlaybackAtCurrentPosition:) withObject:userInfo waitUntilDone:NO];
}

- (void)playbackEngineOutputStatusDidChange:(NSDictionary *)status {
	[[NSNotificationCenter defaultCenter] postNotificationName:CogAudioOutputStatusDidChangeNotification object:self userInfo:status];
}

@end
