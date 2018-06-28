#import "PlaybackController.h"
#import "PlaylistView.h"
#import <Foundation/NSTimer.h>
#import "CogAudio/Status.h"
#import "CogAudio/Helper.h"

#import "PlaylistController.h"
#import "PlaylistEntry.h"
#import "PlaylistLoader.h"

#import "Logging.h"

@implementation PlaybackController

#define DEFAULT_SEEK 5

NSString *CogPlaybackDidBeginNotficiation = @"CogPlaybackDidBeginNotficiation";
NSString *CogPlaybackDidPauseNotficiation = @"CogPlaybackDidPauseNotficiation";
NSString *CogPlaybackDidResumeNotficiation = @"CogPlaybackDidResumeNotficiation";
NSString *CogPlaybackDidStopNotficiation = @"CogPlaybackDidStopNotficiation";

@synthesize playbackStatus;

+ (NSSet *)keyPathsForValuesAffectingSeekable
{
    return [NSSet setWithObjects:@"playlistController.currentEntry",@"playlistController.currentEntry.seekable",nil];
}

- (id)init
{
	self = [super init];
	if (self)
	{
		[self initDefaults];
		
		seekable = NO;
		fading = NO;
	
		audioPlayer = [[AudioPlayer alloc] init];
		[audioPlayer setDelegate:self];
		[self setPlaybackStatus: kCogStatusStopped];
	}
	
	return self;
}

- (void)initDefaults
{
	NSDictionary *defaultsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithDouble:100.0], @"volume",
		nil];
		
	[[NSUserDefaults standardUserDefaults] registerDefaults:defaultsDictionary];
}



- (void)awakeFromNib
{
	double volume = [[NSUserDefaults standardUserDefaults] doubleForKey:@"volume"];

	[volumeSlider setDoubleValue:logarithmicToLinear(volume)];
	[audioPlayer setVolume:volume];

	[self setSeekable:NO];
}
	
- (IBAction)playPauseResume:(id)sender
{
	if (playbackStatus == kCogStatusStopped)
	{
		[self play:self];
	}
	else
	{
		[self pauseResume:self];
	}
}

- (IBAction)pauseResume:(id)sender
{
	if (playbackStatus == kCogStatusPaused)
		[self resume:self];
	else
		[self pause:self];
}

- (IBAction)pause:(id)sender
{
	[audioPlayer pause];
	[self setPlaybackStatus: kCogStatusPaused];
}

- (IBAction)resume:(id)sender
{
	[audioPlayer resume];

}

- (IBAction)stop:(id)sender
{
	[audioPlayer stop];
}

//called by double-clicking on table
- (void)playEntryAtIndex:(int)i
{
    [self playEntryAtIndex:i startPaused:NO];
}

- (void)playEntryAtIndex:(int)i startPaused:(BOOL)paused
{
	PlaylistEntry *pe = [playlistController entryAtIndex:i];

	[self playEntry:pe startPaused:paused];
}


- (IBAction)play:(id)sender
{
	if ([playlistView selectedRow] == -1)
		[playlistView selectRowIndexes:[NSIndexSet indexSetWithIndex:0] byExtendingSelection:NO];

	if ([playlistView selectedRow] > -1)
		[self playEntryAtIndex:(int)[playlistView selectedRow]];
}

NSDictionary * makeRGInfo(PlaylistEntry *pe)
{
    NSMutableDictionary * dictionary = [NSMutableDictionary dictionary];
    if ([pe replayGainAlbumGain] != 0)
        [dictionary setObject:[NSNumber numberWithFloat:[pe replayGainAlbumGain]] forKey:@"replayGainAlbumGain"];
    if ([pe replayGainAlbumPeak] != 0)
        [dictionary setObject:[NSNumber numberWithFloat:[pe replayGainAlbumPeak]] forKey:@"replayGainAlbumPeak"];
    if ([pe replayGainTrackGain] != 0)
        [dictionary setObject:[NSNumber numberWithFloat:[pe replayGainTrackGain]] forKey:@"replayGainTrackGain"];
    if ([pe replayGainTrackPeak] != 0)
        [dictionary setObject:[NSNumber numberWithFloat:[pe replayGainTrackPeak]] forKey:@"replayGainTrackPeak"];
    if ([pe volume] != 1)
        [dictionary setObject:[NSNumber numberWithFloat:[pe volume]] forKey:@"volume"];
    return dictionary;
}

