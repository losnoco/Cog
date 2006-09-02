#import "AppController.h"

@implementation AppController

- (IBAction)openFiles:(id)sender
{
	NSOpenPanel *p;
	
	p = [NSOpenPanel openPanel];
	
	[p setCanChooseDirectories:YES];
	[p setAllowsMultipleSelection:YES];
	
	[p beginSheetForDirectory:nil file:nil types:[playlistController acceptableFileTypes] modalForWindow:mainWindow modalDelegate:self didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:) contextInfo:NULL];
	[p beginForDirectory:nil file:nil types:[playlistController acceptableFileTypes] modelessDelegate:self didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:) contextInfo:nil];
	
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
	[userDefaultsValuesDict setObject:[NSNumber numberWithInt:35] forKey:@"hotkeyCodePlay"];
	[userDefaultsValuesDict setObject:[NSNumber numberWithInt:controlKey+cmdKey] forKey:@"hotkeyModifiersPlay"];
	
	[userDefaultsValuesDict setObject:[NSNumber numberWithInt:45] forKey:@"hotkeyCodeNext"];
	[userDefaultsValuesDict setObject:[NSNumber numberWithInt:controlKey+cmdKey] forKey:@"hotkeyModifiersNext"];
	
	[userDefaultsValuesDict setObject:[NSNumber numberWithInt:15] forKey:@"hotkeyCodePrevious"];
	[userDefaultsValuesDict setObject:[NSNumber numberWithInt:controlKey+cmdKey] forKey:@"hotkeyModifiersPrevious"];
	
	//Register and sync defaults
	[[NSUserDefaults standardUserDefaults] registerDefaults:userDefaultsValuesDict];
	[[NSUserDefaults standardUserDefaults] synchronize];
}

//Register the Hotkeys.  Added by Chris Henderson, 21 May 2006
//See http://www.dbachrach.com/blog/2005/11/program-global-hotkeys-in-cocoa-easily.html
- (void)registerHotKeys
{
	EventHotKeyRef gMyHotKeyRef;
	EventHotKeyID gMyHotKeyID;
	EventTypeSpec eventType;
	eventType.eventClass=kEventClassKeyboard;
	eventType.eventKind=kEventHotKeyPressed;
	InstallApplicationEventHandler(&handleHotKey,1,&eventType,self,NULL);
	//Play
	gMyHotKeyID.signature='htk1';
	gMyHotKeyID.id=1;
	if([[NSUserDefaults standardUserDefaults] integerForKey:@"hotkeyCodePlay"]!=-999)
	{
		RegisterEventHotKey([[NSUserDefaults standardUserDefaults] integerForKey:@"hotkeyCodePlay"], [[NSUserDefaults standardUserDefaults] integerForKey:@"hotkeyModifiersPlay"], gMyHotKeyID, GetApplicationEventTarget(), 0, &gMyHotKeyRef);
	}
	//Previous
	gMyHotKeyID.signature='htk2';
	gMyHotKeyID.id=2;
	if([[NSUserDefaults standardUserDefaults] integerForKey:@"hotkeyCodePrevious"]!=-999)
	{
		NSLog(@"REGISTERING: %i", [[NSUserDefaults standardUserDefaults] integerForKey:@"hotkeyCodePrevious"]);
		RegisterEventHotKey([[NSUserDefaults standardUserDefaults] integerForKey:@"hotkeyCodePrevious"], [[NSUserDefaults standardUserDefaults] integerForKey:@"hotkeyModifiersPrevious"], gMyHotKeyID, GetApplicationEventTarget(), 0, &gMyHotKeyRef);
	}
	//Next
	gMyHotKeyID.signature='htk3';
	gMyHotKeyID.id=3;
	if([[NSUserDefaults standardUserDefaults] integerForKey:@"hotkeyCodeNext"]!=-999)
	{
		RegisterEventHotKey([[NSUserDefaults standardUserDefaults] integerForKey:@"hotkeyCodeNext"], [[NSUserDefaults standardUserDefaults] integerForKey:@"hotkeyModifiersNext"], gMyHotKeyID, GetApplicationEventTarget(), 0, &gMyHotKeyRef);
	}
}

//Handle the Hotkeys.  Added by Chris Henderson, 21 May 2006
OSStatus handleHotKey(EventHandlerCallRef nextHandler,EventRef theEvent,void *userData)
{
	EventHotKeyID hkID;
	GetEventParameter(theEvent,kEventParamDirectObject,typeEventHotKeyID,NULL,sizeof(hkID),NULL,&hkID);
	int i = hkID.id;

	NSLog(@"Handling: %i", i);
	switch (i) 
	{
		case 1: [userData clickPlay];
			break;
		case 2: [userData clickPrev];
			break;
		case 3: [userData clickNext];
			break;
	}
	return noErr;
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
