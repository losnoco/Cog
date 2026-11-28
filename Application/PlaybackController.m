#import "PlaybackController.h"
#import "Cog-Swift.h"
#import "CogAudio/Helper.h"
#import "CogAudio/Status.h"
#import "PlaylistView.h"
#import <Foundation/NSTimer.h>

#import "PlaylistController.h"
#import "PlaylistEntry.h"
#import "PlaylistLoader.h"

#import "MainWindow.h"

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
	NSOperationQueue *backgroundQueue = [NSOperationQueue new];
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

	// Invoke from the main run loop, not the main queue: stopping playback
	// waits for the feeder while turning the run loop, and a feeder that
	// dispatches to the main queue meanwhile (SandboxBroker does, opening a
	// file) is only served if this is not itself running as a main queue
	// block.
	[invocation retainArguments];
	[invocation performSelectorOnMainThread:@selector(invoke) withObject:nil waitUntilDone:NO];
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

@interface PlaybackController ()
- (void)setVolumeControlsValue:(double)value;
@end

static void *kVolumeDefaultContext = &kVolumeDefaultContext;

@implementation PlaybackController {
	// Bumped by every play or stop, so a start deferred until an entry's
	// tags load is dropped if something else happened meanwhile.
	NSUInteger startToken;
	// The entry last handed to the player, and where it starts, so that the
	// position shows that offset, rather than 0, once the stream begins.
	PlaylistEntry *startingEntry;
	double startingPosition;
}

#define DEFAULT_SEEK 5

NSNotificationName CogPlaybackDidBeginNotificiation = @"CogPlaybackDidBeginNotificiation";
NSNotificationName CogPlaybackDidPauseNotificiation = @"CogPlaybackDidPauseNotificiation";
NSNotificationName CogPlaybackDidResumeNotificiation = @"CogPlaybackDidResumeNotificiation";
NSNotificationName CogPlaybackDidStopNotificiation = @"CogPlaybackDidStopNotificiation";
NSNotificationName CogPlaybackWillResetPositionNotification = @"CogPlaybackWillResetPositionNotification";

NSNotificationName CogPlaybackDidPrebufferNotification = @"CogPlaybackDidPrebufferNotification";

@synthesize playbackStatus;

@synthesize progressOverall;
@synthesize progressJob;
@synthesize progressMetadata;

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
		progressMetadata = nil;

		audioPlayer = [AudioPlayer new];
		[audioPlayer setDelegate:self];
		[self setPlaybackStatus:CogStatusStopped];

		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(entryMetadataLoaded:) name:CogPlaylistEntryMetadataLoadedNotification object:nil];
		// The volume set from outside the app (defaults write), which was
		// only read at launch.
		[[NSUserDefaults standardUserDefaults] addObserver:self forKeyPath:@"volume" options:0 context:kVolumeDefaultContext];
	}

	return self;
}

