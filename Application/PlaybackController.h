/* PlaybackController */

#import <Cocoa/Cocoa.h>

#import "AppController.h"
#import "CogAudio/AudioPlayer.h"
#import "CogAudio/Status.h"
#import "TrackingSlider.h"

#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>
#import <CoreAudio/CoreAudioTypes.h>

#import "EqualizerWindowController.h"

#import "PlaylistEntry.h"

#define DEFAULT_VOLUME_DOWN 5
#define DEFAULT_VOLUME_UP DEFAULT_VOLUME_DOWN

#define DEFAULT_SPEED_DOWN 0.2
#define DEFAULT_SPEED_UP DEFAULT_SPEED_DOWN

extern NSString *CogPlaybackDidBeginNotificiation;
extern NSString *CogPlaybackDidPauseNotificiation;
extern NSString *CogPlaybackDidResumeNotificiation;
extern NSString *CogPlaybackDidStopNotificiation;

extern NSDictionary *makeRGInfo(PlaylistEntry *pe);

@class PlaylistController;
@class PlaylistView;
@class PlaylistLoader;

@interface PlaybackController : NSObject {
	IBOutlet AppController *appController;

	IBOutlet PlaylistController *playlistController;
	IBOutlet PlaylistView *playlistView;
	IBOutlet PlaylistLoader *playlistLoader;

	IBOutlet EqualizerWindowController *equalizerWindowController;

	IBOutlet NSSlider *volumeSlider;
	IBOutlet NSSlider *speedSlider;

	IBOutlet NSArrayController *outputDevices;

	NSTimer *positionTimer;

	AudioPlayer *audioPlayer;

	CogStatus playbackStatus;
	double position;
	double lastPosition;
	BOOL seekable;
	BOOL fading;

	// progress bar display
	NSProgress *progressOverall;
	NSProgress *progressJob;

	AudioUnit _eq;
}

@property CogStatus playbackStatus;

@property NSProgress *progressOverall;
@property NSProgress *progressJob;

- (IBAction)changeVolume:(id)sender;
- (IBAction)volumeDown:(id)sender;
- (IBAction)volumeUp:(id)sender;

- (IBAction)changeSpeed:(id)sender;
- (IBAction)speedDown:(id)sender;
- (IBAction)speedUp:(id)sender;

- (IBAction)playPauseResume:(id)sender;
- (IBAction)pauseResume:(id)sender;
- (IBAction)skipToNextAlbum:(id)sender;
- (IBAction)skipToPreviousAlbum:(id)sender;

- (IBAction)play:(id)sender;
- (IBAction)pause:(id)sender;
- (IBAction)resume:(id)sender;
- (IBAction)stop:(id)sender;

- (IBAction)next:(id)sender;
- (IBAction)prev:(id)sender;
- (IBAction)seek:(id)sender;
- (IBAction)seek:(id)sender toTime:(NSTimeInterval)time;
- (IBAction)eventSeekForward:(id)sender;
- (void)seekForward:(double)sender;
- (IBAction)eventSeekBackward:(id)sender;
- (void)seekBackward:(double)amount;
- (IBAction)fade:(id)sender;

- (IBAction)spam:(id)sender;

- (void)sendMetaData;

- (void)initDefaults;
- (void)audioFadeDown:(NSTimer *)audioTimer;
- (void)audioFadeUp:(NSTimer *)audioTimer;

- (void)playEntryAtIndex:(NSInteger)i;
- (void)playEntryAtIndex:(NSInteger)i startPaused:(BOOL)paused;
- (void)playEntryAtIndex:(NSInteger)i startPaused:(BOOL)paused andSeekTo:(id)offset;
- (void)playEntry:(PlaylistEntry *)pe;
- (void)playEntry:(PlaylistEntry *)pe startPaused:(BOOL)paused;
- (void)playEntry:(PlaylistEntry *)pe startPaused:(BOOL)paused andSeekTo:(id)offset;

// Playlist notifications
- (void)playlistDidChange:(PlaylistController *)p;

// For bindings

- (void)setPosition:(double)p;
- (double)position;

- (void)setSeekable:(BOOL)s;
- (BOOL)seekable;

@end
