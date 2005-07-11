#import "SoundController.h"
#import "Sound.h"
#import "PlaylistView.h"

#import "DBLog.h"

@implementation SoundController

//Note: use distributed objects for communication between Sound and SoundController....each should be in their own threads

- (id)init
{
	self = [super init];
	if (self)
	{
		sound = [[Sound alloc] init];
		playbackStatus = kCogStatusStopped;
		
		showTimeRemaining = NO;
	}
	
	return self;
}

- (void)awakeFromNib
{
	sendPort = [NSPort port];
	if (sendPort)
	{
		[sendPort setDelegate:self];
		
		NSArray *modes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, nil];
		NSEnumerator *enumerator;
		NSString *mode;
		
		enumerator = [modes objectEnumerator];
		while ((mode = [enumerator nextObject]))
			[[NSRunLoop currentRunLoop] addPort:sendPort forMode:mode];
		
		
		[NSThread detachNewThreadSelector:@selector(launchThreadWithPort:) toTarget:sound withObject:sendPort];
	}
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
//	DBLog(@"Pause/Resume Sent!");
	[self sendPortMessage:kCogPauseResumeMessage];
}

- (IBAction)pause:(id)sender
{
//	DBLog(@"Pause Sent!");
	[self sendPortMessage:kCogPauseMessage];
}

- (IBAction)resume:(id)sender
{
//	DBLog(@"Resume Sent!");

	[self sendPortMessage:kCogResumeMessage];
}

- (IBAction)stop:(id)sender
{
//	DBLog(@"Stop Sent!");

	waitingForPlay = NO;
	[self sendPortMessage:kCogStopMessage];
}

//called by double-clicking on table
- (void)playEntryAtIndex:(int)i
{
	PlaylistEntry *pe = [[playlistController arrangedObjects] objectAtIndex:i];

	[playlistController setCurrentEntry:pe addToHistory:YES];
	[playlistController reset];

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
	waitingForPlay = NO;

//	DBLog(@"PlayEntry: %@ Sent!", [pe filename]);

	[self sendPortMessage:kCogPlayFileMessage withString:[pe filename]];
}

- (IBAction)next:(id)sender
{
	waitingForPlay = NO;
	if ([playlistController nextEntry] == nil)
		return;
	
	[playlistController next];
	[self playEntry:[playlistController currentEntry]];
}

- (IBAction)prev:(id)sender
{
	waitingForPlay = NO;
	if ([playlistController prevEntry] == nil)
		return;

	[playlistController prev];
	[self playEntry:[playlistController currentEntry]];
}

- (IBAction)seek:(id)sender
{
//	DBLog(@"SEEKING?");
	double time;
	time = [positionSlider doubleValue];
	
	if ([sender tracking] == NO) // check if user stopped sliding  before playing audio
        [self sendPortMessage:kCogSeekMessage withData:&time ofSize: (sizeof(double))];
	
	[self updateTimeField:time];
}

- (void)sendPortMessage:(int)msgid
{
	NSPortMessage *portMessage = [[NSPortMessage alloc] initWithSendPort:distantPort receivePort:sendPort components:nil];
	
	if (portMessage)
	{
		[portMessage setMsgid:msgid];
		[portMessage sendBeforeDate:[NSDate date]];
		[portMessage release];
	}	
}

- (void)sendPortMessage:(int)msgid withData:(void *)data ofSize:(int)size
{
	NSPortMessage *portMessage;
	NSData *d = [[NSData alloc] initWithBytes:data length:size];
	NSArray *a = [[NSArray alloc] initWithObjects:d,nil];
	portMessage = [[NSPortMessage alloc] initWithSendPort:distantPort receivePort:sendPort components:a];
	
	[a release];
	[d release];
	
	if (portMessage)
	{
		NSDate *date = [[NSDate alloc] init];
		
		[portMessage setMsgid:msgid];
		[portMessage sendBeforeDate:date];
		
		[date release];
		[portMessage release];
	}
	
}

- (void)sendPortMessage:(int)msgid withString:(NSString *)s
{
	NSData *dataString = [s dataUsingEncoding:NSUTF8StringEncoding allowLossyConversion:NO];

	NSPortMessage *portMessage = [[NSPortMessage alloc] initWithSendPort:distantPort receivePort:sendPort components:[NSArray arrayWithObject:dataString]];
	
	if (portMessage)
	{
		[portMessage setMsgid:msgid];
		[portMessage sendBeforeDate:[NSDate date]];

		[portMessage release];
	}
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
	float v = (float)[sender floatValue];
	[self sendPortMessage:kCogSetVolumeMessage withData:&v ofSize:sizeof(float)];
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

- (void)handlePortMessage:(NSPortMessage *)portMessage
{
	
    unsigned int message = [portMessage msgid];

    if (message == kCogCheckinMessage)
    {
        // Get the worker thread’s communications port.
		DBLog(@"CHECKIN RECEIVED");
        distantPort = [portMessage sendPort];

        // Retain and save the worker port for later use.
		[distantPort retain];
    }
    else if (message == kCogRequestNextFileMessage)
    {
		PlaylistEntry *pe;
		
		pe = [playlistController nextEntry];

		if (pe == nil)
		{
			[self sendPortMessage:kCogEndOfPlaylistMessage];
		}
		else
		{
			DBLog(@"REQUESTING");
			waitingForPlay = YES;
			
			[self sendPortMessage:kCogChangeFileMessage withString:[pe filename]];
		}
	}
	else if (message == kCogFileChangedMessage)
	{
		DBLog(@"FILE CHANGED");
		if (waitingForPlay == YES)
		{
			waitingForPlay = NO;
			[playlistController next];
			[self updateTimeField:0.0f];
		}
	}
	else if (message == kCogBitrateUpdateMessage)
	{
		NSArray* components = [portMessage components];
		NSData *data = [components objectAtIndex:0];
		
		int bitrate;
		bitrate = (*(int *)[data bytes]);
		//		DBLog(@"Received length update: %f", max);
//		[bitrateField setIntValue:bitrate];
	}
	else if (message == kCogLengthUpdateMessage)
	{
		NSArray* components = [portMessage components];
		NSData *data = [components objectAtIndex:0];
		
		double max;
		max = (*(double *)[data bytes]);
//		DBLog(@"Received length update: %f", max);
		[positionSlider setMaxValue:max];
		[positionSlider setDoubleValue:0];
//		DBLog(@"Length changed: %f", max);
//		[lengthField setDoubleValue:max/1000.0];
		[self updateTimeField:0.0f];
	}
	else if (message == kCogPositionUpdateMessage)
	{
		NSArray* components = [portMessage components];
		NSData *data = [components objectAtIndex:0];

		double pos;
		pos = (*(double *)[data bytes]);

		if ([positionSlider tracking] == NO)
		{
			//		DBLog(@"Received pos update: %f", pos);
			[positionSlider setDoubleValue:pos];
			[self updateTimeField:pos];
		}
	}
	else if (message == kCogStatusUpdateMessage)
	{
		NSArray* components = [portMessage components];
		NSData *data = [components objectAtIndex:0];
		
		int s;
		s = (*(int *)[data bytes]);
		
		playbackStatus = s;
		DBLog(@"STATUS UPDATE: %i", s);
		
		if (s == kCogStatusStopped || s == kCogStatusPaused)
		{
			//Show play image
			[self changePlayButtonImage:@"play"];
		}
		else if (s == kCogStatusPlaying)
		{
			//Show pause
			[self changePlayButtonImage:@"pause"];
		}
	}
}

@end
