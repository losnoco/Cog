/* SoundController */

#import <Cocoa/Cocoa.h>

#import "SoundController.h"
#import "PlaylistController.h"
#import "TrackingSlider.h"

@class PlaylistView;

@interface PlaybackController : NSObject
{
    IBOutlet PlaylistController *playlistController;
	IBOutlet PlaylistView *playlistView;
	
	IBOutlet TrackingSlider *positionSlider;
	IBOutlet NSSlider *volumeSlider;
	IBOutlet NSTextField *timeField;
	IBOutlet NSTextField *lengthField;
	IBOutlet NSTextField *bitrateField;
	
	IBOutlet NSButton *playButton;
	
	NSTimer *positionTimer;
		
	BOOL waitingForPlay; //No sneaky changing on us
	SoundController *soundController;
	
	int playbackStatus;
	
	float currentVolume;
	
	BOOL showTimeRemaining;
 }

- (IBAction)toggleShowTimeRemaining:(id)sender;
- (IBAction)changeVolume:(id)sender;

- (IBAction)playPauseResume:(id)sender;
- (IBAction)pauseResume:(id)sender;

- (IBAction)play:(id)sender;
- (IBAction)pause:(id)sender;
- (IBAction)resume:(id)sender;
- (IBAction)stop:(id)sender;

- (IBAction)next:(id)sender;
- (IBAction)prev:(id)sender;
- (IBAction)seek:(id)sender;


- (void)updateTimeField:(double)pos;

- (void)playEntryAtIndex:(int)i;
- (void)playEntry:(PlaylistEntry *)pe;


//Methods since this is SoundController's delegate
- (void)delegateNotifyStatusUpdate:(NSNumber *)status;
- (void)delegateNotifyBitrateUpdate:(float)bitrate;
- (void)delegateNotifySongChanged;
- (void)delegateRequestNextSong:(PlaylistEntry *)pe;

@end
