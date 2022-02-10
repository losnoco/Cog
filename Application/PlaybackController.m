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

@implementation PlaybackController

#define DEFAULT_SEEK 5

NSString *CogPlaybackDidBeginNotficiation = @"CogPlaybackDidBeginNotficiation";
NSString *CogPlaybackDidPauseNotficiation = @"CogPlaybackDidPauseNotficiation";
NSString *CogPlaybackDidResumeNotficiation = @"CogPlaybackDidResumeNotficiation";
NSString *CogPlaybackDidStopNotficiation = @"CogPlaybackDidStopNotficiation";

@synthesize playbackStatus;

@synthesize progressBarStatus;

+ (NSSet *)keyPathsForValuesAffectingSeekable {
	return [NSSet setWithObjects:@"playlistController.currentEntry", @"playlistController.currentEntry.seekable", nil];
}

- (id)init {
	self = [super init];
	if(self) {
		[self initDefaults];

		seekable = NO;
		fading = NO;
		_eqWasOpen = NO;
		_equi = nil;

		progressBarStatus = -1;

		audioPlayer = [[AudioPlayer alloc] init];
		[audioPlayer setDelegate:self];
		[self setPlaybackStatus:CogStatusStopped];
	}

	return self;
}

- (void)initDefaults {
	NSDictionary *defaultsDictionary = @{@"volume": [NSNumber numberWithDouble:100.0],
										 @"GraphicEQenable": [NSNumber numberWithBool:NO],
										 @"GraphicEQpreset": [NSNumber numberWithInt:-1],
										 @"GraphicEQtrackgenre": [NSNumber numberWithBool:NO],
										 @"volumeLimit": [NSNumber numberWithBool:YES],
										 @"headphoneVirtualization": [NSNumber numberWithBool:NO]};

	[[NSUserDefaults standardUserDefaults] registerDefaults:defaultsDictionary];
}

- (void)awakeFromNib {
	BOOL volumeLimit = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"volumeLimit"];
	const double MAX_VOLUME = (volumeLimit) ? 100.0 : 800.0;

	double volume = [[NSUserDefaults standardUserDefaults] doubleForKey:@"volume"];

	[volumeSlider setDoubleValue:logarithmicToLinear(volume, MAX_VOLUME)];
	[audioPlayer setVolume:volume];

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

	[self removeHDCD:nil];
}

// called by double-clicking on table
- (void)playEntryAtIndex:(NSInteger)i {
	[self playEntryAtIndex:i startPaused:NO andSeekTo:[NSNumber numberWithDouble:0.0]];
}

- (void)playEntryAtIndex:(NSInteger)i startPaused:(BOOL)paused {
	PlaylistEntry *pe = [playlistController entryAtIndex:i];

	[self playEntry:pe startPaused:paused andSeekTo:[NSNumber numberWithDouble:0.0]];
}

- (void)playEntryAtIndex:(NSInteger)i startPaused:(BOOL)paused andSeekTo:(id)offset {
	PlaylistEntry *pe = [playlistController entryAtIndex:i];

	[self playEntry:pe startPaused:paused andSeekTo:offset];
}

- (IBAction)play:(id)sender {
	if([playlistView selectedRow] == -1)
		[playlistView selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];

	if([playlistView selectedRow] > -1)
		[self playEntryAtIndex:(int)[playlistView selectedRow]];
}

NSDictionary *makeRGInfo(PlaylistEntry *pe) {
	NSMutableDictionary *dictionary = [NSMutableDictionary dictionary];
	if([pe replayGainAlbumGain] != 0)
		[dictionary setObject:[NSNumber numberWithFloat:[pe replayGainAlbumGain]] forKey:@"replayGainAlbumGain"];
	if([pe replayGainAlbumPeak] != 0)
		[dictionary setObject:[NSNumber numberWithFloat:[pe replayGainAlbumPeak]] forKey:@"replayGainAlbumPeak"];
	if([pe replayGainTrackGain] != 0)
		[dictionary setObject:[NSNumber numberWithFloat:[pe replayGainTrackGain]] forKey:@"replayGainTrackGain"];
	if([pe replayGainTrackPeak] != 0)
		[dictionary setObject:[NSNumber numberWithFloat:[pe replayGainTrackPeak]] forKey:@"replayGainTrackPeak"];
	if([pe volume] != 1)
		[dictionary setObject:[NSNumber numberWithFloat:[pe volume]] forKey:@"volume"];
	return dictionary;
}

