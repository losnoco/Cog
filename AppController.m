#import "AppController.h"

@implementation AppController

- (IBAction)openFiles:(id)sender
{
	NSOpenPanel *p;
	
	p = [NSOpenPanel openPanel];
	
	[p setCanChooseDirectories:YES];
	[p setAllowsMultipleSelection:YES];
	
	[p beginSheetForDirectory:nil file:nil types:[playlistController acceptableFileTypes] modalForWindow:mainWindow modalDelegate:self didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:) contextInfo:NULL];
//	[p beginForDirectory:nil file:nil types:[playlistController acceptableFileTypes] modelessDelegate:self didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:) contextInfo:nil];
	
/*	if ([p runModalForTypes:[playlistController acceptableFileTypes]] == NSOKButton)
	{
		[playlistController addPaths:[p filenames] sort:YES];
	}
*/	
}

- (void)openPanelDidEnd:(NSOpenPanel *)panel returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
	if (returnCode == NSOKButton)
	{
		[playlistController addPaths:[panel filenames] sort:YES];
	}
	
//	[panel release];
}

- (IBAction)delEntries:(id)sender
{
	[playlistController remove:self];
}

- (IBAction)addFiles:(id)sender
{
	NSMutableArray *paths = [[NSMutableArray alloc] init];
	NSArray *nodes = [fileTreeController selectedObjects];
	NSEnumerator *e = [nodes objectEnumerator];
	
	id n;
	while (n = [e nextObject]) {
		[paths addObject:[n path]];
	}
	
	[playlistController addPaths:paths sort:YES];
	[paths release];
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
	[self initDefaults];
	
	//	DBLog(@"AWAKe");
	[playButton setToolTip:NSLocalizedString(@"PlayButtonTooltip", @"")];
	[stopButton setToolTip:NSLocalizedString(@"StopButtonTooltip", @"")];
	[prevButton setToolTip:NSLocalizedString(@"PrevButtonTooltip", @"")];
	[nextButton setToolTip:NSLocalizedString(@"NextButtonTooltip", @"")];
	[addButton setToolTip:NSLocalizedString(@"AddButtonTooltip", @"")];
	[remButton setToolTip:NSLocalizedString(@"RemoveButtonTooltip", @"")];
	[infoButton setToolTip:NSLocalizedString(@"InfoButtonTooltip", @"")];
	[shuffleButton setToolTip:NSLocalizedString(@"ShuffleButtonTooltip", @"")];
	[repeatButton setToolTip:NSLocalizedString(@"RepeatButtonTooltip", @"")];
	
	[self registerHotKeys];
	
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

	[mainWindow makeKeyAndOrderFront:self];
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag
{
	//	if (flag == NO)
	[mainWindow makeKeyAndOrderFront:self];
	
	return NO;
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
	DBLog(@"Adding path: %@", filename);
	[playlistController addPaths:[NSArray arrayWithObject:filename] sort:NO];

	return YES;
}

- (void)application:(NSApplication *)theApplication openFiles:(NSArray *)filenames
{
	DBLog(@"Adding paths: %@", filenames);
	
	[playlistController addPaths:filenames sort:YES];
	[theApplication replyToOpenOrPrint:NSApplicationDelegateReplySuccess];
}

- (IBAction)toggleInfoDrawer:(id)sender
{
	[mainWindow makeKeyAndOrderFront:self];
	
	[infoDrawer toggle:self];
}

- (IBAction)toggleFileDrawer:(id)sender
{
	[mainWindow makeKeyAndOrderFront:self];
	
	[fileDrawer toggle:self];
}

- (void)drawerDidOpen:(NSNotification *)notification
{
	if ([notification object] == infoDrawer)
		[infoButton setState:NSOnState];
	else if ([notification object] == fileDrawer)
		[fileButton setState:NSOnState];
}

- (void)drawerDidClose:(NSNotification *)notification
{
	if ([notification object] == infoDrawer)
		[infoButton setState:NSOffState];
	else if ([notification object] == fileDrawer)
		[fileButton setState:NSOffState];
}

- (IBAction)donate:(id)sender
{
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://sourceforge.net/project/project_donations.php?group_id=140003"]];
}

- (void)initDefaults
{
	NSMutableDictionary *userDefaultsValuesDict = [NSMutableDictionary dictionary];
	[userDefaultsValuesDict setObject:[NSNumber numberWithInt:35] forKey:@"hotKeyPlayKeyCode"];
	[userDefaultsValuesDict setObject:[NSNumber numberWithInt:(NSControlKeyMask|NSCommandKeyMask)] forKey:@"hotKeyPlayModifiers"];
	[userDefaultsValuesDict setObject:[NSNumber numberWithInt:'P'] forKey:@"hotKeyPlayCharacter"];
	
	[userDefaultsValuesDict setObject:[NSNumber numberWithInt:45] forKey:@"hotKeyNextKeyCode"];
	[userDefaultsValuesDict setObject:[NSNumber numberWithInt:(NSControlKeyMask|NSCommandKeyMask)] forKey:@"hotKeyNextModifiers"];
	[userDefaultsValuesDict setObject:[NSNumber numberWithInt:'N'] forKey:@"hotKeyNextCharacter"];
	
	[userDefaultsValuesDict setObject:[NSNumber numberWithInt:15] forKey:@"hotKeyPreviousKeyCode"];
	[userDefaultsValuesDict setObject:[NSNumber numberWithInt:(NSControlKeyMask|NSCommandKeyMask)] forKey:@"hotKeyPreviousModifiers"];
	[userDefaultsValuesDict setObject:[NSNumber numberWithInt:'R'] forKey:@"hotKeyPreviousCharacter"];

	[userDefaultsValuesDict setObject:[@"~/Music" stringByExpandingTildeInPath] forKey:@"fileDrawerRootPath"];

	//Register and sync defaults
	[[NSUserDefaults standardUserDefaults] registerDefaults:userDefaultsValuesDict];
	[[NSUserDefaults standardUserDefaults] synchronize];

	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.hotKeyPlayKeyCode"		options:0 context:nil];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.hotKeyPreviousKeyCode"	options:0 context:nil];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.hotKeyNextKeyCode"		options:0 context:nil];

	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.fileDrawerRootPath"		options:0 context:nil];
}

