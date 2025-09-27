#import "PlaybackController.h"
#import "CogAudio/Helper.h"
#import "CogAudio/Status.h"
#import "PlaylistView.h"
#import <Foundation/NSTimer.h>

#import "PlaylistController.h"
#import "PlaylistEntry.h"
#import "PlaylistLoader.h"

#import "MainWindow.h"
#import "MiniWindow.h"

#import <MediaPlayer/MPMediaItem.h>
#import <MediaPlayer/MPNowPlayingInfoCenter.h>
#import <MediaPlayer/MPRemoteCommand.h>
#import <MediaPlayer/MPRemoteCommandCenter.h>
#import <MediaPlayer/MPRemoteCommandEvent.h>

#import "Logging.h"

@import Sentry;

// Sentry captureMessage is too spammy to use for anything but actual errors

extern BOOL kAppControllerShuttingDown;

static inline void dispatch_async_or_reentrant(dispatch_queue_t queue, dispatch_block_t block) {
	if(dispatch_queue_get_label(queue) == dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL)) {
		block();
	} else {
		dispatch_async(queue, block);
	}
}

@implementation NSObject (NxAdditions)

#if 0
-(void)performSelectorInBackground:(SEL)selector withObjects:(id)object, ...
{
	NSMethodSignature *signature = [self methodSignatureForSelector:selector];

	// Setup the invocation
	NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:signature];
	invocation.target = self;
	invocation.selector = selector;

	// Associate the arguments
	va_list objects;
	va_start(objects, object);
	unsigned int objectCounter = 2;
	for (id obj = object; obj != nil; obj = va_arg(objects, id))
	{
		[invocation setArgument:&obj atIndex:objectCounter++];
	}
	va_end(objects);

	// Make sure to invoke on a background queue
	NSInvocationOperation *operation = [[NSInvocationOperation alloc] initWithInvocation:invocation];
	NSOperationQueue *backgroundQueue = [[NSOperationQueue alloc] init];
	[backgroundQueue addOperation:operation];
}
#endif

-(void)performSelectorOnMainThread:(SEL)selector withObjects:(id)object, ...
{
	NSMethodSignature *signature = [self methodSignatureForSelector:selector];

	// Setup the invocation
	NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:signature];
	invocation.target = self;
	invocation.selector = selector;

	// Associate the arguments
	va_list objects;
	va_start(objects, object);
	unsigned int objectCounter = 2;
	for (id obj = object; obj != nil; obj = va_arg(objects, id))
	{
		[invocation setArgument:&obj atIndex:objectCounter++];
	}
	va_end(objects);

	// Invoke on the main operation queue
	NSInvocationOperation *operation = [[NSInvocationOperation alloc] initWithInvocation:invocation];
	NSOperationQueue *mainQueue = [NSOperationQueue mainQueue];
	[mainQueue addOperation:operation];
}

@end

@interface FadeTimerData : NSObject {
	double originalVolume;
	int interval;
	BOOL faded;
}
@property double originalVolume;
@property int interval;
@property BOOL faded;
@end

@implementation FadeTimerData
@synthesize originalVolume;
@synthesize interval;
@synthesize faded;
@end

@implementation PlaybackController

#define DEFAULT_SEEK 5

NSString *CogPlaybackDidBeginNotificiation = @"CogPlaybackDidBeginNotificiation";
NSString *CogPlaybackDidPauseNotificiation = @"CogPlaybackDidPauseNotificiation";
NSString *CogPlaybackDidResumeNotificiation = @"CogPlaybackDidResumeNotificiation";
NSString *CogPlaybackDidStopNotificiation = @"CogPlaybackDidStopNotificiation";

NSString *CogPlaybackDidPrebufferNotification = @"CogPlaybackDidPrebufferNotification";

@synthesize playbackStatus;

@synthesize progressOverall;
@synthesize progressJob;

+ (NSSet *)keyPathsForValuesAffectingSeekable {
	return [NSSet setWithObjects:@"playlistController.currentEntry", @"playlistController.currentEntry.seekable", nil];
}

- (id)init {
	self = [super init];
	if(self) {
		[self initDefaults];

		seekable = NO;
		fading = NO;

		progressOverall = nil;
		progressJob = nil;

		audioPlayer = [[AudioPlayer alloc] init];
		[audioPlayer setDelegate:self];
		[self setPlaybackStatus:CogStatusStopped];
	}

	return self;
}

- (void)initDefaults {
	NSDictionary *defaultsDictionary = @{ @"volume": @(75.0),
		                                  @"pitch": @(1.0),
		                                  @"tempo": @(1.0),
		                                  @"speedLock": @(YES),
		                                  @"GraphicEQenable": @(NO),
		                                  @"GraphicEQpreset": @(-1),
		                                  @"GraphicEQtrackgenre": @(NO),
		                                  @"volumeLimit": @(YES),
		                                  @"enableHrtf": @(NO),
		                                  @"enableHeadTracking": @(NO),
		                                  @"enableHDCD": @(NO),
		                                  @"rubberbandEngine": @"disabled",
		                                  @"rubberbandTransients": @"crisp",
		                                  @"rubberbandDetector": @"compound",
		                                  @"rubberbandPhase": @"laminar",
		                                  @"rubberbandWindow": @"standard",
		                                  @"rubberbandSmoothing": @"off",
		                                  @"rubberbandFormant": @"shifted",
		                                  @"rubberbandPitch": @"highspeed",
		                                  @"rubberbandChannels": @"apart"
	};

	[[NSUserDefaults standardUserDefaults] registerDefaults:defaultsDictionary];
}

