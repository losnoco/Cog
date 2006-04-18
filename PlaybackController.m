#import "PlaybackController.h"
#import "PlaylistView.h"

#import "DBLog.h"
#import "Status.h"

@implementation PlaybackController

- (id)init
{
	self = [super init];
	if (self)
	{
		soundController = [[SoundController alloc] initWithDelegate:self];
		playbackStatus = kCogStatusStopped;
		
		showTimeRemaining = NO;
	}
	
	return self;
}

- (void)awakeFromNib
{
	currentVolume = 100.0;
}


- (IBAction)playPauseResume:(id)sender
{
	DBLog(@"PLAYING");
	if (playbackStatus == kCogStatusStopped)
		[self play:self];
	else
		[self pauseResume:self];
}

- (IBAction)pauseResume:(id)sender
{
//	DBLog(@"Pause/Resume Sent!");
	if (playbackStatus == kCogStatusPaused)
		[self resume:self];
	else
		[self pause:self];
}

- (IBAction)pause:(id)sender
{
//	DBLog(@"Pause Sent!");
	[soundController pause];
}

- (IBAction)resume:(id)sender
{
//	DBLog(@"Resume Sent!");
	[soundController resume];
}

- (IBAction)stop:(id)sender
{
//	DBLog(@"Stop Sent!");

	[soundController stop];
}

//called by double-clicking on table
- (void)playEntryAtIndex:(int)i
{
	PlaylistEntry *pe = [[playlistController arrangedObjects] objectAtIndex:i];

	[playlistController setCurrentEntry:pe];

	[self playEntry:pe];
}

- (IBAction)play:(id)sender
{
	if ([playlistView selectedRow] == -1)
		[playlistView selectRow:0 byExtendingSelection:NO];
	
	[self playEntryAtIndex:[playlistView selectedRow]];
}

- (void)playEntry:(PlaylistEntry *)pe;
{
//	DBLog(@"PlayEntry: %@ Sent!", [pe filename]);
	if (playbackStatus != kCogStatusStopped)
		[self stop:self];
	
	DBLog(@"LENGTH: %lf", [pe length]);
	[positionSlider setMaxValue:[pe length]];
	[positionSlider setDoubleValue:0.0f];
	
	[self updateTimeField:0.0f];
	
	[soundController play:pe];
	[soundController setVolume:currentVolume];
}

- (IBAction)next:(id)sender
{
	DBLog(@"CALLING: %i %i", playbackStatus, kCogStatusStopped);
	if ([playlistController next] == NO)
		return;

	if (playbackStatus != kCogStatusStopped)
	{
		DBLog(@"STOPPING");
		[self stop:self];
		[self playEntry:[playlistController currentEntry]];
	}
}

- (IBAction)prev:(id)sender
{
	DBLog(@"CALLING");
	if ([playlistController prev] == nil)
		return;

	if (playbackStatus != kCogStatusStopped)
	{
		[self stop:self];
		[self playEntry:[playlistController currentEntry]];
	}
}

- (IBAction)seek:(id)sender
{
//	DBLog(@"SEEKING?");
	double time;
	time = [positionSlider doubleValue];

	if ([sender tracking] == NO) // check if user stopped sliding  before playing audio
        [soundController seekToTime:time];
	
	[self updateTimeField:time];
}

- (void)changePlayButtonImage:(NSString *)name
{
	NSImage *img = [NSImage imageNamed:[name stringByAppendingString:@"_gray"]];
	NSImage *alt = [NSImage imageNamed:[name stringByAppendingString:@"_blue"]];
	[img retain];
	[alt retain];
	if (img == nil)
	{
		DBLog(@"NIL IMAGE!!!");
	}
	if (alt == nil)
	{
		DBLog(@"NIL ALT");
	}
	
	[playButton setImage:img];
	[playButton setAlternateImage:alt];
}

- (IBAction)changeVolume:(id)sender
{
	currentVolume = (float)[sender doubleValue];
	
	//gravitates at the halfway mark
	float v = ([sender frame].size.width/[sender maxValue])*(currentVolume-([sender maxValue]/2.0));
	if (fabs(v) < 10.0)
	{
		currentVolume = [sender maxValue]/2.0;
		[sender setDoubleValue:currentVolume];
	}
	
	[soundController  setVolume:currentVolume];
}


- (void)updateTimeField:(double)pos
{
	NSString *text;
	if (showTimeRemaining == NO)
	{
		int sec = (int)(pos/1000.0);
		text = [NSString stringWithFormat:NSLocalizedString(@"TimeElapsed", @""), sec/60, sec%60];
	}
	else
	{
		int sec = (int)(([positionSlider maxValue] - pos)/1000.0);
		text = [NSString stringWithFormat:NSLocalizedString(@"TimeRemaining", @""), sec/60, sec%60];
	}
	[timeField setStringValue:text];
}	

- (IBAction)toggleShowTimeRemaining:(id)sender
{
	showTimeRemaining = !showTimeRemaining;

	[self updateTimeField:[positionSlider doubleValue]];
}

- (void)delegateRequestNextEntry:(PlaylistEntry *)curEntry
{
	PlaylistEntry *pe;
	pe = [playlistController entryAtIndex:[curEntry index]+1];
	
	if (pe == nil)
		[soundController setNextEntry:nil];
	else
	{
		DBLog(@"NEXT SONG: %@", [pe filename]);
		[soundController setNextEntry:pe];
	}
}

- (void)delegateNotifySongChanged:(PlaylistEntry *)pe
{
	[playlistController setCurrentEntry:pe];
	
	[positionSlider setMaxValue:[pe length]];
	[positionSlider setDoubleValue:0.0f];

	[self updateTimeField:0.0f];
	
}

- (void)delegateNotifyBitrateUpdate:(float)bitrate
{
	//		[bitrateField setIntValue:bitrate];
}

- (void)updatePosition:(id)sender
{
	double pos = [soundController amountPlayed];

	if ([positionSlider tracking] == NO)
	{
		//		DBLog(@"Received pos update: %f", pos);
		[positionSlider setDoubleValue:pos];
		[self updateTimeField:pos];
	}
	
}

- (void)delegateNotifyStatusUpdate:(NSNumber *)s
{
	int status = [s intValue];
	if (status == kCogStatusStopped || status == kCogStatusPaused)
	{
		DBLog(@"INVALIDATING");
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
	
	playbackStatus = status;
}

@end