- (void)playEntry:(PlaylistEntry *)pe
{
    [self playEntry:pe startPaused:NO];
}

- (void)playEntry:(PlaylistEntry *)pe startPaused:(BOOL)paused
{
	if (playbackStatus != kCogStatusStopped)
		[self stop:self];

	DLog(@"PLAYLIST CONTROLLER: %@", [playlistController class]);
	[playlistController setCurrentEntry:pe];
	
	[self setPosition:0.0];

	if (pe == nil)
		return;
	
#if 0
	// Race here, but the worst that could happen is we re-read the data
    if ([pe metadataLoaded] != YES) {
		[pe performSelectorOnMainThread:@selector(setMetadata:) withObject:[playlistLoader readEntryInfo:pe] waitUntilDone:YES];
	}
#elif 0
    // Racing with this version is less likely to jam up the main thread
    if ([pe metadataLoaded] != YES) {
        NSArray * entries = [NSArray arrayWithObject:pe];
        [playlistLoader loadInfoForEntries:entries];
    }
#else
    // Let's do it this way instead
    if ([pe metadataLoaded] != YES) {
        NSArray *entries = [NSArray arrayWithObject:pe];
        [playlistLoader performSelectorInBackground:@selector(loadInfoForEntries:) withObject:entries];
    }
#endif
	
	[audioPlayer play:[pe URL] withUserInfo:pe withRGInfo:makeRGInfo(pe) startPaused:paused];
}

- (IBAction)next:(id)sender
{
	if ([playlistController next] == NO)
		return;

	[self playEntry:[playlistController currentEntry]];
}

- (IBAction)prev:(id)sender
{
	if ([playlistController prev] == NO)
		return;

	[self playEntry:[playlistController currentEntry]];
}

- (void)updatePosition:(id)sender
{
	double pos = [audioPlayer amountPlayed];
	
	[self setPosition:pos];
    
    [[playlistController currentEntry] setCurrentPosition:pos];
}

- (IBAction)seek:(id)sender
{
	double time = [sender doubleValue];

    [audioPlayer seekToTime:time];
    
    [self setPosition:time];

	[[playlistController currentEntry] setCurrentPosition:time];
}

- (IBAction)spam
{
    NSPasteboard *pboard = [NSPasteboard generalPasteboard];
    
    [pboard clearContents];

    [pboard writeObjects:[NSArray arrayWithObjects:[[playlistController currentEntry] spam], nil]];
}

- (IBAction)eventSeekForward:(id)sender
{
	[self seekForward:DEFAULT_SEEK];
}

- (void)seekForward:(double)amount
{
	double seekTo = [audioPlayer amountPlayed] + amount;
	
	if (seekTo > [[[playlistController currentEntry] length] doubleValue]) 
	{
		[self next:self];
	}
	else
	{
		[audioPlayer seekToTime:seekTo];
		[self setPosition:seekTo];
	}
}

- (IBAction)eventSeekBackward:(id)sender
{
	[self seekBackward:DEFAULT_SEEK];
}

