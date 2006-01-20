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

}


- (IBAction)playPauseResume:(id)sender
{
	NSLog(@"PLAYING");
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

	[soundController play:[pe filename]];
}

- (IBAction)next:(id)sender
{
	if ([playlistController next] == nil)
		return;
	
	[self playEntry:[playlistController currentEntry]];
}

- (IBAction)prev:(id)sender
{
	if ([playlistController prev] == nil)
		return;

	[self playEntry:[playlistController currentEntry]];
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
	float v = (float)[sender doubleValue];
	[soundController  setVolume:v];
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

- (void)delegateRequestNextSong:(int)queueSize
{
	PlaylistEntry *pe;
	
	pe = [playlistController entryAtOffset:(queueSize+1)];
	
	if (pe == nil)
		[soundController setNextSong:nil];
	else
		[soundController setNextSong:[pe filename]];
}

- (void)delegateNotifySongChanged:(double)length
{
	[playlistController next];
	
//	[positionSlider setMaxValue:length];
//	[positionSlider setDoubleValue:0];

//	[self updateTimeField:0.0f];
	
}

- (void)delegateNotifyBitrateUpdate:(float)bitrate
{
	//		[bitrateField setIntValue:bitrate];
}

- (void)delegateNotifyPositionUpdate:(double)pos
{
	if ([positionSlider tracking] == NO)
	{
		//		DBLog(@"Received pos update: %f", pos);
		[positionSlider setDoubleValue:pos];
		[self updateTimeField:pos];
	}
	
}

- (void)delegateNotifyStatusUpdate:(int)status
{
	if (status == kCogStatusStopped || status == kCogStatusPaused)
	{
		//Show play image
		[self changePlayButtonImage:@"play"];
	}
	else if (status == kCogStatusPlaying)
	{
		//Show pause
		[self changePlayButtonImage:@"pause"];
	}
	
	playbackStatus = status;
}

@end
