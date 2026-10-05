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

#import <CogAudio/Status.h>

// What playback sends to the output device, as heard. Posted on the main
// thread by the AudioPlayer whenever any of it changes (a new track, a DSP
// setting, the device), and with no userInfo once nothing is playing.
FOUNDATION_EXPORT NSNotificationName const CogAudioOutputStatusDidChangeNotification;

// NSValue (AudioStreamBasicDescription): the track as its decoder produces
// it. Missing if the decoder did not describe it.
FOUNDATION_EXPORT NSString *const CogAudioOutputSourceFormatKey;
// NSString: the track's codec and encoding ("lossless", "lossy" or
// "synthesized"), as its decoder names them, if it does.
FOUNDATION_EXPORT NSString *const CogAudioOutputSourceCodecKey;
FOUNDATION_EXPORT NSString *const CogAudioOutputSourceEncodingKey;
// NSValue (AudioStreamBasicDescription): what Cog renders for Core Audio.
FOUNDATION_EXPORT NSString *const CogAudioOutputRenderFormatKey;
// NSNumber (BOOL): the render format carries DSD over PCM.
FOUNDATION_EXPORT NSString *const CogAudioOutputDoPKey;
// NSString: the output device's name, if it has one.
FOUNDATION_EXPORT NSString *const CogAudioOutputDeviceNameKey;
// NSNumber (BOOL): Cog follows the system's default output device.
FOUNDATION_EXPORT NSString *const CogAudioOutputSystemDefaultKey;
// NSNumber (BOOL): Cog holds the device exclusively (hog mode, mixing off).
FOUNDATION_EXPORT NSString *const CogAudioOutputExclusiveKey;
// NSArray of NSValue (AudioStreamBasicDescription): the device's output
// streams as Core Audio mixes into them, and as the hardware runs.
FOUNDATION_EXPORT NSString *const CogAudioOutputVirtualFormatsKey;
FOUNDATION_EXPORT NSString *const CogAudioOutputPhysicalFormatsKey;
// NSArray of the modifications below, in signal order: how Cog changes the
// decoded samples before Core Audio has them. Empty if it passes them on as
// decoded; missing if that cannot be told (no source format).
FOUNDATION_EXPORT NSString *const CogAudioOutputModificationsKey;
// NSNumber (double), with CogAudioOutputModificationTrackGain: the track's
// gain in dB.
FOUNDATION_EXPORT NSString *const CogAudioOutputTrackGainKey;
// NSNumber (int), with CogAudioOutputModificationChannelLayout: the channels
// the DSP chain produced before they were fitted to the device.
FOUNDATION_EXPORT NSString *const CogAudioOutputFittedChannelsKey;
// NSNumber (double), with CogAudioOutputModificationVolume: Cog's volume in
// percent.
FOUNDATION_EXPORT NSString *const CogAudioOutputVolumeKey;

// Each stage's settings, for inspecting the signal path. A stage's
// dictionary is present while the stage is in the chain, which it may be
// without changing anything (CogAudioOutputStageActiveKey NO): FreeSurround
// given anything but stereo, or the resampler at equal rates.
//
// NSDictionary: the resampler, while the track reaches it as PCM. Input and
// output rates, quality ("HQ", soxr's), active.
FOUNDATION_EXPORT NSString *const CogAudioOutputResamplerKey;
// NSDictionary: tempo and pitch, while they are off unity. Engine (the
// rubberbandEngine setting: "varispeed", "faster", "finer" or
// "signalsmith"), tempo and pitch as ratios.
FOUNDATION_EXPORT NSString *const CogAudioOutputTimeStretchKey;
// NSDictionary: FreeSurround, while it is on. Active, and the channels and
// channel config it produces.
FOUNDATION_EXPORT NSString *const CogAudioOutputFreeSurroundKey;
// NSDictionary: the equalizer, while it is on. Preamp, band frequencies and
// band gains.
FOUNDATION_EXPORT NSString *const CogAudioOutputEqualizerKey;
// NSDictionary: the spatial mixer, while it renders the track. Spatial
// output ("headphones", "builtInSpeakers" or "externalSpeakers") and head
// tracking.
FOUNDATION_EXPORT NSString *const CogAudioOutputSpatialKey;

