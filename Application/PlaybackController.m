#import "PlaybackController.h"
#import "PlaylistView.h"
#import <Foundation/NSTimer.h>
#import "CogAudio/Status.h"

#import "PlaylistController.h"
#import "PlaylistEntry.h"

@implementation PlaybackController

#define DEFAULT_SEEK 10

//MAX_VOLUME is in percent
#define MAX_VOLUME 400

//These functions are helpers for the process of converting volume from a linear to logarithmic scale.
//Numbers that goes in to audioPlayer should be logarithmic. Numbers that are displayed to the user should be linear.
//Here's why: http://www.dr-lex.34sp.com/info-stuff/volumecontrols.html
//We are using the approximation of X^4.
//Input/Output values are in percents.
double logarithmicToLinear(double logarithmic)
{
	return pow((logarithmic/MAX_VOLUME), 0.25) * 100.0;
}
double linearToLogarithmic(double linear)
{
	return (linear/100) * (linear/100) * (linear/100) * (linear/100) * MAX_VOLUME;
}
//End helper volume function thingies. ONWARDS TO GLORY!

- (id)init
{
	self = [super init];
	if (self)
	{
		[self initDefaults];
	
		audioPlayer = [[AudioPlayer alloc] init];
		[audioPlayer setDelegate:self];
		playbackStatus = kCogStatusStopped;
		
		showTimeRemaining = NO;
		
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

	[volumeSlider setDoubleValue:logarithmicToLinear(100.0)];
	[audioPlayer setVolume: 100];
		
	[positionSlider setEnabled:NO];
}
	
- (IBAction)playPauseResume:(id)sender
{
	if (playbackStatus == kCogStatusStopped)
		[self play:self];
	else
		[self pauseResume:self];
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
	playbackStatus = kCogStatusPaused;
	
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
	PlaylistEntry *pe = [[playlistController arrangedObjects] objectAtIndex:i];

	[self playEntry:pe];
}


- (IBAction)playbackButtonClick:(id)sender
{
	int clickedSegment = [sender selectedSegment];
	if (clickedSegment == 0) //Previous
	{
		[sender setSelected:YES forSegment:0];
		[sender setSelected:YES forSegment:1];
		[self prev:sender];
	}
	else if (clickedSegment == 1) //Play
	{
		[self playPauseResume:sender];
	}
	else if (clickedSegment == 2) //Next
	{
		[self next:sender];
	}
}


- (IBAction)play:(id)sender
{
	if ([playlistView selectedRow] == -1)
		[playlistView selectRow:0 byExtendingSelection:NO];
	
	[self playEntryAtIndex:[playlistView selectedRow]];
}

- (void)playEntry:(PlaylistEntry *)pe
{
	if (playbackStatus != kCogStatusStopped)
		[self stop:self];
	
	[playlistController setCurrentEntry:pe];

	[positionSlider setDoubleValue:0.0];
	
	[self updateTimeField:0.0f];
	
	[audioPlayer play:[pe url] withUserInfo:pe];
	
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

	[self stop:self];
	[self playEntry:[playlistController currentEntry]];
}

- (IBAction)prev:(id)sender
{
	if ([playlistController prev] == NO)
		return;

	[self stop:self];
	[self playEntry:[playlistController currentEntry]];
}

- (IBAction)seek:(id)sender
{
	double time;
	time = [positionSlider doubleValue];

	if ([sender tracking] == NO) // check if user stopped sliding  before playing audio
        [audioPlayer seekToTime:time];
	
	[self updateTimeField:time];
}

- (IBAction)eventSeekForward:(id)sender
{
	[self seekForward:DEFAULT_SEEK];
}

- (void)seekForward:(double)amount
{
	double seekTo = [audioPlayer amountPlayed] + amount;
	
	if (seekTo > (int)[positionSlider maxValue]) 
	{
		[self next:self];
	}
	else
	{
		[audioPlayer seekToTime:seekTo];
		[self updateTimeField:seekTo];
		[positionSlider setDoubleValue:seekTo];
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
	{
		[audioPlayer seekToTime:0];
		[self updateTimeField:0];
		[positionSlider setDoubleValue:0.0];
	}
	else 
	{
		[audioPlayer seekToTime:seekTo];
		[self updateTimeField:seekTo];
		[positionSlider setDoubleValue:seekTo];
	}
}

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

- (IBAction)changeVolume:(id)sender
{
	double oneLog = logarithmicToLinear(100.0);
	double distance = [sender frame].size.height*([sender doubleValue] - oneLog)/100.0;
	if (fabs(distance) < 2.0)
	{
		[sender setDoubleValue:oneLog];
	}

	NSLog(@"VOLUME: %lf, %lf", [sender doubleValue], linearToLogarithmic([sender doubleValue]));

	[audioPlayer setVolume:linearToLogarithmic([sender doubleValue])];
}

/* selector for NSTimer - gets passed the Timer object itself
 and the appropriate userInfo, which in this case is an NSNumber
 containing the current volume before we start fading. */
- (void)audioFadeDown:(NSTimer *)audioTimer
{
	double volume = [audioPlayer volume];
	double originalVolume = [[audioTimer userInfo] doubleValue];

	NSLog(@"VOLUME IS %lf", volume);
	
	if (volume > 0.0001) //YAY! Roundoff error!
	{
		[self volumeDown:5];
	}
	else  // volume is at 0 or below, we are ready to release the timer and move on
	{
		[audioPlayer pause];
		[audioPlayer setVolume:originalVolume];
		[volumeSlider setDoubleValue: logarithmicToLinear(originalVolume)];
		[audioTimer invalidate];
	}
	
}

- (void)audioFadeUp:(NSTimer *)audioTimer
{
	double volume = [audioPlayer volume];
	double originalVolume = [[audioTimer userInfo] doubleValue];

	NSLog(@"VOLUME IS %lf", volume);
	
	if (volume < originalVolume) 
	{
		if ((volume + 5) > originalVolume)
			[self volumeUp:(originalVolume - volume)];
		else
			[self volumeUp:5];
	}
	else  // volume is at 0 or below, we are ready to release the timer and move on
	{
		[volumeSlider setDoubleValue: logarithmicToLinear(originalVolume)];
		[audioTimer invalidate];
	}
	
}

- (IBAction)fadeOut:(id)sender withTime:(double)time
{
	// we can not allow multiple fade timers to be registered
	if (playbackStatus == kCogStatusFading)
		return;

	NSNumber  *originalVolume = [NSNumber numberWithDouble: [audioPlayer volume]];
	NSTimer   *fadeTimer;
	
	if (playbackStatus == kCogStatusPlaying)
		fadeTimer = [NSTimer scheduledTimerWithTimeInterval:time
												 target:self
											   selector:@selector(audioFadeDown:) 
											   userInfo:originalVolume
												repeats:YES];
	else
	{
		[audioPlayer setVolume:0];
		fadeTimer = [NSTimer scheduledTimerWithTimeInterval:time
													 target:self
												   selector:@selector(audioFadeUp:) 
												   userInfo:originalVolume
													repeats:YES];
		[self pauseResume:self];
	}

	playbackStatus = kCogStatusFading;

}

- (IBAction)skipToNextAlbum:(id)sender
{
    BOOL found = NO;
	
	NSNumber *index = (NSNumber *)[[playlistController currentEntry] index];
	NSString *origAlbum = [[playlistController entryAtIndex:[index intValue]] album];
	
	int playlistLength = [[playlistController arrangedObjects] count] - 1;
	int i = [index intValue] + 1;
	NSString *curAlbum;
	
	PlaylistEntry *pe;

	while (!found)
	{
		pe = [[playlistController arrangedObjects] objectAtIndex:i];
		curAlbum = [pe album];

		// check for untagged files, and just play the first untagged one
		// we come across
		if (curAlbum == nil)
			break;
		
		if (![curAlbum caseInsensitiveCompare:origAlbum])
		{
			i++;
			if (i > playlistLength)
				return;
			continue;
		}
		else
		{
			found = YES;
		}
	}

	[self playEntryAtIndex:i];

}

- (IBAction)skipToPreviousAlbum:(id)sender
{
    BOOL found = NO;
	BOOL foundAlbum = NO;
	
	NSNumber *index = (NSNumber *)[[playlistController currentEntry] index];
	NSString *origAlbum = [[playlistController entryAtIndex:[index intValue]] album];
	NSString *curAlbum;
	
	int i = [index intValue] - 1;
	
	if (i <= 0)
		return;
	
	PlaylistEntry *pe;
	
	while (!found)
	{
		pe = [[playlistController arrangedObjects] objectAtIndex:i];
		curAlbum = [pe album];
		if (curAlbum == nil)
			break;
		
		if (![curAlbum caseInsensitiveCompare:origAlbum])
		{
			i--;
			if (i == 0) // first entry in playlist
				if (foundAlbum == YES)
					break;
				else
					return;
			continue;
		}
		else
		{
			if (foundAlbum == NO)
			{
				foundAlbum = YES;
				// now we need to move up to the first song in the album, so we'll
				// go till we either find index 0, or the first song in the album
				origAlbum = [[playlistController entryAtIndex:i] album];
				i--;
			}
			else
				found = YES; // terminate loop
			
		}
	}
	
	if ((foundAlbum == YES) && i != 0)
		i++;
	[self playEntryAtIndex:i];
	
}


- (IBAction)eventVolumeDown:(id)sender
{
	[self volumeDown:DEFAULT_VOLUME_DOWN];
}

- (void)volumeDown:(double)amount
{
	double newVolume;
	if (amount > [audioPlayer volume])
		newVolume = 0.0;
	else
		newVolume = linearToLogarithmic(logarithmicToLinear([audioPlayer volume]) - amount);

	[volumeSlider setDoubleValue:logarithmicToLinear(newVolume)];
	[audioPlayer setVolume:newVolume];
}

- (IBAction)eventVolumeUp:(id)sender
{
	[self volumeUp:DEFAULT_VOLUME_UP];
}
- (void)volumeUp:(double)amount
{
	double newVolume = linearToLogarithmic(logarithmicToLinear([audioPlayer volume]) + amount);
	if (newVolume > MAX_VOLUME)
		newVolume = MAX_VOLUME;

	[volumeSlider setDoubleValue:logarithmicToLinear(newVolume)];
	[audioPlayer setVolume:newVolume];
}


- (void)updateTimeField:(double)pos
{
	NSString *text;
	if (showTimeRemaining == NO)
	{
		int sec = (int)(pos);
		text = [NSString stringWithFormat:NSLocalizedString(@"TimeElapsed", @""), sec/60, sec%60];
	}
	else
	{
		int sec = (int)(([positionSlider maxValue] - pos));
		if (sec < 0)
			sec = 0;
		text = [NSString stringWithFormat:NSLocalizedString(@"TimeRemaining", @""), sec/60, sec%60];
	}
	[timeField setStringValue:text];
}	

- (IBAction)toggleShowTimeRemaining:(id)sender
{
	showTimeRemaining = !showTimeRemaining;

	[self updateTimeField:[positionSlider doubleValue]];
}

- (void)audioPlayer:(AudioPlayer *)player requestNextStream:(id)userInfo
{
	PlaylistEntry *curEntry = (PlaylistEntry *)userInfo;
	PlaylistEntry *pe = [playlistController getNextEntry:curEntry];

	[player setNextStream:[pe url] withUserInfo:pe];
}

- (void)audioPlayer:(AudioPlayer *)player streamChanged:(id)userInfo
{
	PlaylistEntry *pe = (PlaylistEntry *)userInfo;
	
	[playlistController setCurrentEntry:pe];
	
	[positionSlider setDoubleValue:0.0f];

	[self updateTimeField:0.0f];
	
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

- (void)updatePosition:(id)sender
{
	double pos = [audioPlayer amountPlayed];

	if ([positionSlider tracking] == NO)
	{
		[positionSlider setDoubleValue:pos];
		[self updateTimeField:pos];
	}
	
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
			[positionSlider setDoubleValue:0.0f];
			
			[self updateTimeField:0.0f];			
		}
		
		//Show play image
		[self changePlayButtonImage:@"play"];
	}
	else if (status == kCogStatusPlaying)
	{
		if (!positionTimer)
			positionTimer = [NSTimer scheduledTimerWithTimeInterval:1.00 target:self selector:@selector(updatePosition:) userInfo:nil repeats:YES];

		//Show pause
		[self changePlayButtonImage:@"pause"];
	}
	
	if (status == kCogStatusStopped) {
		[positionSlider setEnabled:NO];
	}
	else {
		[positionSlider setEnabled:[[[playlistController currentEntry] seekable] boolValue]];
	}
	
	playbackStatus = status;
}

-(BOOL)validateMenuItem:(NSMenuItem*)menuItem
{
	SEL action = [menuItem action];
	
	if (action == @selector(seekBackward:) && (playbackStatus == kCogStatusStopped))
		return NO;
	
	if (action == @selector(seekForward:) && (playbackStatus == kCogStatusStopped))
		return NO;
	
	if (action == @selector(stop:) && (playbackStatus == kCogStatusStopped))
		return NO;
	
	return YES;
}



@end