static double speedScale(double input, double min, double max) {
	input = (input - min) * 100.0 / (max - min);
	return ((input * input) * (5.0 - 0.2) / 10000.0) + 0.2;
}

static double reverseSpeedScale(double input, double min, double max) {
	input = sqrtf((input - 0.2) * 10000.0 / (5.0 - 0.2));
	return (input * (max - min) / 100.0) + min;
}

- (void)snapSpeeds {
	double pitch = [[NSUserDefaults standardUserDefaults] doubleForKey:@"pitch"];
	double tempo = [[NSUserDefaults standardUserDefaults] doubleForKey:@"tempo"];
	if(fabs(pitch - 1.0) < 1e-6) {
		[[NSUserDefaults standardUserDefaults] setDouble:1.0 forKey:@"pitch"];
	}
	if(fabs(tempo - 1.0) < 1e-6) {
		[[NSUserDefaults standardUserDefaults] setDouble:1.0 forKey:@"tempo"];
	}
}

- (void)awakeFromNib {
	BOOL volumeLimit = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"volumeLimit"];
	const double MAX_VOLUME = (volumeLimit) ? 100.0 : 800.0;

	double volume = [[NSUserDefaults standardUserDefaults] doubleForKey:@"volume"];

	[volumeSlider setDoubleValue:logarithmicToLinear(volume, MAX_VOLUME)];
	[audioPlayer setVolume:volume];

	double pitch = [[NSUserDefaults standardUserDefaults] doubleForKey:@"pitch"];
	[pitchSlider setDoubleValue:reverseSpeedScale(pitch, [pitchSlider minValue], [pitchSlider maxValue])];
	double tempo = [[NSUserDefaults standardUserDefaults] doubleForKey:@"tempo"];
	[tempoSlider setDoubleValue:reverseSpeedScale(tempo, [tempoSlider minValue], [tempoSlider maxValue])];

	[self snapSpeeds];

	BOOL speedLock = [[NSUserDefaults standardUserDefaults] boolForKey:@"speedLock"];
	[lockButton setTitle:speedLock ? @"🔒" : @"🔓"];

	[self setSeekable:NO];
}

- (IBAction)playPauseResume:(id)sender {
	if(playbackStatus == CogStatusStopped || playbackStatus == CogStatusStopping) {
		[self play:self];
	} else {
		[self pauseResume:self];
	}

	[self sendMetaData];
}

- (IBAction)pauseResume:(id)sender {
	if(playbackStatus == CogStatusPaused)
		[self resume:self];
	else
		[self pause:self];
}

- (IBAction)pause:(id)sender {
	if(![self seekable]) {
		[self stop:sender];
		return;
	}

	[[NSUserDefaults standardUserDefaults] setInteger:CogStatusPaused forKey:@"lastPlaybackStatus"];

	[audioPlayer pause];
	[self setPlaybackStatus:CogStatusPaused];

	[self sendMetaData];
}

- (IBAction)resume:(id)sender {
	[[NSUserDefaults standardUserDefaults] setInteger:CogStatusPlaying forKey:@"lastPlaybackStatus"];

	[audioPlayer resume];
}

- (IBAction)stop:(id)sender {
	[[NSUserDefaults standardUserDefaults] setInteger:CogStatusStopped forKey:@"lastPlaybackStatus"];

	[self audioPlayer:audioPlayer removeEqualizer:_eq];

	[audioPlayer stop];

	[self sendMetaData];
}

// called by double-clicking on table
- (void)playEntryAtIndex:(NSInteger)i {
	[self playEntryAtIndex:i startPaused:NO andSeekTo:@(0.0)];
}

- (void)playEntryAtIndex:(NSInteger)i startPaused:(BOOL)paused {
	PlaylistEntry *pe = [playlistController entryAtIndex:i];

	[self playEntry:pe startPaused:paused andSeekTo:@(0.0)];
}

- (void)playEntryAtIndex:(NSInteger)i startPaused:(BOOL)paused andSeekTo:(id)offset {
	PlaylistEntry *pe = [playlistController entryAtIndex:i];

	[self playEntry:pe startPaused:paused andSeekTo:offset];
}

