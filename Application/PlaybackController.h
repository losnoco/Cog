/* PlaybackController */

#import <Cocoa/Cocoa.h>

#import "CogAudio/AudioPlayer.h"
#import "CogAudio/Status.h"
#import "TrackingSlider.h"
#import "AudioScrobbler.h"

#define DEFAULT_VOLUME_DOWN 5
#define DEFAULT_VOLUME_UP DEFAULT_VOLUME_DOWN

extern NSString *CogPlaybackDidBeginNotficiation;
extern NSString *CogPlaybackDidPauseNotficiation;
extern NSString *CogPlaybackDidResumeNotficiation;
extern NSString *CogPlaybackDidStopNotficiation;

extern NSDictionary * makeRGInfo(PlaylistEntry *pe);

@class PlaylistController;
@class PlaylistView;
@class PlaylistLoader;

@interface PlaybackController : NSObject
{
    IBOutlet PlaylistController *playlistController;
	IBOutlet PlaylistView *playlistView;
	IBOutlet PlaylistLoader *playlistLoader;
	
	IBOutlet NSSlider *volumeSlider;
	
	IBOutlet NSArrayController *outputDevices;
	
	NSTimer *positionTimer;
		
	AudioPlayer *audioPlayer;
	
	CogStatus playbackStatus;
	double position;
    double lastPosition;
	BOOL seekable;
	BOOL fading;
    
    // progress bar display
    double progressBarStatus;
 }

@property CogStatus playbackStatus;

@property double progressBarStatus;

- (IBAction)changeVolume:(id)sender;
- (IBAction)volumeDown:(id)sender;
- (IBAction)volumeUp:(id)sender;

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

- (IBAction)spam;

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