- (void)seekBackward:(double)amount
{
	double seekTo = [audioPlayer amountPlayed] - amount;
	
	if (seekTo < 0) 
		seekTo = 0;

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
- (IBAction)changeVolume:(id)sender
{
	DLog(@"VOLUME: %lf, %lf", [sender doubleValue], linearToLogarithmic([sender doubleValue]));

	[audioPlayer setVolume:linearToLogarithmic([sender doubleValue])];

	[[NSUserDefaults standardUserDefaults] setDouble:[audioPlayer volume] forKey:@"volume"];
}

/* selector for NSTimer - gets passed the Timer object itself
 and the appropriate userInfo, which in this case is an NSNumber
 containing the current volume before we start fading. */
- (void)audioFadeDown:(NSTimer *)audioTimer
{
	double volume = [audioPlayer volume];
	double originalVolume = [[audioTimer userInfo] doubleValue];
	double down = originalVolume/10;
	
	DLog(@"VOLUME IS %lf", volume);
	
	if (volume > 0.0001) //YAY! Roundoff error!
	{
		[audioPlayer volumeDown:down];
	}
	else  // volume is at 0 or below, we are ready to release the timer and move on
	{
		[audioPlayer pause];
		[audioPlayer setVolume:originalVolume];
		[volumeSlider setDoubleValue: logarithmicToLinear(originalVolume)];
		[audioTimer invalidate];
		
		fading = NO;
	}
	
}

- (void)audioFadeUp:(NSTimer *)audioTimer
{
	double volume = [audioPlayer volume];
	double originalVolume = [[audioTimer userInfo] doubleValue];
	double up = originalVolume/10;
	
	DLog(@"VOLUME IS %lf", volume);
	
	if (volume < originalVolume) 
	{
		if ((volume + up) > originalVolume)
			[audioPlayer volumeUp:(originalVolume - volume)];
		else
			[audioPlayer volumeUp:up];
	}
	else  // volume is at 0 or below, we are ready to release the timer and move on
	{
		[volumeSlider setDoubleValue: logarithmicToLinear(originalVolume)];
		[audioTimer invalidate];
		
		fading = NO;
	}
	
}

- (IBAction)fade:(id)sender
{
	double time = 0.1;
	
	// we can not allow multiple fade timers to be registered
	if (YES == fading)
		return;
	fading = YES;

	NSNumber  *originalVolume = [NSNumber numberWithDouble: [audioPlayer volume]];
	NSTimer   *fadeTimer;
	
	if (playbackStatus == kCogStatusPlaying) {
		fadeTimer = [NSTimer timerWithTimeInterval:time
												 target:self
											   selector:@selector(audioFadeDown:) 
											   userInfo:originalVolume
												repeats:YES];
		[[NSRunLoop currentRunLoop] addTimer:fadeTimer forMode:NSRunLoopCommonModes];
	}
	else
	{
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

- (IBAction)skipToNextAlbum:(id)sender
{
    BOOL found = NO;
	
	int index = [[playlistController currentEntry] index];
	NSString *origAlbum = [[playlistController currentEntry] album];
	
	int i;
	NSString *curAlbum;
	
	PlaylistEntry *pe;

	for (i = 1; i < [[playlistController arrangedObjects] count]; i++)
	{
		pe = [playlistController entryAtIndex:index + i];
		if (pe == nil) 
			break;
		
		curAlbum = [pe album];

		// check for untagged files, and just play the first untagged one
		// we come across
		if (curAlbum == nil)
		{
			found = YES;
			break;
		}
			
		if ([curAlbum caseInsensitiveCompare:origAlbum] != NSOrderedSame)
		{
			found = YES;
			break;
		}
	}

	if (found)
	{
		[self playEntryAtIndex:i + index];
	}
}

- (IBAction)skipToPreviousAlbum:(id)sender
{
	BOOL found = NO;
	BOOL foundAlbum = NO;
	
	int index = [[playlistController currentEntry] index];
	NSString *origAlbum = [[playlistController currentEntry] album];
	NSString *curAlbum;
	
	int i;
	
	PlaylistEntry *pe;

	for (i = 1; i < [[playlistController arrangedObjects] count]; i++)
	{
		pe = [playlistController entryAtIndex:index - i];
		if (pe == nil) 
			break;

		curAlbum = [pe album];
		if (curAlbum == nil)
		{
			found = YES;
			break;
		}
		
		if ([curAlbum caseInsensitiveCompare:origAlbum] != NSOrderedSame)
		{
			if (foundAlbum == NO)
			{
				foundAlbum = YES;
				// now we need to move up to the first song in the album, so we'll
				// go till we either find index 0, or the first song in the album
				origAlbum = curAlbum;
				continue;
			}
			else
			{
				found = YES; // terminate loop
				break;
			}
		}
	}
	
	if (found || foundAlbum)
	{
		if (foundAlbum == YES)
			i--;
		[self playEntryAtIndex:index - i];
	}
}


- (IBAction)volumeDown:(id)sender
{
	double newVolume = [audioPlayer volumeDown:DEFAULT_VOLUME_DOWN];
	[volumeSlider setDoubleValue:logarithmicToLinear(newVolume)];
	
	[[NSUserDefaults standardUserDefaults] setDouble:[audioPlayer volume] forKey:@"volume"];

}

- (IBAction)volumeUp:(id)sender
{
	double newVolume;
	newVolume = [audioPlayer volumeUp:DEFAULT_VOLUME_UP];
	[volumeSlider setDoubleValue:logarithmicToLinear(newVolume)];

	[[NSUserDefaults standardUserDefaults] setDouble:[audioPlayer volume] forKey:@"volume"];
}

- (void)audioPlayer:(AudioPlayer *)player willEndStream:(id)userInfo
{
	PlaylistEntry *curEntry = (PlaylistEntry *)userInfo;
	PlaylistEntry *pe;
	
	if (curEntry.stopAfter)
		pe = nil;
	else
    {
		pe = [playlistController getNextEntry:curEntry];
        if (pe && [pe metadataLoaded] != YES) {
            NSArray * entries = [NSArray arrayWithObject:pe];
            [playlistLoader performSelectorInBackground:@selector(loadInfoForEntries:) withObject:entries];
        }
    }

    if (pe)
        [player setNextStream:[pe URL] withUserInfo:pe withRGInfo:makeRGInfo(pe)];
    else
        [player setNextStream:nil];
}

- (void)audioPlayer:(AudioPlayer *)player didBeginStream:(id)userInfo
{
	PlaylistEntry *pe = (PlaylistEntry *)userInfo;
	
	[playlistController setCurrentEntry:pe];
	
	[self setPosition:0];
	
	[[NSNotificationCenter defaultCenter] postNotificationName:CogPlaybackDidBeginNotficiation object:pe];
}

- (void)audioPlayer:(AudioPlayer *)player didChangeStatus:(NSNumber *)s userInfo:(id)userInfo
{
	int status = [s intValue];
	if (status == kCogStatusStopped || status == kCogStatusPaused)
	{
		if (positionTimer)
		{
			[positionTimer invalidate];
			positionTimer = NULL;
		}
		
		if (status == kCogStatusStopped)
		{
			[self setPosition:0];
			[self setSeekable:NO]; // the player stopped, disable the slider

			[[NSNotificationCenter defaultCenter] postNotificationName:CogPlaybackDidStopNotficiation object:nil];
		}
		else // paused
		{
			[[NSNotificationCenter defaultCenter] postNotificationName:CogPlaybackDidPauseNotficiation object:nil];
		}
	}
	else if (status == kCogStatusPlaying)
	{
		if (!positionTimer) {
			positionTimer = [NSTimer timerWithTimeInterval:1.00 target:self selector:@selector(updatePosition:) userInfo:nil repeats:YES];
			[[NSRunLoop currentRunLoop] addTimer:positionTimer forMode:NSRunLoopCommonModes];
		}

		[[NSNotificationCenter defaultCenter] postNotificationName:CogPlaybackDidResumeNotficiation object:nil];
	}
	
	if (status == kCogStatusStopped) {
		DLog(@"DONE!");
		[playlistController setCurrentEntry:nil];
		[self setSeekable:NO]; // the player stopped, disable the slider
	}
	else {
		DLog(@"PLAYING!");
		[self setSeekable:YES];
	}
	
	[self setPlaybackStatus:status];
}

- (void)playlistDidChange:(PlaylistController *)p
{
	[audioPlayer resetNextStreams];
}

- (void)setPosition:(double)p
{
	position = p;

	[[playlistController currentEntry] setCurrentPosition:p];
}

- (double)position
{
	return position;
}

- (void)setSeekable:(BOOL)s
{
	seekable = s;
}

- (BOOL)seekable
{
	return seekable && [[playlistController currentEntry] seekable];
}


@end
