/* AppController */

#import <Cocoa/Cocoa.h>

@class FileTreeViewController;
@class PlaybackController;
@class PlaylistController;
@class PlaylistView;
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
	IBOutlet NSMenuItem *showLengthColumn;
	IBOutlet NSMenuItem *showTrackColumn;
	IBOutlet NSMenuItem *showYearColumn;
	
    IBOutlet NSWindowController *spotlightWindowController;
    
    IBOutlet FileTreeViewController *fileTreeViewController;

    NSOperationQueue *queue; // Since we are the app delegate, we take care of the op queue
    
    NSMutableSet* expandedNodes;
    
    BOOL miniMode;
}

@property (strong) IBOutlet NSButton *infoButton;
@property (strong) IBOutlet NSButton *infoButtonMini;

- (IBAction)openURL:(id)sender;

- (IBAction)openFiles:(id)sender;
- (IBAction)delEntries:(id)sender;
- (IBAction)savePlaylist:(id)sender;

- (IBAction)openLiberapayPage:(id)sender;
- (IBAction)openPaypalPage:(id)sender;
- (IBAction)openBitcoinPage:(id)sender;
- (IBAction)openPatreonPage:(id)sender;
- (IBAction)openKofiPage:(id)sender;

- (IBAction)feedback:(id)sender;

- (void)initDefaults;

	//Fun stuff
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
- (void)clickSeek: (NSTimeInterval)position;

- (IBAction)increaseFontSize:(id)sender;
- (IBAction)decreaseFontSize:(id)sender;
- (void)changeFontSize:(float)size;

- (void)nodeExpanded:(NSNotification*)notification;
- (void)nodeCollapsed:(NSNotification*)notification;

- (IBAction)toggleMiniMode:(id)sender;
- (IBAction)toggleToolbarStyle:(id)sender;

@property BOOL miniMode;

@property (nonatomic) BOOL floatingMiniWindow;

@end
