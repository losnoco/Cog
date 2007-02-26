#import "AppController.h"
#import "KFTypeSelectTableView.h""

@implementation AppController

- (id)init
{
	self = [super init];
	if (self)
	{
		[self initDefaults];
		
		/* Use KFTypeSelectTableView as our NSTableView base class to allow type-select searching of all
	     * table and outline views.
		*/
		[[KFTypeSelectTableView class] poseAsClass:[NSTableView class]];
		
		remote = [[AppleRemote alloc] init];
		[remote setDelegate: self];
	}
	
	return self; 
}

// Listen to the remote in exclusive mode, only when Cog is the active application
- (void)applicationDidBecomeActive:(NSNotification *)notification
{
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"remoteEnabled"] && [[NSUserDefaults standardUserDefaults] boolForKey:@"remoteOnlyOnActive"]) {
		[remote startListening: self];
	}
}
- (void)applicationDidResignActive:(NSNotification *)motification
{
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"remoteEnabled"] && [[NSUserDefaults standardUserDefaults] boolForKey:@"remoteOnlyOnActive"]) {
		[remote stopListening: self];
	}
}

/* Helper method for the remote control interface in order to trigger forward/backward and volume
increase/decrease as long as the user holds the left/right, plus/minus button */
- (void) executeHoldActionForRemoteButton: (NSNumber*) buttonIdentifierNumber 
{
    if (remoteButtonHeld) 
    {
        switch([buttonIdentifierNumber intValue]) 
        {
            case kRemoteButtonRight_Hold:       
				//Seek forward?
				break;
            case kRemoteButtonLeft_Hold:
				//Seek back
				break;
            case kRemoteButtonVolume_Plus_Hold:
                //Volume Up
				[playbackController volumeUp: self];
				break;
            case kRemoteButtonVolume_Minus_Hold:
                //Volume Down
				[playbackController volumeDown: self];
				break;              
        }
        if (remoteButtonHeld) 
        {
            /* trigger event */
            [self performSelector:@selector(executeHoldActionForRemoteButton:) 
					   withObject:buttonIdentifierNumber
					   afterDelay:0.25];         
        }
    }
}

/* Apple Remote callback */
- (void) appleRemoteButton: (AppleRemoteEventIdentifier)buttonIdentifier 
               pressedDown: (BOOL) pressedDown 
                clickCount: (unsigned int) count 
{
    switch( buttonIdentifier )
    {
        case kRemoteButtonPlay:
			[self clickPlay];

            break;
        case kRemoteButtonVolume_Plus:
			[playbackController volumeUp: self];
            break;
        case kRemoteButtonVolume_Minus:
			[playbackController volumeDown: self];
            break;
        case kRemoteButtonRight:
            [self clickNext];
            break;
        case kRemoteButtonLeft:
            [self clickPrev];
            break;
        case kRemoteButtonRight_Hold:
        case kRemoteButtonLeft_Hold:
        case kRemoteButtonVolume_Plus_Hold:
        case kRemoteButtonVolume_Minus_Hold:
            /* simulate an event as long as the user holds the button */
            remoteButtonHeld = pressedDown;
            if( pressedDown )
            {                
                NSNumber* buttonIdentifierNumber = [NSNumber numberWithInt: buttonIdentifier];  
                [self performSelector:@selector(executeHoldActionForRemoteButton:) 
                           withObject:buttonIdentifierNumber];
            }
				break;
        case kRemoteButtonMenu:
            break;
        default:
            /* Add here whatever you want other buttons to do */
            break;
    }
}



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

- (PlaylistEntry *)currentEntry
{
	return [playlistController currentEntry];
}

- (BOOL)application:(NSApplication *)sender delegateHandlesKey:(NSString *)key
{
	//	DBLog(@"W00t");
	return [key isEqualToString:@"currentEntry"];
}

- (void)initShowColumn:(NSMenuItem *)showColumn withIdentifier:(NSString *)identifier
{
	id tc = [playlistView tableColumnWithIdentifier:identifier];

	NSArray *visibleColumnIdentifiers = [[NSUserDefaults standardUserDefaults] objectForKey:[playlistView columnVisibilitySaveName]];
	if (visibleColumnIdentifiers) {
		NSEnumerator *enumerator = [visibleColumnIdentifiers objectEnumerator];
		id column;
		while (column = [enumerator nextObject]) {
			if ([visibleColumnIdentifiers containsObject:identifier]) {
				[showColumn setState:NSOnState];
			}
			else {
				[showColumn setState:NSOffState];
			}
		}
	}
	
	[showColumn setRepresentedObject: tc];
}

