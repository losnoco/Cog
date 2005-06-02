/* AppController */

#import <Cocoa/Cocoa.h>

#import "PlaylistController.h"

@interface AppController : NSObject
{
    IBOutlet PlaylistController *playlistController;
	IBOutlet NSPanel *infoPanel;
	IBOutlet NSWindow *mainWindow;
}
- (IBAction)addFiles:(id)sender;
- (IBAction)delEntries:(id)sender;
- (IBAction)showInfo:(id)sender;
- (IBAction)savePlaylist:(id)sender;
- (IBAction)savePlaylistAs:(id)sender;
- (IBAction)loadPlaylist:(id)sender;

- (void)openPanelDidEnd:(NSOpenPanel *)panel returnCode:(int)returnCode contextInfo:(void *)contextInfo;

//Fun stuff
- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag;
- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename;
- (void)application:(NSApplication *)theApplication openFiles:(NSArray *)filenames;

@end
