#import "PlaybackController.h"
#import "PlaylistView.h"
#import <Foundation/NSTimer.h>
#import "CogAudio/Status.h"
#import "CogAudio/Helper.h"

#import "PlaylistController.h"
#import "PlaylistEntry.h"

@implementation PlaybackController

#define DEFAULT_SEEK 5

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
		
		scrobbler = [[AudioScrobbler alloc] init];
		[GrowlApplicationBridge setGrowlDelegate:self];
	}
	
	return self;
}

- (void)initDefaults
{
	NSDictionary *defaultsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithBool:YES], @"enableAudioScrobbler",
		[NSNumber numberWithBool:NO],  @"automaticallyLaunchLastFM",
		[NSNumber numberWithDouble:100.0], @"volume",
		nil];
		
	[[NSUserDefaults standardUserDefaults] registerDefaults:defaultsDictionary];
}

- (NSDictionary *) registrationDictionaryForGrowl
{
	NSArray *notifications = [NSArray arrayWithObjects:@"Stream Changed", nil];
	
	return [NSDictionary dictionaryWithObjectsAndKeys:
		@"Cog", GROWL_APP_NAME,  
		notifications, GROWL_NOTIFICATIONS_ALL, 
		notifications, GROWL_NOTIFICATIONS_DEFAULT,
		nil];
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
	
	if([[NSUserDefaults standardUserDefaults] boolForKey:@"enableAudioScrobbler"]) {
		[scrobbler pause];
	}
}

- (IBAction)resume:(id)sender
{
	[audioPlayer resume];

	
	if([[NSUserDefaults standardUserDefaults] boolForKey:@"enableAudioScrobbler"]) {
		[scrobbler resume];
	}
}

- (IBAction)stop:(id)sender
{
	[audioPlayer stop];

	if([[NSUserDefaults standardUserDefaults] boolForKey:@"enableAudioScrobbler"]) {
		[scrobbler stop];
	}

}

//called by double-clicking on table
- (void)playEntryAtIndex:(int)i
{
	PlaylistEntry *pe = [playlistController entryAtIndex:i];

	[self playEntry:pe];
}


- (IBAction)play:(id)sender
{
	if ([playlistView selectedRow] == -1)
		[playlistView selectRow:0 byExtendingSelection:NO];	

	if ([playlistView selectedRow] > -1)
		[self playEntryAtIndex:[playlistView selectedRow]];
}

- (void)playEntry:(PlaylistEntry *)pe
{
	if (playbackStatus != kCogStatusStopped)
		[self stop:self];

	NSLog(@"PLAYLIST CONTROLLER: %@", [playlistController class]);
	[playlistController setCurrentEntry:pe];
	
	[self setPosition:0.0];

	if (pe == nil)
		return;
	
	[audioPlayer play:[pe URL] withUserInfo:pe];
	
	if([[NSUserDefaults standardUserDefaults] boolForKey:@"enableAudioScrobbler"]) {
		[scrobbler start:pe];
	}

	[GrowlApplicationBridge notifyWithTitle:[pe title]
								description:[pe artist]
						   notificationName:@"Stream Changed"
								   iconData:nil
								   priority:0 
								   isSticky:NO 
							   clickContext:nil];
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
}

- (IBAction)seek:(id)sender
{
	double time = [sender doubleValue];

    [audioPlayer seekToTime:time];
	
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
		NSLog(@"Error loading image!");
	}
	
	[playbackButtons setImage:img forSegment:1];
}
*/
- (IBAction)changeVolume:(id)sender
{
	NSLog(@"VOLUME: %lf, %lf", [sender doubleValue], linearToLogarithmic([sender doubleValue]));

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
	
	NSLog(@"VOLUME IS %lf", volume);
	
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
	
	NSLog(@"VOLUME IS %lf", volume);
	
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

- (void)audioPlayer:(AudioPlayer *)player requestNextStream:(id)userInfo
{
	PlaylistEntry *curEntry = (PlaylistEntry *)userInfo;
	PlaylistEntry *pe;
	
	if (curEntry.stopAfter)
		pe = nil;
	else
		pe = [playlistController getNextEntry:curEntry];

	[player setNextStream:[pe URL] withUserInfo:pe];
}

- (void)audioPlayer:(AudioPlayer *)player streamChanged:(id)userInfo
{
	PlaylistEntry *pe = (PlaylistEntry *)userInfo;
	
	[playlistController setCurrentEntry:pe];
	
	[self setPosition:0];

	if([[NSUserDefaults standardUserDefaults] boolForKey:@"enableAudioScrobbler"]) {
		[scrobbler start:pe];
	}

	[GrowlApplicationBridge notifyWithTitle:[pe title]
								description:[pe artist]
						   notificationName:@"Stream Changed"
								   iconData:nil
								   priority:0 
								   isSticky:NO 
							   clickContext:nil];
}

- (void)audioPlayer:(AudioPlayer *)player statusChanged:(id)s
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
		}
	}
	else if (status == kCogStatusPlaying)
	{
		if (!positionTimer) {
			positionTimer = [NSTimer timerWithTimeInterval:1.00 target:self selector:@selector(updatePosition:) userInfo:nil repeats:YES];
			[[NSRunLoop currentRunLoop] addTimer:positionTimer forMode:NSRunLoopCommonModes];
		}
	}
	
	if (status == kCogStatusStopped) {
		NSLog(@"DONE!");
		[playlistController setCurrentEntry:nil];
		[self setSeekable:NO]; // the player stopped, disable the slider
	}
	else {
		NSLog(@"PLAYING!");
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