- (IBAction)play:(id)sender {
	if([playlistController shuffle] != ShuffleOff) {
		PlaylistEntry *pe = nil;

		NSInteger index = [playlistView selectedRow];
		if(index != -1) pe = [playlistController entryAtIndex:index];

		pe = [playlistController shuffledEntryAtIndex:(pe ? pe.shuffleIndex : 0)];
		[self playEntryAtIndex:pe.index];
	} else {
		if([playlistView selectedRow] == -1)
			[playlistView selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];

		if([playlistView selectedRow] > -1)
			[self playEntryAtIndex:(int)[playlistView selectedRow]];
	}
}

NSDictionary *makeRGInfo(PlaylistEntry *pe) {
	NSMutableDictionary *dictionary = [NSMutableDictionary dictionary];
	if(pe.replayGainAlbumGain != 0)
		[dictionary setObject:@(pe.replayGainAlbumGain) forKey:@"replayGainAlbumGain"];
	if(pe.replayGainAlbumPeak != 0)
		[dictionary setObject:@(pe.replayGainAlbumPeak) forKey:@"replayGainAlbumPeak"];
	if(pe.replayGainTrackGain != 0)
		[dictionary setObject:@(pe.replayGainTrackGain) forKey:@"replayGainTrackGain"];
	if(pe.replayGainTrackPeak != 0)
		[dictionary setObject:@(pe.replayGainTrackPeak) forKey:@"replayGainTrackPeak"];
	if(pe.volume != 1)
		[dictionary setObject:@(pe.volume) forKey:@"volume"];
	return dictionary;
}

- (void)playEntry:(PlaylistEntry *)pe {
	[self playEntry:pe startPaused:NO andSeekTo:@(0.0)];
}

- (void)playEntry:(PlaylistEntry *)pe startPaused:(BOOL)paused {
	[self playEntry:pe startPaused:paused andSeekTo:@(0.0)];
}

- (void)playEntry:(PlaylistEntry *)pe startPaused:(BOOL)paused andSeekTo:(id)offset {
	if(playbackStatus != CogStatusStopped && playbackStatus != CogStatusStopping)
		[self stop:self];

	if(!pe.url) {
		pe.error = YES;
		pe.errorMessage = NSLocalizedStringFromTableInBundle(@"ErrorMessageBadFile", nil, [NSBundle bundleForClass:[self class]], @"");
		[SentrySDK captureMessage:@"Attempted to play a bad file with no URL"];
		return;
	}

	//[SentrySDK captureMessage:[NSString stringWithFormat:@"Playing track: %@", pe.url]];

	DLog(@"PLAYLIST CONTROLLER: %@", [playlistController class]);
	[playlistController setCurrentEntry:pe];

	lastPosition = -10;

	[self setPosition:[offset doubleValue]];

	if(pe == nil)
		return;

	BOOL loadData = YES;
	NSString *urlScheme = [pe.url scheme];
	if([urlScheme isEqualToString:@"http"] ||
	   [urlScheme isEqualToString:@"https"])
		loadData = NO;

#if 0
	// Race here, but the worst that could happen is we re-read the data
    if([pe metadataLoaded] != YES) {
		[pe performSelectorOnMainThread:@selector(setMetadata:) withObject:[playlistLoader readEntryInfo:pe] waitUntilDone:YES];
	}
#elif 0
	// Racing with this version is less likely to jam up the main thread
	if([pe metadataLoaded] != YES) {
		NSArray *entries = @[pe];
		[playlistLoader loadInfoForEntries:entries];
	}
#else
	// Let's do it this way instead
	if([pe metadataLoaded] != YES && loadData == YES) {
		NSArray *entries = @[pe];
		[playlistLoader performSelectorInBackground:@selector(loadInfoForEntries:) withObject:entries];
	}
#endif

	[self sendMetaData];

	double seekTime = pe.seekable ? [offset doubleValue] : 0.0;

	[audioPlayer performSelectorOnMainThread:@selector(playBG:withUserInfo:withRGInfo:startPaused:andSeekTo:) withObjects:pe.url, pe, makeRGInfo(pe), @(paused), @(seekTime), nil];
}

- (IBAction)next:(id)sender {
	if([playlistController next] == NO)
		return;

	[self playEntry:[playlistController currentEntry]];
}

- (IBAction)prev:(id)sender {
	double pos = [audioPlayer amountPlayed];

	if(pos < 5.0) {
		if([playlistController prev] == NO)
			return;
	}

	[self playEntry:[playlistController currentEntry]];
}

- (void)updatePosition:(id)sender {
	double pos = [audioPlayer amountPlayed];

	[self setPosition:pos];

	if(!kAppControllerShuttingDown) {
		PlaylistEntry *pe = [playlistController currentEntry];
		if(pe) pe.currentPosition = pos;
	}
}

- (IBAction)seek:(id)sender {
	if(![sender respondsToSelector:@selector(doubleValue)]) {
		ALog(@"Someone sent [PlaybackController seek:] a non-seekbar object: %@", sender);
		return;
	}

	double time = [sender doubleValue];

	[audioPlayer performSelectorOnMainThread:@selector(seekToTimeBG:) withObjects:@(time), nil];

	lastPosition = -10;

	[self setPosition:time];

	if(!kAppControllerShuttingDown) {
		PlaylistEntry *pe = [playlistController currentEntry];
		if(pe) pe.currentPosition = time;
	}
}