// A track can start before its tags have loaded, and so without its
// ReplayGain; hand the gain to the player once they arrive. The player
// ignores entries it is not playing or holding queued.
- (void)entryMetadataLoaded:(NSNotification *)notification {
	PlaylistEntry *pe = notification.object;
	dispatch_async_or_reentrant(dispatch_get_main_queue(), ^{
		if(self->playbackStatus == CogStatusStopped) return;
		[self->audioPlayer setRGInfo:makeRGInfo(pe) forTrack:pe];
	});
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context {
	if(context != kVolumeDefaultContext) {
		[super observeValueForKeyPath:keyPath ofObject:object change:change context:context];
		return;
	}
	dispatch_async(dispatch_get_main_queue(), ^{
		BOOL volumeLimit = [[NSUserDefaults standardUserDefaults] boolForKey:@"volumeLimit"];
		const double MAX_VOLUME = (volumeLimit) ? 100.0 : 800.0;
		double volume = MAX(0.0, MIN([[NSUserDefaults standardUserDefaults] doubleForKey:@"volume"], MAX_VOLUME));
		// What the app itself just saved is already playing.
		if(fabs(volume - [self->audioPlayer volume]) < 1e-6) return;
		[self->audioPlayer setVolume:volume];
		[self setVolumeControlsValue:logarithmicToLinear(volume, MAX_VOLUME)];
	});
}

- (void)initDefaults {
	NSDictionary *defaultsDictionary = @{ @"volume": @(75.0),
		                                  @"pitch": @(1.0),
		                                  @"tempo": @(1.0),
		                                  @"speedLock": @YES,
		                                  @"GraphicEQenable": @NO,
		                                  @"GraphicEQpreset": @(-1),
		                                  @"GraphicEQtrackgenre": @NO,
		                                  @"volumeLimit": @YES,
		                                  @"volumeStep": @(5.0),
		                                  @"volumeStepMode": @"volume",
		                                  @"enableSpatialAudio": @YES,
		                                  @"enableHeadTracking": @NO,
		                                  @"enableHDCD": @NO,
		                                  @"rubberbandEngine": @"varispeed",
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

// Within the speed sliders' range; locked, the other follows, as the speed
// popover's sliders do.
static void setSpeed(NSString *key, NSString *lockedKey, double speed) {
	speed = MIN(MAX(speed, 0.2), 5.0);
	[[NSUserDefaults standardUserDefaults] setDouble:speed forKey:key];
	if([[NSUserDefaults standardUserDefaults] boolForKey:@"speedLock"]) {
		[[NSUserDefaults standardUserDefaults] setDouble:speed forKey:lockedKey];
	}
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

	[self setVolumeControlsValue:logarithmicToLinear(volume, MAX_VOLUME)];
	[audioPlayer setVolume:volume];

	[self snapSpeeds];

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
	++startToken;
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
		// The selection, or where shuffle starts.
		NSInteger index = [playlistView selectedRow];
		PlaylistEntry *pe = index != -1 ? [playlistController entryAtIndex:index] : [playlistController firstEntryToPlay];
		if(pe) [self playEntry:pe];
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
	if(pe.soundcheck && pe.soundcheck.length != 0)
		[dictionary setObject:@(pe.soundcheckVolume) forKey:@"soundcheck"];
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
	if(!pe.url) {
		if(playbackStatus != CogStatusStopped && playbackStatus != CogStatusStopping)
			[self stop:self];
		pe.error = YES;
		pe.errorMessage = NSLocalizedStringFromTableInBundle(@"ErrorMessageBadFile", nil, [NSBundle bundleForClass:[self class]], @"");
		[SentrySDK captureMessage:[NSString stringWithFormat:@"Attempted to play a bad file with no URL: %@", pe.urlString]];
		return;
	}

	//[SentrySDK captureMessage:[NSString stringWithFormat:@"Playing track: %@", pe.url]];

	// Not stopped first: the player moves to the new entry itself, in place
	// while playing, and keeps an output device it holds exclusively.
	// Stopping gave the device back and took it again, reconfiguring it
	// twice (0.8 to 1.4 s of silence on a USB DAC). What plays goes on
	// until the player moves, and the position it reports until then is
	// not this entry's.
	[positionTimer invalidate];
	positionTimer = nil;

	DLog(@"PLAYLIST CONTROLLER: %@", [playlistController class]);
	[playlistController setCurrentEntry:pe];

	lastPosition = -10;

	double startPosition = [offset doubleValue];
	if(startPosition <= 0.0) {
		[[NSNotificationCenter defaultCenter] postNotificationName:CogPlaybackWillResetPositionNotification object:self];
	}
	[self setPosition:startPosition];

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
		// Start once the tags are in: the ReplayGain and whether the track can
		// be resumed part-way come from them. Starting without them played
		// the track at the wrong level, and from the top, until they arrived.
		// A file whose tags never load still starts, after a moment.
		NSUInteger token = ++startToken;
		__block id observer = nil;
		void (^start)(void) = ^{
			if(observer) {
				[[NSNotificationCenter defaultCenter] removeObserver:observer];
				observer = nil;
			}
			if(token != self->startToken) return;
			++self->startToken;
			[self startPlayingEntry:pe startPaused:paused andSeekTo:offset];
		};
		observer = [[NSNotificationCenter defaultCenter] addObserverForName:CogPlaylistEntryMetadataLoadedNotification
		                                                             object:pe
		                                                              queue:[NSOperationQueue mainQueue]
		                                                         usingBlock:^(NSNotification *notification) {
			start();
		}];
		dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 2 * NSEC_PER_SEC), dispatch_get_main_queue(), start);

		NSArray *entries = @[pe];
		[playlistLoader performSelectorInBackground:@selector(loadInfoForEntries:) withObject:entries];
		return;
	}
#endif

	++startToken;
	[self startPlayingEntry:pe startPaused:paused andSeekTo:offset];
}

