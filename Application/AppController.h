/* AppController */

#import <Cocoa/Cocoa.h>

@class FileTreeViewController;
@class PlaybackController;
@class PlaylistController;
@class PlaylistView;
@class PlaylistLoader;
@class PreferencesController;

@interface AppController : NSObject {
	IBOutlet NSObjectController *currentEntryController;

	IBOutlet PlaybackController *playbackController;

	IBOutlet PlaylistController *playlistController;
	IBOutlet PlaylistLoader *playlistLoader;

	IBOutlet NSWindow *mainWindow;
	IBOutlet NSWindow *miniWindow;
	IBOutlet NSSplitView *mainView;

	IBOutlet NSSegmentedControl *playbackButtons;
	IBOutlet NSButton *fileButton;
	IBOutlet NSButton *shuffleButton;
	IBOutlet NSButton *repeatButton;
	IBOutlet NSButton *randomizeButton;

	IBOutlet NSTextField *totalTimeField;

	IBOutlet PlaylistView *playlistView;

	IBOutlet NSMenuItem *showIndexColumn;
	IBOutlet NSMenuItem *showTitleColumn;
	IBOutlet NSMenuItem *showAlbumArtistColumn;
	IBOutlet NSMenuItem *showArtistColumn;
	IBOutlet NSMenuItem *showAlbumColumn;
	IBOutlet NSMenuItem *showGenreColumn;
	IBOutlet NSMenuItem *showPlayCountColumn;
	IBOutlet NSMenuItem *showLengthColumn;
	IBOutlet NSMenuItem *showTrackColumn;
	IBOutlet NSMenuItem *showYearColumn;

	IBOutlet NSMenu *dockMenu;
	IBOutlet NSMenuItem *currentArtistItem;

	IBOutlet NSWindowController *spotlightWindowController;

	IBOutlet FileTreeViewController *fileTreeViewController;

	IBOutlet PreferencesController *preferencesController;

	NSOperationQueue *queue; // Since we are the app delegate, we take care of the op queue

	NSMutableSet *expandedNodes;

	BOOL miniMode;
}

@property(strong) IBOutlet NSButton *infoButton;
@property(strong) IBOutlet NSButton *infoButtonMini;

- (IBAction)openURL:(id)sender;

- (IBAction)openFiles:(id)sender;
- (IBAction)delEntries:(id)sender;
- (IBAction)savePlaylist:(id)sender;
- (IBAction)savePlaylistFromSelection:(id)sender;

- (IBAction)privacyPolicy:(id)sender;

- (IBAction)feedback:(id)sender;

- (void)initDefaults;

// Fun stuff
- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag;
- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename;
- (void)application:(NSApplication *)theApplication openFiles:(NSArray *)filenames;

- (void)registerHotKeys;

- (void)clickPlay;
- (void)clickPause;
- (void)clickStop;
- (void)clickPrev;
- (void)clickNext;
- (void)clickSpam;
- (void)clickSeek:(NSTimeInterval)position;

- (IBAction)increaseFontSize:(id)sender;
- (IBAction)decreaseFontSize:(id)sender;
- (void)changeFontSize:(float)size;

- (void)nodeExpanded:(NSNotification *)notification;
- (void)nodeCollapsed:(NSNotification *)notification;

- (IBAction)toggleMiniMode:(id)sender;
- (IBAction)toggleToolbarStyle:(id)sender;

- (BOOL)pathSuggesterEmpty;
+ (BOOL)globalPathSuggesterEmpty;
- (void)showPathSuggester;
+ (void)globalShowPathSuggester;

- (void)selectTrack:(id)sender;

- (IBAction)showRubberbandSettings:(id)sender;
+ (void)globalShowRubberbandSettings;

@property NSWindow *mainWindow;
@property NSWindow *miniWindow;

@property BOOL miniMode;

@property(nonatomic) BOOL floatingMiniWindow;

@end