- (IBAction)seek:(id)sender toTime:(NSTimeInterval)position {
	double time = (double)(position);

	lastPosition = -10;

	[audioPlayer performSelectorOnMainThread:@selector(seekToTimeBG:) withObjects:@(time), nil];

	[self setPosition:time];

	if(!kAppControllerShuttingDown) {
		PlaylistEntry *pe = [playlistController currentEntry];
		if(pe) pe.currentPosition = time;
	}
}

- (IBAction)spam:(id)sender {
	PlaylistEntry *pe = [playlistController currentEntry];
	if(pe) {
		NSPasteboard *pboard = [NSPasteboard generalPasteboard];

		[pboard clearContents];

		[pboard writeObjects:@[[pe spam]]];
	}
}

- (IBAction)eventSeekForward:(id)sender {
	[self seekForward:DEFAULT_SEEK];
}

- (void)seekForward:(double)amount {
	if([self seekable]) {
		double seekTo = [audioPlayer amountPlayed] + amount;
		
		if(seekTo > [[[playlistController currentEntry] length] doubleValue]) {
			[self next:self];
		} else {
			lastPosition = -10;
			[audioPlayer performSelectorOnMainThread:@selector(seekToTimeBG:) withObjects:@(seekTo), nil];
			[self setPosition:seekTo];
		}
	}
}

- (IBAction)eventSeekBackward:(id)sender {
	[self seekBackward:DEFAULT_SEEK];
}

- (void)seekBackward:(double)amount {
	if([self seekable]) {
		double seekTo = [audioPlayer amountPlayed] - amount;
		
		if(seekTo < 0)
			seekTo = 0;
		
		lastPosition = -10;
		
		[audioPlayer performSelectorOnMainThread:@selector(seekToTimeBG:) withObjects:@(seekTo), nil];
		[self setPosition:seekTo];
	}
}

/*
 - (void)changePlayButtonImage:(NSString *)name
{
    NSImage *img = [NSImage imageNamed:name];
//	[img retain];

    if(img == nil)
    {
        DLog(@"Error loading image!");
    }

    [playbackButtons setImage:img forSegment:1];
}
*/
- (IBAction)changeVolume:(id)sender {
	BOOL volumeLimit = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"volumeLimit"];
	const double MAX_VOLUME = (volumeLimit) ? 100.0 : 800.0;

	DLog(@"VOLUME: %lf, %lf", [sender doubleValue], linearToLogarithmic([sender doubleValue], MAX_VOLUME));

	[audioPlayer setVolume:linearToLogarithmic([sender doubleValue], MAX_VOLUME)];

	[[NSUserDefaults standardUserDefaults] setDouble:[audioPlayer volume] forKey:@"volume"];
}

/* selector for NSTimer - gets passed the Timer object itself
 and the appropriate userInfo, which in this case is an NSNumber
 containing the current volume before we start fading. */
- (void)audioFadeDown:(NSTimer *)audioTimer {
	FadeTimerData *data = [audioTimer userInfo];
	double volume = [audioPlayer volume];
	double originalVolume = data.originalVolume;
	double down = originalVolume / 10;

	DLog(@"VOLUME IS %lf", volume);

	if(volume > 0.0001) { // YAY! Roundoff error!
		[audioPlayer volumeDown:down];
	} else if(!data.faded) { // ACK HACK must do this once
		data.faded = YES;
		data.interval = 3; // ACK XXX slightly more than the current pause/seek fade interval, in 100ms steps
		[audioPlayer pause];
	} else { // done the above once, now
		if((data.interval -= 1) <= 0) {
			BOOL volumeLimit = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"volumeLimit"];
			const double MAX_VOLUME = (volumeLimit) ? 100.0 : 800.0;

			[audioPlayer setVolume:originalVolume];
			[volumeSlider setDoubleValue:logarithmicToLinear(originalVolume, MAX_VOLUME)];
			[audioTimer invalidate];

			fading = NO;
		}
	}
}

- (void)audioFadeUp:(NSTimer *)audioTimer {
	double volume = [audioPlayer volume];
	FadeTimerData *data = [audioTimer userInfo];
	double originalVolume = data.originalVolume;
	double up = originalVolume / 10;

	DLog(@"VOLUME IS %lf", volume);

	if(originalVolume - volume > 0.0001) {
		if((volume + up) > originalVolume)
			[audioPlayer volumeUp:(originalVolume - volume)];
		else
			[audioPlayer volumeUp:up];
	} else // volume is at or near original level, we are ready to release the timer and move on
	{
		BOOL volumeLimit = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"volumeLimit"];
		const double MAX_VOLUME = (volumeLimit) ? 100.0 : 800.0;

		[audioPlayer setVolume:originalVolume];
		[volumeSlider setDoubleValue:logarithmicToLinear(originalVolume, MAX_VOLUME)];
		[audioTimer invalidate];

		fading = NO;
	}
}