- (void)startPlayingEntry:(PlaylistEntry *)pe startPaused:(BOOL)paused andSeekTo:(id)offset {
	[self sendMetaData];

	double seekTime = pe.seekable ? [offset doubleValue] : 0.0;
	startingEntry = pe;
	startingPosition = seekTime;

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

// Asks for a position in the playing track, with where it is filled in (to
// note down, or to change), and seeks there. A position past the track's end,
// or that isn't one, is refused with a beep.
- (IBAction)jumpToTime:(id)sender {
	if(![playlistController currentEntry] || ![self seekable]) {
		NSBeep();
		return;
	}

	NSAlert *alert = [NSAlert new];
	alert.messageText = NSLocalizedString(@"Jump to Time", @"Jump to Time prompt: title");
	alert.informativeText = NSLocalizedString(@"Enter a position in the playing track, as hours:minutes:seconds.", @"Jump to Time prompt: explanation");
	[alert addButtonWithTitle:NSLocalizedString(@"Jump", @"Jump to Time prompt: the button that seeks")];
	[alert addButtonWithTitle:NSLocalizedString(@"Cancel", @"")];

	NSTextField *field = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 220, 22)];
	field.stringValue = [[SecondsFormatter new] stringForObjectValue:@([self position])] ?: @"";
	field.alignment = NSTextAlignmentCenter;
	field.font = [NSFont monospacedDigitSystemFontOfSize:NSFont.systemFontSize weight:NSFontWeightRegular];
	alert.accessoryView = field;
	alert.window.initialFirstResponder = field;

	void (^seek)(NSModalResponse) = ^(NSModalResponse response) {
		if(response != NSAlertFirstButtonReturn) return;
		// The track may have changed while the prompt was up.
		PlaylistEntry *pe = [self->playlistController currentEntry];
		NSTimeInterval time = [PlaybackController secondsFromTimeString:field.stringValue];
		double length = [[pe length] doubleValue];
		if(!pe || ![self seekable] || time < 0 || (length > 0 && time > length)) {
			NSBeep();
			return;
		}
		[self seek:self toTime:time];
	};

	NSWindow *window = [NSApp mainWindow];
	if(window) {
		[alert beginSheetModalForWindow:window completionHandler:seek];
	} else {
		seek([alert runModal]);
	}
}

// Seconds, minutes:seconds or hours:minutes:seconds, the seconds with a
// fraction if wanted; -1 for anything else.
+ (NSTimeInterval)secondsFromTimeString:(NSString *)string {
	NSString *trimmed = [string stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
	NSArray<NSString *> *parts = [trimmed componentsSeparatedByString:@":"];
	if(!trimmed.length || parts.count > 3) return -1;

	NSCharacterSet *notDigits = [[NSCharacterSet characterSetWithCharactersInString:@"0123456789"] invertedSet];
	NSTimeInterval seconds = 0;
	for(NSUInteger i = 0; i < parts.count; i++) {
		NSString *part = parts[i];
		BOOL last = (i == parts.count - 1);
		// Only the seconds may have a fraction, after a period or a comma.
		NSString *whole = part, *fraction = @"";
		if(last) {
			NSRange mark = [part rangeOfCharacterFromSet:[NSCharacterSet characterSetWithCharactersInString:@".,"]];
			if(mark.location != NSNotFound) {
				whole = [part substringToIndex:mark.location];
				fraction = [part substringFromIndex:mark.location + 1];
			}
		}
		if(!whole.length && !fraction.length) return -1;
		if([whole rangeOfCharacterFromSet:notDigits].location != NSNotFound ||
		   [fraction rangeOfCharacterFromSet:notDigits].location != NSNotFound) return -1;
		double value = [[NSString stringWithFormat:@"%@.%@", whole.length ? whole : @"0", fraction.length ? fraction : @"0"] doubleValue];
		seconds = seconds * 60.0 + value;
	}
	return seconds;
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

	[self setVolumeControlsValue:[sender doubleValue]];
	[audioPlayer setVolume:linearToLogarithmic([sender doubleValue], MAX_VOLUME)];

	[[NSUserDefaults standardUserDefaults] setDouble:[audioPlayer volume] forKey:@"volume"];
}

- (void)setVolumeControlsValue:(double)value {
	[volumeSlider setDoubleValue:value];
	[miniVolumeSlider setDoubleValue:value];
	[miniPlusVolumeSlider setDoubleValue:value];
}

- (AudioPlayer *)audioPlayer {
	return audioPlayer;
}

- (double)volume {
	BOOL volumeLimit = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"volumeLimit"];
	const double MAX_VOLUME = (volumeLimit) ? 100.0 : 800.0;

	return logarithmicToLinear([audioPlayer volume], MAX_VOLUME);
}