- (void)playEntry:(PlaylistEntry *)pe {
	[self playEntry:pe startPaused:NO andSeekTo:[NSNumber numberWithDouble:0.0]];
}

- (void)playEntry:(PlaylistEntry *)pe startPaused:(BOOL)paused {
	[self playEntry:pe startPaused:paused andSeekTo:[NSNumber numberWithDouble:0.0]];
}

- (void)playEntry:(PlaylistEntry *)pe startPaused:(BOOL)paused andSeekTo:(id)offset {
	if(playbackStatus != CogStatusStopped && playbackStatus != CogStatusStopping)
		[self stop:self];

	DLog(@"PLAYLIST CONTROLLER: %@", [playlistController class]);
	[playlistController setCurrentEntry:pe];

	lastPosition = -10;

	[self setPosition:[offset doubleValue]];

	if(pe == nil)
		return;

	BOOL loadData = YES;
	NSString *urlScheme = [[pe URL] scheme];
	if([urlScheme isEqualToString:@"http"] ||
	   [urlScheme isEqualToString:@"https"])
		loadData = NO;

#if 0
	// Race here, but the worst that could happen is we re-read the data
    if ([pe metadataLoaded] != YES) {
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

	[audioPlayer play:[pe URL] withUserInfo:pe withRGInfo:makeRGInfo(pe) startPaused:paused andSeekTo:[offset doubleValue]];
}

- (IBAction)next:(id)sender {
	if([playlistController next] == NO)
		return;

	[self playEntry:[playlistController currentEntry]];
}

- (IBAction)prev:(id)sender {
	if([playlistController prev] == NO)
		return;

	[self playEntry:[playlistController currentEntry]];
}

- (void)updatePosition:(id)sender {
	double pos = [audioPlayer amountPlayed];

	[self setPosition:pos];

	[[playlistController currentEntry] setCurrentPosition:pos];
}

- (IBAction)seek:(id)sender {
	double time = [sender doubleValue];

	[audioPlayer seekToTime:time];

	lastPosition = -10;

	[self setPosition:time];

	[[playlistController currentEntry] setCurrentPosition:time];
}

- (IBAction)seek:(id)sender toTime:(NSTimeInterval)position {
	double time = (double)(position);

	lastPosition = -10;

	[audioPlayer seekToTime:time];

	[self setPosition:time];

	[[playlistController currentEntry] setCurrentPosition:time];
}

- (IBAction)spam:(id)sender {
	NSPasteboard *pboard = [NSPasteboard generalPasteboard];

	[pboard clearContents];

	[pboard writeObjects:@[[[playlistController currentEntry] spam]]];
}

- (IBAction)eventSeekForward:(id)sender {
	[self seekForward:DEFAULT_SEEK];
}

- (void)seekForward:(double)amount {
	double seekTo = [audioPlayer amountPlayed] + amount;

	if(seekTo > [[[playlistController currentEntry] length] doubleValue]) {
		[self next:self];
	} else {
		lastPosition = -10;
		[audioPlayer seekToTime:seekTo];
		[self setPosition:seekTo];
	}
}

- (IBAction)eventSeekBackward:(id)sender {
	[self seekBackward:DEFAULT_SEEK];
}

- (void)seekBackward:(double)amount {
	double seekTo = [audioPlayer amountPlayed] - amount;

	if(seekTo < 0)
		seekTo = 0;

	lastPosition = -10;

	[audioPlayer seekToTime:seekTo];
	[self setPosition:seekTo];
}

/*
 - (void)changePlayButtonImage:(NSString *)name
{
    NSImage *img = [NSImage imageNamed:name];
//	[img retain];

    if (img == nil)
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
	double volume = [audioPlayer volume];
	double originalVolume = [[audioTimer userInfo] doubleValue];
	double down = originalVolume / 10;

	DLog(@"VOLUME IS %lf", volume);

	if(volume > 0.0001) // YAY! Roundoff error!
	{
		[audioPlayer volumeDown:down];
	} else // volume is at 0 or below, we are ready to release the timer and move on
	{
		BOOL volumeLimit = [[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"volumeLimit"];
		const double MAX_VOLUME = (volumeLimit) ? 100.0 : 800.0;

		[audioPlayer pause];
		[audioPlayer setVolume:originalVolume];
		[volumeSlider setDoubleValue:logarithmicToLinear(originalVolume, MAX_VOLUME)];
		[audioTimer invalidate];

		fading = NO;
	}
}

- (void)audioFadeUp:(NSTimer *)audioTimer {
	double volume = [audioPlayer volume];
	double originalVolume = [[audioTimer userInfo] doubleValue];
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

	NSNumber *originalVolume = [NSNumber numberWithDouble:[audioPlayer volume]];
	NSTimer *fadeTimer;

	if(playbackStatus == CogStatusPlaying) {
		fadeTimer = [NSTimer timerWithTimeInterval:time
		                                    target:self
		                                  selector:@selector(audioFadeDown:)
		                                  userInfo:originalVolume
		                                   repeats:YES];
		[[NSRunLoop currentRunLoop] addTimer:fadeTimer forMode:NSRunLoopCommonModes];
	} else {
		[audioPlayer setVolume:0];
		fadeTimer = [NSTimer timerWithTimeInterval:time
		                                    target:self
		                                  selector:@selector(audioFadeUp:)
		                                  userInfo:originalVolume
		                                   repeats:YES];
		[[NSRunLoop currentRunLoop] addTimer:fadeTimer forMode:NSRunLoopCommonModes];
		[self pauseResume:self];
	}
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

- (void)eqAlloc {
	// Show a stopped equalizer as a stub
	OSStatus err;
	AudioComponentDescription desc;

	desc.componentType = kAudioUnitType_Effect;
	desc.componentSubType = kAudioUnitSubType_GraphicEQ;
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;

	AudioComponent comp = NULL;

	desc.componentType = kAudioUnitType_Effect;
	desc.componentSubType = kAudioUnitSubType_GraphicEQ;

	comp = AudioComponentFindNext(comp, &desc);
	if(!comp)
		return;

	err = AudioComponentInstanceNew(comp, &_eq);
	if(err)
		return;

	AudioUnitInitialize(_eq);
}

- (void)eqDealloc {
	AudioUnitUninitialize(_eq);
	AudioComponentInstanceDispose(_eq);
	_eq = nil;
	_eqStubbed = NO;
}

- (IBAction)showEq:(id)sender {
	if(_eq) {
		if(_equi && [_equi isOpen])
			[_equi bringToFront];
		else
			_equi = [[AUPluginUI alloc] initWithSampler:_eq bringToFront:YES orWindowNumber:0];
	} else {
		[self eqAlloc];
		_eqWasOpen = YES;
		[self audioPlayer:nil displayEqualizer:_eq];
		[_equi bringToFront];
	}
}

- (void)audioPlayer:(AudioPlayer *)player displayEqualizer:(AudioUnit)eq {
	if(_equi) {
		_eqWasOpen = [_equi isOpen];
		_equi = nil;
	}

	if(_eq && _eq != eq) {
		OSStatus err;
		CFPropertyListRef classData;
		UInt32 size;

		size = sizeof(classData);
		err = AudioUnitGetProperty(_eq, kAudioUnitProperty_ClassInfo, kAudioUnitScope_Global, 0, &classData, &size);
		if(err == noErr) {
			CFPreferencesSetAppValue(CFSTR("GraphEQ_Preset"), classData, kCFPreferencesCurrentApplication);
			CFRelease(classData);
		}

		CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);

		// Caller relinquishes EQ to us
		[self eqDealloc];
	}

	_eq = eq;

	{
		OSStatus err;
		ComponentDescription cd;
		CFPropertyListRef classData;
		CFDictionaryRef dict;
		CFNumberRef cfnum;

		classData = CFPreferencesCopyAppValue(CFSTR("GraphEQ_Preset"), kCFPreferencesCurrentApplication);
		if(classData) {
			dict = (CFDictionaryRef)classData;

			cfnum = (CFNumberRef)(CFDictionaryGetValue(dict, CFSTR("type")));
			CFNumberGetValue(cfnum, kCFNumberSInt32Type, &cd.componentType);
			cfnum = (CFNumberRef)(CFDictionaryGetValue(dict, CFSTR("subtype")));
			CFNumberGetValue(cfnum, kCFNumberSInt32Type, &cd.componentSubType);
			cfnum = (CFNumberRef)(CFDictionaryGetValue(dict, CFSTR("manufacturer")));
			CFNumberGetValue(cfnum, kCFNumberSInt32Type, &cd.componentManufacturer);

			if((cd.componentType == kAudioUnitType_Effect) &&
			   (cd.componentSubType == kAudioUnitSubType_GraphicEQ) &&
			   (cd.componentManufacturer == kAudioUnitManufacturer_Apple))
				err = AudioUnitSetProperty(eq, kAudioUnitProperty_ClassInfo, kAudioUnitScope_Global, 0, &classData, sizeof(classData));

			CFRelease(classData);
		}

		equalizerLoadPreset(eq);
	}

	if(_eqWasOpen) {
		NSWindow *window = appController.miniMode ? appController.miniWindow : appController.mainWindow;
		_equi = [[AUPluginUI alloc] initWithSampler:_eq bringToFront:NO orWindowNumber:window.windowNumber];
		_eqWasOpen = NO;
	}
}

- (void)audioPlayer:(AudioPlayer *)player refreshEqualizer:(AudioUnit)eq {
	equalizerLoadPreset(eq);
}

- (void)audioPlayer:(AudioPlayer *)player removeEqualizer:(AudioUnit)eq {
	if(eq == _eq) {
		OSStatus err;
		CFPropertyListRef classData;
		UInt32 size;

		size = sizeof(classData);
		err = AudioUnitGetProperty(eq, kAudioUnitProperty_ClassInfo, kAudioUnitScope_Global, 0, &classData, &size);
		if(err == noErr) {
			CFPreferencesSetAppValue(CFSTR("GraphEQ_Preset"), classData, kCFPreferencesCurrentApplication);
			CFRelease(classData);
		}

		CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);

		if(_equi) {
			_eqWasOpen = [_equi isOpen];
		}

		_equi = nil;
		[self eqDealloc];

		if(_eqWasOpen) {
			[self showEq:nil];
		}
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

	if(pe)
		[player setNextStream:[pe URL] withUserInfo:pe withRGInfo:makeRGInfo(pe)];
	else
		[player setNextStream:nil];
}

- (void)audioPlayer:(AudioPlayer *)player didBeginStream:(id)userInfo {
	PlaylistEntry *pe = (PlaylistEntry *)userInfo;

	[playlistController setCurrentEntry:pe];

	if(_eq)
		equalizerApplyGenre(_eq, [pe genre]);

	lastPosition = -10;

	[self setPosition:0];

	[self removeHDCD:nil];

	[[NSNotificationCenter defaultCenter] postNotificationName:CogPlaybackDidBeginNotficiation object:pe];
}

- (void)audioPlayer:(AudioPlayer *)player didChangeStatus:(NSNumber *)s userInfo:(id)userInfo {
	int status = [s intValue];
	if(status == CogStatusStopped || status == CogStatusPaused) {
		[self removeHDCD:nil];
		if(positionTimer) {
			[positionTimer invalidate];
			positionTimer = NULL;
		}

		if(status == CogStatusStopped) {
			[self setPosition:0];
			[self setSeekable:NO]; // the player stopped, disable the slider

			[[NSNotificationCenter defaultCenter] postNotificationName:CogPlaybackDidStopNotficiation object:nil];
		} else // paused
		{
			[[NSNotificationCenter defaultCenter] postNotificationName:CogPlaybackDidPauseNotficiation object:nil];
		}
	} else if(status == CogStatusPlaying) {
		if(!positionTimer) {
			positionTimer = [NSTimer timerWithTimeInterval:0.2 target:self selector:@selector(updatePosition:) userInfo:nil repeats:YES];
			[[NSRunLoop currentRunLoop] addTimer:positionTimer forMode:NSRunLoopCommonModes];
		}

		[[NSNotificationCenter defaultCenter] postNotificationName:CogPlaybackDidResumeNotficiation object:nil];
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
			DLog(@"STOPED!");
			break;
	}

	if(status == CogStatusStopped) {
		status = CogStatusStopping;
		dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 3 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
			if([self playbackStatus] == CogStatusStopping)
				[self setPlaybackStatus:CogStatusStopped];
		});
	}

	[self setPlaybackStatus:status];
	// If we don't send it here, if we've stopped, then the NPIC will be stuck at the last file we played.
	[self sendMetaData];
}

- (void)audioPlayer:(AudioPlayer *)player sustainHDCD:(id)userInfo {
	MainWindow *mainWindow = (MainWindow *)appController.mainWindow;
	[mainWindow showHDCDLogo:YES];
	MiniWindow *miniWindow = (MiniWindow *)appController.miniWindow;
	[miniWindow showHDCDLogo:YES];
}

- (void)audioPlayer:(AudioPlayer *)player restartPlaybackAtCurrentPosition:(id)userInfo {
	PlaylistEntry *pe = [playlistController currentEntry];
	BOOL paused = playbackStatus == CogStatusPaused;
	[player play:[pe URL] withUserInfo:pe withRGInfo:makeRGInfo(pe) startPaused:paused andSeekTo:[pe seekable] ? [pe currentPosition] : 0.0];
}

- (void)audioPlayer:(AudioPlayer *)player pushInfo:(NSDictionary *)info toTrack:(id)userInfo {
	PlaylistEntry *pe = [playlistController currentEntry];
	[pe setMetadata:info];
	[playlistView refreshCurrentTrack:self];
	[self sendMetaData];
	[[NSNotificationCenter defaultCenter] postNotificationName:CogPlaybackDidBeginNotficiation object:pe];
}

- (void)audioPlayer:(AudioPlayer *)player setError:(NSNumber *)status toTrack:(id)userInfo {
	PlaylistEntry *pe = (PlaylistEntry *)userInfo;
	[pe setError:[status boolValue]];
}

- (void)removeHDCD:(id)sender {
	MainWindow *mainWindow = (MainWindow *)appController.mainWindow;
	[mainWindow showHDCDLogo:NO];
	MiniWindow *miniWindow = (MiniWindow *)appController.miniWindow;
	[miniWindow showHDCDLogo:NO];
}

- (void)playlistDidChange:(PlaylistController *)p {
	[audioPlayer resetNextStreams];
}

- (void)setPosition:(double)p {
	if(p > lastPosition && (p - lastPosition) >= 10.0) {
		PlaylistEntry *pe = [playlistController currentEntry];
		NSInteger lastTrackPlaying = [pe index];

		[[NSUserDefaults standardUserDefaults] setInteger:CogStatusPlaying forKey:@"lastPlaybackStatus"];
		[[NSUserDefaults standardUserDefaults] setInteger:lastTrackPlaying forKey:@"lastTrackPlaying"];
		[[NSUserDefaults standardUserDefaults] setDouble:p forKey:@"lastTrackPosition"];

		// If we handle this here, then it will send on all seek operations, which also reset lastPosition
		[self sendMetaData];

		lastPosition = p;
	}

	position = p;

	[[playlistController currentEntry] setCurrentPosition:p];
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
		if([entry title])
			[songInfo setObject:[entry title] forKey:MPMediaItemPropertyTitle];
		if([entry artist])
			[songInfo setObject:[entry artist] forKey:MPMediaItemPropertyArtist];
		if([entry album])
			[songInfo setObject:[entry album] forKey:MPMediaItemPropertyAlbumTitle];
		if([entry albumArt]) {
			// can't do && with @available
			if(@available(macOS 10.13.2, *)) {
				CGSize artworkSize = CGSizeMake(500, 500);
				MPMediaItemArtwork *mpArtwork = [[MPMediaItemArtwork alloc] initWithBoundsSize:artworkSize
				                                                                requestHandler:^NSImage *_Nonnull(CGSize size) {
					                                                                return [entry albumArt];
				                                                                }];
				[songInfo setObject:mpArtwork forKey:MPMediaItemPropertyArtwork];
			}
		}
		// I don't know what NPIC does with these since they aren't exposed in UI, but if we have them, use it.
		// There's a bunch of other metadata, but PlaylistEntry can't represent a lot of it.
		if([entry genre])
			[songInfo setObject:[entry genre] forKey:MPMediaItemPropertyGenre];
		if([entry year]) {
			// If PlaylistEntry can represent a full date like some tag formats can do, change it
			NSCalendar *calendar = [NSCalendar currentCalendar];
			NSDate *releaseYear = [calendar dateWithEra:1 year:[[entry year] intValue] month:0 day:0 hour:0 minute:0 second:0 nanosecond:0];
			[songInfo setObject:releaseYear forKey:MPMediaItemPropertyReleaseDate];
		}
		[songInfo setObject:[NSNumber numberWithFloat:[entry currentPosition]] forKey:MPNowPlayingInfoPropertyElapsedPlaybackTime];
		[songInfo setObject:[entry length] forKey:MPMediaItemPropertyPlaybackDuration];
		[songInfo setObject:[NSNumber numberWithFloat:[entry index]] forKey:MPMediaItemPropertyPersistentID];
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

@end