- (IBAction)fade:(id)sender {
	double time = 0.1;

	// we can not allow multiple fade timers to be registered
	if(YES == fading)
		return;
	fading = YES;

	FadeTimerData *data = [[FadeTimerData alloc] init];
	data.originalVolume = [audioPlayer volume];
	NSTimer *fadeTimer;

	if(playbackStatus == CogStatusPlaying) {
		fadeTimer = [NSTimer timerWithTimeInterval:time
		                                    target:self
		                                  selector:@selector(audioFadeDown:)
		                                  userInfo:data
		                                   repeats:YES];
		[[NSRunLoop currentRunLoop] addTimer:fadeTimer forMode:NSRunLoopCommonModes];
	} else {
		[audioPlayer setVolume:0];
		fadeTimer = [NSTimer timerWithTimeInterval:time
		                                    target:self
		                                  selector:@selector(audioFadeUp:)
		                                  userInfo:data
		                                   repeats:YES];
		[[NSRunLoop currentRunLoop] addTimer:fadeTimer forMode:NSRunLoopCommonModes];
		[self pauseResume:self];
	}
}

- (IBAction)changePitch:(id)sender {
	const double pitch = speedScale([sender doubleValue], [pitchSlider minValue], [pitchSlider maxValue]);
	DLog(@"PITCH: %lf", pitch);

	[[NSUserDefaults standardUserDefaults] setDouble:pitch forKey:@"pitch"];

	if([[NSUserDefaults standardUserDefaults] boolForKey:@"speedLock"]) {
		[[NSUserDefaults standardUserDefaults] setDouble:pitch forKey:@"tempo"];
	}

	[self snapSpeeds];
}

- (IBAction)changeTempo:(id)sender {
	const double tempo = speedScale([sender doubleValue], [tempoSlider minValue], [tempoSlider maxValue]);
	DLog(@"TEMPO: %lf", tempo);

	[[NSUserDefaults standardUserDefaults] setDouble:tempo forKey:@"tempo"];

	if([[NSUserDefaults standardUserDefaults] boolForKey:@"speedLock"]) {
		[[NSUserDefaults standardUserDefaults] setDouble:tempo forKey:@"pitch"];
	}

	[self snapSpeeds];
}

- (IBAction)skipToNextAlbum:(id)sender {
	BOOL found = NO;

	NSInteger index = [[playlistController currentEntry] index];
	NSString *origAlbum = [[playlistController currentEntry] album];

	NSInteger i;
	NSString *curAlbum;

	PlaylistEntry *pe;

	for(i = 1; i < [[playlistController arrangedObjects] count]; i++) {
		pe = [playlistController entryAtIndex:index + i];
		if(pe == nil)
			break;

		curAlbum = [pe album];

		// check for untagged files, and just play the first untagged one
		// we come across
		if(curAlbum == nil) {
			found = YES;
			break;
		}

		if([curAlbum caseInsensitiveCompare:origAlbum] != NSOrderedSame) {
			found = YES;
			break;
		}
	}

	if(found) {
		[self playEntryAtIndex:i + index];
	}
}

- (IBAction)skipToPreviousAlbum:(id)sender {
	BOOL found = NO;
	BOOL foundAlbum = NO;

	NSInteger index = [[playlistController currentEntry] index];
	NSString *origAlbum = [[playlistController currentEntry] album];
	NSString *curAlbum;

	NSInteger i;

	PlaylistEntry *pe;

	for(i = 1; i < [[playlistController arrangedObjects] count]; i++) {
		pe = [playlistController entryAtIndex:index - i];
		if(pe == nil)
			break;

		curAlbum = [pe album];
		if(curAlbum == nil) {
			found = YES;
			break;
		}

		if([curAlbum caseInsensitiveCompare:origAlbum] != NSOrderedSame) {
			if(foundAlbum == NO) {
				foundAlbum = YES;
				// now we need to move up to the first song in the album, so we'll
				// go till we either find index 0, or the first song in the album
				origAlbum = curAlbum;
				continue;
			} else {
				found = YES; // terminate loop
				break;
			}
		}
	}

	if(found || foundAlbum) {
		if(foundAlbum == YES)
			i--;
		[self playEntryAtIndex:index - i];
	}
}

- (IBAction)volumeDown:(id)sender {
	BOOL volumeLimit = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"volumeLimit"];
	const double MAX_VOLUME = (volumeLimit) ? 100.0 : 800.0;

	double newVolume = [audioPlayer volumeDown:DEFAULT_VOLUME_DOWN];
	[volumeSlider setDoubleValue:logarithmicToLinear(newVolume, MAX_VOLUME)];

	[[NSUserDefaults standardUserDefaults] setDouble:[audioPlayer volume] forKey:@"volume"];
}