- (void)setVolume:(double)volume {
	BOOL volumeLimit = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"volumeLimit"];
	const double MAX_VOLUME = (volumeLimit) ? 100.0 : 800.0;

	volume = MAX(0.0, MIN(volume, 100.0));

	[audioPlayer setVolume:linearToLogarithmic(volume, MAX_VOLUME)];
	[self setVolumeControlsValue:volume];

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
			[self setVolumeControlsValue:logarithmicToLinear(originalVolume, MAX_VOLUME)];
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
		[self setVolumeControlsValue:logarithmicToLinear(originalVolume, MAX_VOLUME)];
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

	FadeTimerData *data = [FadeTimerData new];
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

+ (BOOL)isVarispeed {
	return [[[NSUserDefaults standardUserDefaults] stringForKey:@"rubberbandEngine"] isEqualToString:@"varispeed"];
}

// Album skips go along the playing entry's playlist, whichever is shown.
- (IBAction)skipToNextAlbum:(id)sender {
	BOOL found = NO;

	PlaylistEntry *current = [playlistController currentEntry];
	Playlist *playlist = current.playlist;
	NSInteger index = [current index];
	NSString *origAlbum = [current album];
	NSInteger count = [[playlist orderedEntries] count];

	NSInteger i;
	NSString *curAlbum;

	PlaylistEntry *pe = nil;

	for(i = 1; i < count; i++) {
		pe = [playlistController.playbackOrder entryAtPosition:index + i inPlaylist:playlist];
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
		[self playEntry:pe];
	}
}

- (IBAction)skipToPreviousAlbum:(id)sender {
	BOOL found = NO;
	BOOL foundAlbum = NO;

	PlaylistEntry *current = [playlistController currentEntry];
	Playlist *playlist = current.playlist;
	NSInteger index = [current index];
	NSString *origAlbum = [current album];
	NSInteger count = [[playlist orderedEntries] count];
	NSString *curAlbum;

	NSInteger i;

	PlaylistEntry *pe;

	for(i = 1; i < count; i++) {
		pe = [playlistController.playbackOrder entryAtPosition:index - i inPlaylist:playlist];
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
		pe = [playlistController.playbackOrder entryAtPosition:index - i inPlaylist:playlist];
		if(pe) [self playEntry:pe];
	}
}

- (double)volumeLevel {
	return [audioPlayer volume];
}

- (void)setVolumeLevel:(double)level {
	BOOL volumeLimit = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"volumeLimit"];
	const double MAX_VOLUME = (volumeLimit) ? 100.0 : 800.0;

	level = MAX(0.0, MIN(level, MAX_VOLUME));
	[audioPlayer setVolume:level];
	[self setVolumeControlsValue:logarithmicToLinear(level, MAX_VOLUME)];

	[[NSUserDefaults standardUserDefaults] setDouble:[audioPlayer volume] forKey:@"volume"];
}

// One step of the volume keys, hot keys and menu items, of the size and kind
// set in the preferences: so many points of the volume shown (5 at a time
// is a lot near silence and little near full), or of the slider's travel,
// which is curved to move evenly by ear.
- (void)stepVolume:(double)direction {
	BOOL volumeLimit = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"volumeLimit"];
	const double MAX_VOLUME = (volumeLimit) ? 100.0 : 800.0;

	double step = [[NSUserDefaults standardUserDefaults] doubleForKey:@"volumeStep"];
	if(step <= 0) step = DEFAULT_VOLUME_UP;

	double newVolume;
	if([[[NSUserDefaults standardUserDefaults] stringForKey:@"volumeStepMode"] isEqualToString:@"slider"]) {
		double travel = logarithmicToLinear([audioPlayer volume], MAX_VOLUME) + direction * step;
		newVolume = linearToLogarithmic(MAX(0.0, MIN(travel, 100.0)), MAX_VOLUME);
		[audioPlayer setVolume:newVolume];
	} else {
		newVolume = direction > 0 ? [audioPlayer volumeUp:step] : [audioPlayer volumeDown:step];
	}
	[self setVolumeControlsValue:logarithmicToLinear(newVolume, MAX_VOLUME)];

	[[NSUserDefaults standardUserDefaults] setDouble:[audioPlayer volume] forKey:@"volume"];
}

- (IBAction)volumeDown:(id)sender {
	[self stepVolume:-1];
}

- (IBAction)volumeUp:(id)sender {
	[self stepVolume:1];
}

- (IBAction)pitchDown:(id)sender {
	setSpeed(@"pitch", @"tempo", [[NSUserDefaults standardUserDefaults] doubleForKey:@"pitch"] - DEFAULT_PITCH_DOWN);
}

- (IBAction)pitchUp:(id)sender {
	setSpeed(@"pitch", @"tempo", [[NSUserDefaults standardUserDefaults] doubleForKey:@"pitch"] + DEFAULT_PITCH_UP);
}