- (void)awakeFromNib
{
	[playButton setToolTip:NSLocalizedString(@"PlayButtonTooltip", @"")];
	[prevButton setToolTip:NSLocalizedString(@"PrevButtonTooltip", @"")];
	[nextButton setToolTip:NSLocalizedString(@"NextButtonTooltip", @"")];
	[infoButton setToolTip:NSLocalizedString(@"InfoButtonTooltip", @"")];
	[shuffleButton setToolTip:NSLocalizedString(@"ShuffleButtonTooltip", @"")];
	[repeatButton setToolTip:NSLocalizedString(@"RepeatButtonTooltip", @"")];
	
	[self initShowColumn: showIndexColumn	withIdentifier: @"index"];
	[self initShowColumn: showTitleColumn	withIdentifier: @"title"];
	[self initShowColumn: showArtistColumn	withIdentifier: @"artist"];
	[self initShowColumn: showAlbumColumn	withIdentifier: @"album"];
	[self initShowColumn: showGenreColumn	withIdentifier: @"genre"];
	[self initShowColumn: showLengthColumn	withIdentifier: @"length"];
	[self initShowColumn: showTrackColumn	withIdentifier: @"track"];
	[self initShowColumn: showYearColumn	withIdentifier: @"year"];

	[self registerHotKeys];
	
	//Init Remote
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"remoteEnabled"] && ![[NSUserDefaults standardUserDefaults] boolForKey:@"remoteOnlyOnActive"]) {
		[remote startListening:self];
	}
	
	
	NSString *filename = @"~/Library/Application Support/Cog/Default.playlist";
	[playlistController loadPlaylist:[filename stringByExpandingTildeInPath]];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
	[playbackController stop:self];
	
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
	if ([notification object] == infoDrawer) {
		[infoButton setState:NSOnState];
	}
	else if ([notification object] == fileDrawer) {
		[fileButton setState:NSOnState];
		
		[mainWindow makeFirstResponder: fileOutlineView];
	}
}

- (void)drawerDidClose:(NSNotification *)notification
{
	if ([notification object] == infoDrawer) {
		[infoButton setState:NSOffState];
	}
	else if ([notification object] == fileDrawer) {
		NSLog(@"CLOSED");
		[fileButton setState:NSOffState];
		
		[mainWindow makeFirstResponder: playlistView];
	}
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

	[userDefaultsValuesDict setObject:[NSNumber numberWithBool:YES] forKey:@"remoteEnabled"];
	[userDefaultsValuesDict setObject:[NSNumber numberWithBool:YES] forKey:@"remoteOnlyOnActive"];

	[userDefaultsValuesDict setObject:[NSNumber numberWithBool:YES] forKey:@"enableAudioScrobbler"];
	[userDefaultsValuesDict setObject:[NSNumber numberWithBool:YES] forKey:@"automaticallyLaunchLastFM"];

	//Register and sync defaults
	[[NSUserDefaults standardUserDefaults] registerDefaults:userDefaultsValuesDict];
	[[NSUserDefaults standardUserDefaults] synchronize];
	
	//Add observers
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.hotKeyPlayKeyCode"		options:0 context:nil];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.hotKeyPreviousKeyCode"	options:0 context:nil];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.hotKeyNextKeyCode"		options:0 context:nil];

	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.fileDrawerRootPath"		options:0 context:nil];

	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.fileDrawerRootPath"		options:0 context:nil];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.fileDrawerRootPath"		options:0 context:nil];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.fileDrawerRootPath"		options:0 context:nil];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.fileDrawerRootPath"		options:0 context:nil];

	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.remoteEnabled"			options:0 context:nil];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.remoteOnlyOnActive"		options:0 context:nil];
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
	else if ([keyPath isEqualToString:@"values.remoteEnabled"] || [keyPath isEqualToString:@"values.remoteOnlyOnActive"]) {
		if([[NSUserDefaults standardUserDefaults] boolForKey:@"remoteEnabled"]) {
			NSLog(@"Remote enabled...");
			BOOL onlyOnActive = [[NSUserDefaults standardUserDefaults] boolForKey:@"remoteOnlyOnActive"];
			if (!onlyOnActive || [NSApp isActive]) {
				[remote startListening: self];
			}
			if (onlyOnActive && ![NSApp isActive]) { //Setting a preference without being active? *shrugs*
				[remote stopListening: self]; 
			}
		}
		else {
			NSLog(@"DISABLE REMOTE");
			[remote stopListening: self]; 
		}
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


@end