- (void) observeValueForKeyPath:(NSString *)keyPath
					   ofObject:(id)object
						 change:(NSDictionary *)change
                        context:(void *)context
{
	if ([keyPath isEqualToString:@"values.hotKeyPlayKeyCode"]) {
		[self registerHotKeys];
	}
	else if ([keyPath isEqualToString:@"values.hotKeyPreviousKeyCode"]) {
		[self registerHotKeys];
	}
	else if ([keyPath isEqualToString:@"values.hotKeyNextKeyCode"]) {
		[self registerHotKeys];
	}
	else if ([keyPath isEqualToString:@"values.fileDrawerRootPath"]) {
		[fileTreeController setRootPath:[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"fileDrawerRootPath"]];
	}
}

- (void)registerHotKeys
{
	NSLog(@"REGISTERING HOTKEYS");
	
	[playHotKey release];
	playHotKey = [[NDHotKeyEvent alloc]
		initWithKeyCode: [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"hotKeyPlayKeyCode"] intValue]
			  character: [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"hotKeyPlayCharacter"] intValue]
		  modifierFlags: [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"hotKeyPlayModifiers"] intValue]
		];
	
	[prevHotKey release];
	prevHotKey = [[NDHotKeyEvent alloc]
		initWithKeyCode: [[NSUserDefaults standardUserDefaults] integerForKey:@"hotKeyPreviousKeyCode"]
				character: [[NSUserDefaults standardUserDefaults] integerForKey:@"hotKeyPreviousCharacter"]
			modifierFlags: [[NSUserDefaults standardUserDefaults] integerForKey:@"hotKeyPreviousModifiers"]
		];
	
	[nextHotKey release];
	nextHotKey = [[NDHotKeyEvent alloc]
		initWithKeyCode: [[NSUserDefaults standardUserDefaults] integerForKey:@"hotKeyNextKeyCode"]
				character: [[NSUserDefaults standardUserDefaults] integerForKey:@"hotKeyNextCharacter"]
			modifierFlags: [[NSUserDefaults standardUserDefaults] integerForKey:@"hotKeyNextModifiers"]
		];
	
	[playHotKey setTarget:self selector:@selector(clickPlay)];
	[prevHotKey setTarget:self selector:@selector(clickPrev)];
	[nextHotKey setTarget:self selector:@selector(clickNext)];
	
	[playHotKey setEnabled:YES];	
	[prevHotKey setEnabled:YES];
	[nextHotKey setEnabled:YES];
}

- (void)clickPlay
{
	[playButton performClick:nil];
}

- (void)clickPrev
{
	NSLog(@"PREV");
	[prevButton performClick:nil];
}

- (void)clickNext
{
	NSLog(@"NEXT");
	[nextButton performClick:nil];
}

- (void)clickStop
{
	[stopButton performClick:nil];
}


@end
