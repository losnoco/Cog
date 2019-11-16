/* AppController */

#import <Cocoa/Cocoa.h>

#import <NDHotKey/NDHotKeyEvent.h>
#import "NowPlayingBarController.h"

@class FileTreeViewController;
@class PlaybackController;
@class PlaylistController;
@class PlaylistView;
@class AppleRemote;
@class PlaylistLoader;

@interface AppController : NSObject
{
    IBOutlet NSObjectController *currentEntryController;
    
	IBOutlet PlaybackController *playbackController;

    IBOutlet PlaylistController *playlistController;
	IBOutlet PlaylistLoader *playlistLoader;
	
	IBOutlet NSWindow *mainWindow;
    IBOutlet NSWindow *miniWindow;
    IBOutlet NSSplitView *mainView;
	
	IBOutlet NSSegmentedControl *playbackButtons;
	IBOutlet NSButton *infoButton;
	IBOutlet NSButton *fileButton;
	IBOutlet NSButton *shuffleButton;
	IBOutlet NSButton *repeatButton;
    IBOutlet NSButton *randomizeButton;

	IBOutlet NSTextField *totalTimeField;
	
	IBOutlet NSDrawer *infoDrawer;

	IBOutlet PlaylistView *playlistView;
	
	IBOutlet NSMenuItem *showIndexColumn;
	IBOutlet NSMenuItem *showTitleColumn;
	IBOutlet NSMenuItem *showArtistColumn;
	IBOutlet NSMenuItem *showAlbumColumn;
	IBOutlet NSMenuItem *showGenreColumn;
	IBOutlet NSMenuItem *showLengthColumn;
	IBOutlet NSMenuItem *showTrackColumn;
	IBOutlet NSMenuItem *showYearColumn;
	
    IBOutlet NSWindowController *spotlightWindowController;
    
    IBOutlet FileTreeViewController *fileTreeViewController;
	
	NDHotKeyEvent *playHotKey;
	NDHotKeyEvent *prevHotKey;
	NDHotKeyEvent *nextHotKey;
    NDHotKeyEvent *spamHotKey;
    
    NowPlayingBarController *nowPlaying;
	
	AppleRemote *remote;
	BOOL remoteButtonHeld; /* true as long as the user holds the left,right,plus or minus on the remote control */
	
    NSOperationQueue *queue; // Since we are the app delegate, we take care of the op queue
    
    NSMutableSet* expandedNodes;
    
    BOOL miniMode;
}

- (IBAction)openURL:(id)sender;

- (IBAction)openFiles:(id)sender;
- (IBAction)delEntries:(id)sender;
- (IBAction)savePlaylist:(id)sender;

- (IBAction)donate:(id)sender;
- (IBAction)feedback:(id)sender;

- (IBAction)toggleInfoDrawer:(id)sender;
- (void)drawerDidOpen:(NSNotification *)notification;
- (void)drawerDidClose:(NSNotification *)notification;

- (void)initDefaults;

	//Fun stuff
- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag;
- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename;
- (void)application:(NSApplication *)theApplication openFiles:(NSArray *)filenames;

- (void)registerHotKeys;
OSStatus handleHotKey(EventHandlerCallRef nextHandler,EventRef theEvent,void *userData);


- (void)clickPlay;
- (void)clickPause;
- (void)clickStop;
- (void)clickPrev;
- (void)clickNext;
- (void)clickSpam;
- (void)clickSeek: (NSTimeInterval)position;

- (IBAction)increaseFontSize:(id)sender;
- (IBAction)decreaseFontSize:(id)sender;
- (void)changeFontSize:(float)size;

- (void)nodeExpanded:(NSNotification*)notification;
- (void)nodeCollapsed:(NSNotification*)notification;

- (void)windowDidEnterFullScreen:(NSNotification *)notification;
- (void)windowDidExitFullScreen:(NSNotification *)notification;

- (IBAction)toggleMiniMode:(id)sender;

@property BOOL miniMode;

@end
