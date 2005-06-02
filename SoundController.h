/* SoundController */

#import <Cocoa/Cocoa.h>

#import "Sound.h"
#import "PlaylistController.h"
#import "TrackingSlider.h"

@interface SoundController : NSObject
{
    IBOutlet PlaylistController *playlistController;
	IBOutlet TrackingSlider *positionSlider;
	IBOutlet NSTextField *timeField;
	IBOutlet NSTextField *lengthField;
	IBOutlet NSTextField *bitrateField;
	BOOL waitingForPlay; //No sneaky changing on us
	Sound *sound;
	
	//For communication with the sound
	NSPort *sendPort; 
	NSPort *distantPort; 
}

- (IBAction)pauseResume:(id)sender;

- (IBAction)pause:(id)sender;
- (IBAction)resume:(id)sender;
- (IBAction)stop:(id)sender;

- (IBAction)next:(id)sender;
- (IBAction)prev:(id)sender;
- (IBAction)seek:(id)sender;

- (void)playEntryAtIndex:(int)i;
- (void)playEntry:(PlaylistEntry *)pe;

- (void)sendPortMessage:(int)msgid;
- (void)sendPortMessage:(int)msgid withData:(void *)data ofSize:(int)size;
- (void)sendPortMessage:(int)msgid withString:(NSString *)s;
- (void)handlePortMessage:(NSPortMessage *)portMessage;

@end