// The keys of those dictionaries.
// NSNumber (BOOL): the stage changes the audio.
FOUNDATION_EXPORT NSString *const CogAudioOutputStageActiveKey;
// NSNumber (double), Hz.
FOUNDATION_EXPORT NSString *const CogAudioOutputStageInputRateKey;
FOUNDATION_EXPORT NSString *const CogAudioOutputStageOutputRateKey;
// NSString.
FOUNDATION_EXPORT NSString *const CogAudioOutputStageQualityKey;
FOUNDATION_EXPORT NSString *const CogAudioOutputStageEngineKey;
// NSNumber (double), a ratio: 1 is unchanged.
FOUNDATION_EXPORT NSString *const CogAudioOutputStageTempoKey;
FOUNDATION_EXPORT NSString *const CogAudioOutputStagePitchKey;
// NSNumber (int, and uint32 channel config bits).
FOUNDATION_EXPORT NSString *const CogAudioOutputStageChannelsKey;
FOUNDATION_EXPORT NSString *const CogAudioOutputStageChannelConfigKey;
// NSNumber (double), dB; NSArray of NSNumber (double), Hz and dB.
FOUNDATION_EXPORT NSString *const CogAudioOutputStagePreampKey;
FOUNDATION_EXPORT NSString *const CogAudioOutputStageBandFrequenciesKey;
FOUNDATION_EXPORT NSString *const CogAudioOutputStageBandGainsKey;
// NSString; NSNumber (BOOL).
FOUNDATION_EXPORT NSString *const CogAudioOutputStageSpatialOutputKey;
FOUNDATION_EXPORT NSString *const CogAudioOutputStageHeadTrackingKey;

// NSString: where the track's gain comes from ("album", "track",
// "soundcheck" or "volume"), if anywhere.
FOUNDATION_EXPORT NSString *const CogAudioOutputTrackGainSourceKey;
// NSNumber (BOOL): the gain was lowered so the track's peak cannot clip.
FOUNDATION_EXPORT NSString *const CogAudioOutputTrackGainPeakLimitedKey;
// NSNumber (BOOL): spatial audio is on for the device, but the spatial
// mixer could not be set up, so surround is downmixed instead.
FOUNDATION_EXPORT NSString *const CogAudioOutputSpatialRefusedKey;
// NSNumber (int): the device's I/O buffer, and the frames between Cog
// handing audio over and it being heard, at the render rate.
FOUNDATION_EXPORT NSString *const CogAudioOutputBufferFramesKey;
FOUNDATION_EXPORT NSString *const CogAudioOutputLatencyFramesKey;

FOUNDATION_EXPORT NSString *const CogAudioOutputModificationDSDToPCM;
FOUNDATION_EXPORT NSString *const CogAudioOutputModificationHDCD;
FOUNDATION_EXPORT NSString *const CogAudioOutputModificationPrecision;
FOUNDATION_EXPORT NSString *const CogAudioOutputModificationResampling;
FOUNDATION_EXPORT NSString *const CogAudioOutputModificationTrackGain;
FOUNDATION_EXPORT NSString *const CogAudioOutputModificationTimeStretch;
FOUNDATION_EXPORT NSString *const CogAudioOutputModificationFreeSurround;
FOUNDATION_EXPORT NSString *const CogAudioOutputModificationEqualizer;
FOUNDATION_EXPORT NSString *const CogAudioOutputModificationSpatialAudio;
FOUNDATION_EXPORT NSString *const CogAudioOutputModificationChannelLayout;
FOUNDATION_EXPORT NSString *const CogAudioOutputModificationVolume;

@class SignalMetrics;

@interface AudioPlayer : NSObject {
	double volume;

	NSURL *nextStream;
	id nextStreamUserInfo;
	NSDictionary *nextStreamRGInfo;

	id previousUserInfo; // Track currently last heard track for play counts

	id delegate;

	CogStatus currentPlaybackStatus;
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

// Meters what reaches the output device, for -signalMetrics. Off by
// default; leave it off when nobody is looking.
@property(nonatomic) BOOL meteringEnabled;
// The output since the last call, and the state of the buffers; nil while
// nothing plays. Main thread.
- (SignalMetrics *)signalMetrics;

+ (NSArray *)fileTypes;
+ (NSArray *)schemes;
+ (NSArray *)containerTypes;

@end

@interface AudioPlayer (Private) // Dont use this stuff!

- (void)setPlaybackStatus:(CogStatus)status waitUntilDone:(BOOL)wait;
- (void)setPlaybackStatus:(CogStatus)s;

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