- (IBAction)volumeUp:(id)sender {
	BOOL volumeLimit = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"volumeLimit"];
	const double MAX_VOLUME = (volumeLimit) ? 100.0 : 800.0;

	double newVolume;
	newVolume = [audioPlayer volumeUp:DEFAULT_VOLUME_UP];
	[volumeSlider setDoubleValue:logarithmicToLinear(newVolume, MAX_VOLUME)];

	[[NSUserDefaults standardUserDefaults] setDouble:[audioPlayer volume] forKey:@"volume"];
}

- (IBAction)pitchDown:(id)sender {
	double pitch = speedScale([pitchSlider doubleValue], [pitchSlider minValue], [pitchSlider maxValue]);
	double newPitch = pitch - DEFAULT_PITCH_DOWN;
	if(newPitch < 0.2) {
		newPitch = 0.2;
	}
	[pitchSlider setDoubleValue:reverseSpeedScale(newPitch, [pitchSlider minValue], [pitchSlider maxValue])];

	[[NSUserDefaults standardUserDefaults] setDouble:newPitch forKey:@"pitch"];

	if([[NSUserDefaults standardUserDefaults] boolForKey:@"speedLock"]) {
		[tempoSlider setDoubleValue:reverseSpeedScale(newPitch, [tempoSlider minValue], [tempoSlider maxValue])];

		[[NSUserDefaults standardUserDefaults] setDouble:newPitch forKey:@"tempo"];
	}
}

- (IBAction)pitchUp:(id)sender {
	double pitch = speedScale([pitchSlider doubleValue], [pitchSlider minValue], [pitchSlider maxValue]);
	double newPitch = pitch + DEFAULT_PITCH_UP;
	if(newPitch > 5.0) {
		newPitch = 5.0;
	}
	[pitchSlider setDoubleValue:reverseSpeedScale(newPitch, [pitchSlider minValue], [pitchSlider maxValue])];

	[[NSUserDefaults standardUserDefaults] setDouble:newPitch forKey:@"pitch"];

	if([[NSUserDefaults standardUserDefaults] boolForKey:@"speedLock"]) {
		[tempoSlider setDoubleValue:reverseSpeedScale(newPitch, [tempoSlider minValue], [tempoSlider maxValue])];

		[[NSUserDefaults standardUserDefaults] setDouble:newPitch forKey:@"tempo"];
	}
}

- (IBAction)tempoDown:(id)sender {
	double tempo = speedScale([tempoSlider doubleValue], [tempoSlider minValue], [tempoSlider maxValue]);
	double newTempo = tempo - DEFAULT_TEMPO_DOWN;
	if(newTempo < 0.2) {
		newTempo = 0.2;
	}
	[tempoSlider setDoubleValue:reverseSpeedScale(newTempo, [tempoSlider minValue], [tempoSlider maxValue])];

	[[NSUserDefaults standardUserDefaults] setDouble:newTempo forKey:@"tempo"];

	if([[NSUserDefaults standardUserDefaults] boolForKey:@"speedLock"]) {
		[pitchSlider setDoubleValue:reverseSpeedScale(newTempo, [pitchSlider minValue], [pitchSlider maxValue])];

		[[NSUserDefaults standardUserDefaults] setDouble:newTempo forKey:@"pitch"];
	}
}

- (IBAction)tempoUp:(id)sender {
	double tempo = speedScale([tempoSlider doubleValue], [tempoSlider minValue], [tempoSlider maxValue]);
	double newTempo = tempo + DEFAULT_TEMPO_UP;
	if(newTempo > 5.0) {
		newTempo = 5.0;
	}
	[tempoSlider setDoubleValue:reverseSpeedScale(newTempo, [tempoSlider minValue], [tempoSlider maxValue])];

	[[NSUserDefaults standardUserDefaults] setDouble:newTempo forKey:@"tempo"];

	if([[NSUserDefaults standardUserDefaults] boolForKey:@"speedLock"]) {
		[pitchSlider setDoubleValue:reverseSpeedScale(newTempo, [pitchSlider minValue], [pitchSlider maxValue])];

		[[NSUserDefaults standardUserDefaults] setDouble:newTempo forKey:@"pitch"];
	}
}

- (void)audioPlayer:(AudioPlayer *)player displayEqualizer:(AudioUnit)eq {
	if(_eq && _eq != eq) {
		[equalizerWindowController setEQ:nil];
	}

	_eq = eq;

	equalizerLoadPreset(eq);

	[equalizerWindowController setEQ:eq];
}

- (void)audioPlayer:(AudioPlayer *)player refreshEqualizer:(AudioUnit)eq {
	equalizerLoadPreset(eq);
}

- (void)audioPlayer:(AudioPlayer *)player removeEqualizer:(AudioUnit)eq {
	if(eq == _eq) {
		[equalizerWindowController setEQ:nil];

		_eq = nil;
	}
}

