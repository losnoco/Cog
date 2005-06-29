#import "AppController.h"

@implementation AppController

- (IBAction)addFiles:(id)sender
{
	NSOpenPanel *p;
	
	p = [NSOpenPanel openPanel];
	
	[p setCanChooseDirectories:YES];
	[p setAllowsMultipleSelection:YES];
	
	//	[p beginSheetForDirectory:nil file:nil types:[`listController acceptableFileTypes] modalForWindow:mainWindow modalDelegate:self didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:) contextInfo:NULL];
//	[p beginForDirectory:nil file:nil types:[playlistController acceptableFileTypes] modelessDelegate:self didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:) contextInfo:nil];
	
	if ([p runModalForTypes:[playlistController acceptableFileTypes]] == NSOKButton)
	{
		[playlistController addPaths:[p filenames] sort:NO];
	}

}

- (void)openPanelDidEnd:(NSOpenPanel *)panel returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
	if (returnCode == NSOKButton)
	{
		[playlistController addPaths:[panel filenames] sort:NO];
	}
	
	[panel release];
}

- (IBAction)delEntries:(id)sender
{
	[playlistController remove:self];
}

- (PlaylistEntry *)currentEntry
{
	return [playlistController currentEntry];
}

- (BOOL)application:(NSApplication *)sender delegateHandlesKey:(NSString *)key
{
//	DBLog(@"W00t");
	return [key isEqualToString:@"currentEntry"];
}

- (void)awakeFromNib
{
//	DBLog(@"AWAKe");
	
	[playButton setToolTip:@"Play"];
	[stopButton setToolTip:@"Stop"];
	[prevButton setToolTip:@"Previous"];
	[nextButton setToolTip:@"Next"];
	[addButton setToolTip:@"Add files"];
	[remButton setToolTip:@"Remove selected files"];
	[infoButton setToolTip:@"Information on the selected file."];
	[shuffleButton setToolTip:@"Shuffle mode"];
	[repeatButton setToolTip:@"Repeat mode"];

	
	NSString *filename = @"~/Library/Application Support/Cog/Default.playlist";
	[playlistController loadPlaylist:[filename stringByExpandingTildeInPath]];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
//	DBLog(@"QUITTING");
	NSFileManager *fileManager = [NSFileManager defaultManager];
	NSString *folder = @"~/Library/Application Support/Cog/";
	
	folder = [folder stringByExpandingTildeInPath];
	
	if ([fileManager fileExistsAtPath: folder] == NO)
	{
		[fileManager createDirectoryAtPath: folder attributes: nil];
	}
	
	NSString *fileName = @"Default.playlist";

	[playlistController savePlaylist:[folder stringByAppendingPathComponent: fileName]];

}

- (IBAction)savePlaylist:(id)sender
{
	if ([playlistController playlistFilename] == nil)
		[self savePlaylistAs:sender];
	
	[playlistController savePlaylist:[playlistController playlistFilename]];
}
- (IBAction)savePlaylistAs:(id)sender
{
	NSSavePanel *p;
	
	p = [NSSavePanel savePanel];
	
	[p setAllowedFileTypes:[playlistController acceptablePlaylistTypes]];	
	
	if ([p runModalForDirectory:nil file:[[playlistController playlistFilename] lastPathComponent]] == NSOKButton)
	{
		[playlistController setPlaylistFilename:[p filename]];

		[playlistController savePlaylist:[p filename]];
	}
}
- (IBAction)loadPlaylist:(id)sender
{
	NSOpenPanel *p;
	
	p = [NSOpenPanel openPanel];
	
	[p setCanChooseDirectories:NO];
	[p setAllowsMultipleSelection:NO];
	
	if ([p runModalForTypes:[playlistController acceptablePlaylistTypes]] == NSOKButton)
	{
		[playlistController setPlaylistFilename:[p filename]];
		
		[playlistController loadPlaylist:[p filename]];
	}
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag
{
	//	if (flag == NO)
	[mainWindow makeKeyAndOrderFront:self];
	
	return NO;
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
	if ([playlistController addPaths:[NSArray arrayWithObject:filename] sort:NO] != 1)
		return NO;
	
	return YES;
}

- (void)application:(NSApplication *)theApplication openFiles:(NSArray *)filenames
{
	[playlistController addPaths:filenames sort:YES];
	[theApplication replyToOpenOrPrint:NSApplicationDelegateReplySuccess];
}

@end