- (IBAction)tempoDown:(id)sender {
	setSpeed(@"tempo", @"pitch", [[NSUserDefaults standardUserDefaults] doubleForKey:@"tempo"] - DEFAULT_TEMPO_DOWN);
}

- (IBAction)tempoUp:(id)sender {
	setSpeed(@"tempo", @"pitch", [[NSUserDefaults standardUserDefaults] doubleForKey:@"tempo"] + DEFAULT_TEMPO_UP);
}

- (void)audioPlayer:(AudioPlayer *)player displayEqualizer:(void *)eq {
	if(_eq && _eq != eq) {
		[equalizerWindowController setEQ:nil];
	}

	_eq = eq;

	equalizerLoadPreset((__bridge id<CogEqualizer>)eq);

	[equalizerWindowController setEQ:eq];
}

- (void)audioPlayer:(AudioPlayer *)player removeEqualizer:(void *)eq {
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
		BOOL isSameEntry = pe && pe == [self->playlistController currentEntry];

		[self->playlistController setCurrentEntry:pe];

		// Update metadata for currently playing track to update its position for scrobbling.
		[self sendMetaData];

		if(pe && self->_eq) {
			equalizerApplyGenre((__bridge id<CogEqualizer>)self->_eq, [pe genre]);
		}

		self->lastPosition = -10;

		// A track started part-way, as when resuming on launch, begins at its
		// offset. Paused, no timer would move the position off 0 until played.
		double start = (pe && pe == self->startingEntry) ? self->startingPosition : 0.0;
		self->startingEntry = nil;

		if(!isSameEntry) {
			[[NSNotificationCenter defaultCenter] postNotificationName:CogPlaybackWillResetPositionNotification object:self];
		}
		[self setPosition:start];
	});

	if(pe) {
		[[NSNotificationCenter defaultCenter] postNotificationName:CogPlaybackDidBeginNotificiation object:pe];
	}

	double duration = [pe.length doubleValue];
	if (duration >= 30.0) {
		[player setScrobbleThreshold:MIN(240.0, duration / 2.0)];
	} else {
		[player setScrobbleThreshold:0];
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

			[[NSNotificationCenter defaultCenter] postNotificationName:CogPlaybackWillResetPositionNotification object:self];
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
		if(pe) {
			[[NSNotificationCenter defaultCenter] postNotificationName:CogPlaybackDidBeginNotificiation object:pe];
		}
	});
}

- (void)audioPlayer:(AudioPlayer *)player reportPlayCountForTrack:(id)userInfo {
	if(userInfo) {
		PlaylistEntry *pe = (PlaylistEntry *)userInfo;
		[playlistController updatePlayCountForTrack:pe];
	}
}

- (void)audioPlayer:(AudioPlayer *)player reportScrobbleForTrack:(id)userInfo {
	if(userInfo) {
		PlaylistEntry *pe = (PlaylistEntry *)userInfo;
		AudioScrobblerTrack *track = [pe audioScrobblerTrack];
		[[AudioScrobbler shared] scrobbleTrack:track];
		[[CogListenBrainzScrobbler shared] scrobble:track];
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

		[playlistController commitPersistentStoreAsync];
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
	NSMutableDictionary *songInfo = [NSMutableDictionary new];

	if(entry) {
		if(entry.title && [entry.title length])
			[songInfo setObject:entry.title forKey:MPMediaItemPropertyTitle];
		if(entry.artist && [entry.artist length])
			[songInfo setObject:entry.artist forKey:MPMediaItemPropertyArtist];
		if(entry.album && [entry.album length])
			[songInfo setObject:entry.album forKey:MPMediaItemPropertyAlbumTitle];
		if(entry.albumArt) {
			CGSize artworkSize = CGSizeMake(500, 500);
			MPMediaItemArtwork *mpArtwork = [[MPMediaItemArtwork alloc]
											 initWithBoundsSize:artworkSize
											 requestHandler:^NSImage *_Nonnull(CGSize size) {
				return entry.albumArt;
			}];
			[songInfo setObject:mpArtwork forKey:MPMediaItemPropertyArtwork];
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
		// Unique across playlists, unlike the entry's position.
		[songInfo setObject:@((unsigned long long)[[[entry.objectID URIRepresentation] absoluteString] hash]) forKey:MPMediaItemPropertyPersistentID];
	}

    if (entry) {
        AudioScrobblerTrack *track = [entry audioScrobblerTrack];
        [[AudioScrobbler shared] updateNowPlaying:track];
        [[CogListenBrainzScrobbler shared] updateNowPlaying:track];
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