- (void)audioPlayer:(AudioPlayer *)player willEndStream:(id)userInfo {
	PlaylistEntry *curEntry = (PlaylistEntry *)userInfo;
	PlaylistEntry *pe;

	if(curEntry.stopAfter)
		pe = nil;
	else {
		pe = [playlistController getNextEntry:curEntry];
		if(pe && [pe metadataLoaded] != YES) {
			NSArray *entries = @[pe];
			[playlistLoader performSelectorInBackground:@selector(loadInfoForEntries:) withObject:entries];
		}
	}

	if(pe && pe.url) {
		//[SentrySDK captureMessage:[NSString stringWithFormat:@"Beginning decoding track: %@", pe.url]];
		[player setNextStream:pe.url withUserInfo:pe withRGInfo:makeRGInfo(pe)];
	} else if(pe) {
		[SentrySDK captureMessage:@"Invalid playlist entry reached"];
		[player setNextStream:nil];
		pe.error = YES;
		pe.errorMessage = NSLocalizedStringFromTableInBundle(@"ErrorMessageBadFile", nil, [NSBundle bundleForClass:[self class]], @"");
	} else {
		//[SentrySDK captureMessage:@"End of playlist reached"];
		[player setNextStream:nil];
	}
}

- (void)audioPlayer:(AudioPlayer *)player didBeginStream:(id)userInfo {
	PlaylistEntry *pe = (PlaylistEntry *)userInfo;

	// Dispatch to main thread, unless this is the main thread
	dispatch_async_or_reentrant(dispatch_get_main_queue(), ^{
		[self->playlistController setCurrentEntry:pe];

		if(pe && self->_eq) {
			equalizerApplyGenre(self->_eq, [pe genre]);
		}

		self->lastPosition = -10;

		[self setPosition:0];
	});

	if(pe) {
		[[NSNotificationCenter defaultCenter] postNotificationName:CogPlaybackDidBeginNotificiation object:pe];
	}
}

- (void)audioPlayer:(AudioPlayer *)player didChangeStatus:(NSNumber *)s userInfo:(id)userInfo {
	int status = [s intValue];
	if(status == CogStatusStopped || status == CogStatusPaused) {
		if(positionTimer) {
			[positionTimer invalidate];
			positionTimer = NULL;
		}

		if(status == CogStatusStopped) {
			//[SentrySDK captureMessage:@"Playback stopped"];

			[self setPosition:0];
			[self setSeekable:NO]; // the player stopped, disable the slider

			[[NSNotificationCenter defaultCenter] postNotificationName:CogPlaybackDidStopNotificiation object:nil];
		} else // paused
		{
			//[SentrySDK captureMessage:@"Playback paused"];
			[[NSNotificationCenter defaultCenter] postNotificationName:CogPlaybackDidPauseNotificiation object:nil];
		}
	} else if(status == CogStatusPlaying) {
		//[SentrySDK captureMessage:@"Playback started"];

		if(!positionTimer) {
			positionTimer = [NSTimer timerWithTimeInterval:0.2 target:self selector:@selector(updatePosition:) userInfo:nil repeats:YES];
			[[NSRunLoop currentRunLoop] addTimer:positionTimer forMode:NSRunLoopCommonModes];
		}

		[[NSNotificationCenter defaultCenter] postNotificationName:CogPlaybackDidResumeNotificiation object:nil];
	}

	if(status == CogStatusStopped) {
		[playlistController setCurrentEntry:nil];
		[self setSeekable:NO]; // the player stopped, disable the slider
	} else {
		[self setSeekable:YES];
	}
	switch(status) {
		case CogStatusPlaying:
			DLog(@"PLAYING!");
			break;
		case CogStatusPaused:
			DLog(@"PAUSED!");
			break;

		default:
			DLog(@"STOPPED!");
			break;
	}

	[self setPlaybackStatus:status];
	// If we don't send it here, if we've stopped, then the NPIC will be stuck at the last file we played.
	[self sendMetaData];
}

- (void)audioPlayer:(AudioPlayer *)player didStopNaturally:(id)userInfo {
	if([[NSUserDefaults standardUserDefaults] boolForKey:@"quitOnNaturalStop"]) {
		//[SentrySDK captureMessage:@"Playback stopped naturally, terminating app"];
		[NSApp terminate:nil];
	}
}

- (void)audioPlayer:(AudioPlayer *)player restartPlaybackAtCurrentPosition:(id)userInfo {
	PlaylistEntry *pe = [playlistController currentEntry];
	BOOL paused = playbackStatus == CogStatusPaused;
	//[SentrySDK captureMessage:[NSString stringWithFormat:@"Playback restarting for track: %@", pe.url]];
	[player performSelectorOnMainThread:@selector(playBG:withUserInfo:withRGInfo:startPaused:andSeekTo:) withObjects:pe.url, pe, makeRGInfo(pe), @(paused), @(pe.seekable ? pe.currentPosition : 0.0), nil];
}

- (void)audioPlayer:(AudioPlayer *)player pushInfo:(NSDictionary *)info toTrack:(id)userInfo {
	PlaylistEntry *pe = (PlaylistEntry *)userInfo;
	if(!pe) pe = [playlistController currentEntry];
	[pe setMetadata:info];
	[playlistView refreshTrack:pe];
	// Dispatch async to main thread, unless this is the main thread
	dispatch_async_or_reentrant(dispatch_get_main_queue(), ^{
		self->playlistController.currentEntry = pe;
		[self sendMetaData];
		[[NSNotificationCenter defaultCenter] postNotificationName:CogPlaybackDidBeginNotificiation object:pe];
	});
}

