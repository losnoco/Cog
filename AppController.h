/* AppController */

#import <Cocoa/Cocoa.h>

#import "PlaylistController.h"
#import "FileTreeController.h"
#import "NDHotKeyEvent.h"


@interface AppController : NSObject
{
    IBOutlet PlaylistController *playlistController;
	IBOutlet NSPanel *mainWindow;
	
	IBOutlet NSButton *playButton;
	IBOutlet NSButton *stopButton;
	IBOutlet NSButton *prevButton;
	IBOutlet NSButton *nextButton;
	IBOutlet NSButton *addButton;
	IBOutlet NSButton *remButton;
	IBOutlet NSButton *infoButton;
	IBOutlet NSButton *fileButton;
	IBOutlet NSButton *shuffleButton;
	IBOutlet NSButton *repeatButton;
	
	IBOutlet NSDrawer *infoDrawer;
	IBOutlet NSDrawer *fileDrawer;

	IBOutlet FileTreeController *fileTreeController;
	
	NDHotKeyEvent *playHotKey;
	NDHotKeyEvent *prevHotKey;
	NDHotKeyEvent *nextHotKey;
}

- (IBAction)openFiles:(id)sender;
- (IBAction)delEntries:(id)sender;
- (IBAction)savePlaylist:(id)sender;
- (IBAction)savePlaylistAs:(id)sender;
- (IBAction)loadPlaylist:(id)sender;

- (IBAction)addFiles:(id)sender;

- (void)openPanelDidEnd:(NSOpenPanel *)panel returnCode:(int)returnCode contextInfo:(void *)contextInfo;

- (IBAction)donate:(id)sender;

- (IBAction)toggleInfoDrawer:(id)sender;
- (IBAction)toggleFileDrawer:(id)sender;
- (void)drawerDidOpen:(NSNotification *)notification;
- (void)drawerDidClose:(NSNotification *)notification;

	//Fun stuff
- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag;
- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename;
- (void)application:(NSApplication *)theApplication openFiles:(NSArray *)filenames;

- (void)registerHotKeys;
OSStatus handleHotKey(EventHandlerCallRef nextHandler,EventRef theEvent,void *userData);

@end
