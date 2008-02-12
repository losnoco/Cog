#import "PlaybackController.h"
#import "PlaylistView.h"
#import <Foundation/NSTimer.h>
#import "CogAudio/Status.h"

#import "PlaylistController.h"
#import "PlaylistEntry.h"

@implementation PlaybackController

#define DEFAULT_SEEK 10

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
	[volumeSlider setDoubleValue:pow(10.0, log10(0.5)/4.0)*100];
	
	double percent;
	percent = (float)[volumeSlider doubleValue]/[volumeSlider maxValue];//100.0;
	percent = percent * percent * percent * percent;
	
	currentVolume = ((float)[volumeSlider doubleValue]/100.0)*[volumeSlider maxValue];//percent * 1000;//0;//[volumeSlider doubleValue];
		
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
	[audioPlayer setVolume:currentVolume];
	
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
	if ([playlistController prev] == nil)
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

- (IBAction)seekForward:(id)sender
{
	double seekTo = [audioPlayer amountPlayed] + DEFAULT_SEEK;
	
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

- (IBAction)seekBackward:(id)sender
{
	double seekTo = [audioPlayer amountPlayed] - DEFAULT_SEEK;
	
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
	double percent;
	
	//Approximated log
	percent = (float)[sender doubleValue]/100.0;
	percent = percent * percent * percent * percent;

	//gravitates at the 100% mark
	float v = [sender frame].size.width - ([sender frame].size.width*(percent*[sender maxValue])/100.0);
	if (fabs(v) < 10.0)
	{
		percent = 0.5;
		v = pow(10.0, log10(percent)/4.0);
		[sender setDoubleValue:v*100.0];
	}
	
	currentVolume = percent * 100.0;
	
	[audioPlayer  setVolume:currentVolume];
}

/* selector for NSTimer - gets passed the Timer object itself
 and the appropriate userInfo, which in this case is an NSNumber
 containing the current volume before we start fading. */
- (void)audioFader:(NSTimer *)audioTimer
{
	double volume = currentVolume;
	NSArray *origValues = [audioTimer userInfo];
	id originalVolume = [origValues objectAtIndex:0];
	id origSliderVolume = [origValues objectAtIndex:1];

	if (volume > 0)
	{
		[self volumeDown:self];
	}
	else  // volume is at 0 or below, we are ready to release the timer and move on
	{
		[audioPlayer pause];
		currentVolume = [originalVolume doubleValue];
		[audioPlayer setVolume:currentVolume];
		[volumeSlider setDoubleValue:[origSliderVolume doubleValue]];
		[audioTimer invalidate];
	}
	
}

- (IBAction)fadeOut:(id)sender withTime:(double)time
{
	id      origCurrentVolume    = [NSNumber numberWithDouble: currentVolume];
	id      origSliderVolume     = [NSNumber numberWithDouble: [volumeSlider doubleValue]];

	NSArray *originalValues      = [NSArray arrayWithObjects:origCurrentVolume,origSliderVolume,nil];
	NSTimer *fadeTimer;

	NSLog(@"currentVolume here%f", [volumeSlider doubleValue]);
	fadeTimer = [NSTimer scheduledTimerWithTimeInterval:time
												 target:self
											   selector:@selector(audioFader:) 
											   userInfo:originalValues
												repeats:YES];
}

- (IBAction)volumeDown:(id)sender
{
	double percent;
	[volumeSlider setDoubleValue:([volumeSlider doubleValue] - 5)];
	
	percent = (float)[volumeSlider doubleValue]/[volumeSlider maxValue];//100.0;
	percent = percent * percent * percent * percent;
	
	currentVolume = (percent * [volumeSlider maxValue]) + [volumeSlider doubleValue];//100.0;
	NSLog(@"currentVolume %f", currentVolume);
	
	[audioPlayer  setVolume:currentVolume];
}

- (IBAction)volumeUp:(id)sender
{
	double percent;
	
	[volumeSlider setDoubleValue:([volumeSlider doubleValue] + 5)];
	
	percent = (float)[volumeSlider doubleValue]/[volumeSlider maxValue];//100.0;
	percent = percent * percent * percent * percent;
	
	currentVolume = (percent * [volumeSlider maxValue]) + [volumeSlider doubleValue];//100.0);
	if (currentVolume > 400)
		currentVolume = 400;

	NSLog(@"%f", currentVolume);


	[audioPlayer  setVolume:currentVolume];
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