- (void)audioPlayer:(AudioPlayer *)player reportPlayCountForTrack:(id)userInfo {
	if(userInfo) {
		PlaylistEntry *pe = (PlaylistEntry *)userInfo;
		[playlistController updatePlayCountForTrack:pe];
	}
}

- (void)audioPlayer:(AudioPlayer *)player updatePosition:(id)userInfo {
	if(userInfo) {
		PlaylistEntry *pe = (PlaylistEntry *)userInfo;
		if([pe current]) {
			[self updatePosition:userInfo];
		}
	}
}

- (void)audioPlayer:(AudioPlayer *)player setError:(NSNumber *)status toTrack:(id)userInfo {
	PlaylistEntry *pe = (PlaylistEntry *)userInfo;
	[pe setError:[status boolValue]];
}

- (void)playlistDidChange:(PlaylistController *)p {
	[audioPlayer resetNextStreams];
}

- (void)setPosition:(double)p {
	position = p;

	if(kAppControllerShuttingDown) return;

	PlaylistEntry *pe = [playlistController currentEntry];
	if(pe) pe.currentPosition = p;

	if(p > lastPosition && (p - lastPosition) >= 10.0) {
		[[NSUserDefaults standardUserDefaults] setInteger:CogStatusPlaying forKey:@"lastPlaybackStatus"];

		// If we handle this here, then it will send on all seek operations, which also reset lastPosition
		[self sendMetaData];

		lastPosition = p;

		[playlistController commitPersistentStore];
	}
}

- (double)position {
	return position;
}

- (void)setSeekable:(BOOL)s {
	seekable = s;
}

- (BOOL)seekable {
	return seekable && [[playlistController currentEntry] seekable];
}

- (void)sendMetaData {
	MPNowPlayingInfoCenter *defaultCenter = [MPNowPlayingInfoCenter defaultCenter];

	PlaylistEntry *entry = [playlistController currentEntry];
	NSMutableDictionary *songInfo = [[NSMutableDictionary alloc] init];

	if(entry) {
		if(entry.title && [entry.title length])
			[songInfo setObject:entry.title forKey:MPMediaItemPropertyTitle];
		if(entry.artist && [entry.artist length])
			[songInfo setObject:entry.artist forKey:MPMediaItemPropertyArtist];
		if(entry.album && [entry.album length])
			[songInfo setObject:entry.album forKey:MPMediaItemPropertyAlbumTitle];
		if(entry.albumArt) {
			// can't do && with @available
			if(@available(macOS 10.13.2, *)) {
				CGSize artworkSize = CGSizeMake(500, 500);
				MPMediaItemArtwork *mpArtwork = [[MPMediaItemArtwork alloc] initWithBoundsSize:artworkSize
				                                                                requestHandler:^NSImage *_Nonnull(CGSize size) {
					                                                                return entry.albumArt;
				                                                                }];
				[songInfo setObject:mpArtwork forKey:MPMediaItemPropertyArtwork];
			}
		}
		// I don't know what NPIC does with these since they aren't exposed in UI, but if we have them, use it.
		// There's a bunch of other metadata, but PlaylistEntry can't represent a lot of it.
		if(entry.genre && [entry.genre length])
			[songInfo setObject:entry.genre forKey:MPMediaItemPropertyGenre];
		if(entry.year) {
			// If PlaylistEntry can represent a full date like some tag formats can do, change it
			NSCalendar *calendar = [NSCalendar currentCalendar];
			NSDate *releaseYear = [calendar dateWithEra:1 year:entry.year month:1 day:1 hour:0 minute:0 second:0 nanosecond:0];
			if(releaseYear) {
				[songInfo setObject:releaseYear forKey:MPMediaItemPropertyReleaseDate];
			}
		}
		[songInfo setObject:@(entry.currentPosition) forKey:MPNowPlayingInfoPropertyElapsedPlaybackTime];
		[songInfo setObject:entry.length forKey:MPMediaItemPropertyPlaybackDuration];
		[songInfo setObject:@(entry.index) forKey:MPMediaItemPropertyPersistentID];
	}

	switch(playbackStatus) {
		case CogStatusPlaying:
			defaultCenter.playbackState = MPNowPlayingPlaybackStatePlaying;
			break;
		case CogStatusPaused:
			defaultCenter.playbackState = MPNowPlayingPlaybackStatePaused;
			break;
		default:
			defaultCenter.playbackState = MPNowPlayingPlaybackStateStopped;
			break;
	}

	[defaultCenter setNowPlayingInfo:songInfo];
}

- (BOOL)isError {
	if(playlistController) {
		PlaylistEntry *pe = [playlistController currentEntry];
		if(pe && [pe error])
			return YES;
	}
	return NO;
}

@end
