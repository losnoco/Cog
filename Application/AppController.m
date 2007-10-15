#import "AppController.h"
#import "KFTypeSelectTableView.h""
#import "PlaybackController.h"
#import "PlaylistController.h"
#import "PlaylistView.h"
#import "FileOutlineView.h"
#import "NDHotKeyEvent.h"
#import "AppleRemote.h"
#import "PlaylistLoader.h"
#import "OpenURLPanel.h"

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
	
	[p beginSheetForDirectory:nil file:nil types:[playlistLoader acceptableFileTypes] modalForWindow:mainWindow modalDelegate:self didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:) contextInfo:NULL];
}

- (void)openPanelDidEnd:(NSOpenPanel *)panel returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
	if (returnCode == NSOKButton)
	{
		[playlistLoader addURLs:[panel URLs] sort:YES];
	}
}

- (IBAction)savePlaylist:(id)sender
{
	NSSavePanel *p;
	
	p = [NSSavePanel savePanel];
	
	[p setAllowedFileTypes:[playlistLoader acceptablePlaylistTypes]];
	[p beginSheetForDirectory:nil file:nil modalForWindow:mainWindow modalDelegate:self didEndSelector:@selector(savePanelDidEnd:returnCode:contextInfo:) contextInfo:NULL];
}

- (void)savePanelDidEnd:(NSSavePanel *)panel returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
	if (returnCode == NSOKButton)
	{
		[playlistLoader save:[panel filename]];
	}
}

- (IBAction)openURL:(id)sender
{
	OpenURLPanel *p;
	
	p = [OpenURLPanel openURLPanel];

	[p beginSheetWithWindow:mainWindow delegate:self didEndSelector:@selector(openURLPanelDidEnd:returnCode:contextInfo:) contextInfo:nil];
}

- (void)openURLPanelDidEnd:(OpenURLPanel *)panel returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
	if (returnCode == NSOKButton)
	{
		[playlistLoader addURLs:[NSArray arrayWithObject:[panel url]] sort:NO];
	}
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
	return [key isEqualToString:@"currentEntry"] ||  [key isEqualToString:@"play"];
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
	[mainWindow setExcludedFromWindowsMenu:YES];
	
	[playButton setToolTip:NSLocalizedString(@"PlayButtonTooltip", @"")];
	[prevButton setToolTip:NSLocalizedString(@"PrevButtonTooltip", @"")];
	[nextButton setToolTip:NSLocalizedString(@"NextButtonTooltip", @"")];
	[infoButton setToolTip:NSLocalizedString(@"InfoButtonTooltip", @"")];
	[shuffleButton setToolTip:NSLocalizedString(@"ShuffleButtonTooltip", @"")];
	[repeatButton setToolTip:NSLocalizedString(@"RepeatButtonTooltip", @"")];
	[fileButton setToolTip:NSLocalizedString(@"FileButtonTooltip", @"")];
	
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
	
	
	NSString *filename = @"~/Library/Application Support/Cog/Default.m3u";
	[playlistLoader addURL:[NSURL fileURLWithPath:[filename stringByExpandingTildeInPath]]];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
	[playbackController stop:self];
	
	NSFileManager *fileManager = [NSFileManager defaultManager];
	NSString *folder = @"~/Library/Application Support/Cog/";
	
	folder = [folder stringByExpandingTildeInPath];
	
	if ([fileManager fileExistsAtPath: folder] == NO)
	{
		[fileManager createDirectoryAtPath: folder attributes: nil];
	}
	
	NSString *fileName = @"Default.m3u";
	
	[playlistLoader saveM3u:[folder stringByAppendingPathComponent: fileName]];
	
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag
{
	if (flag == NO)
		[mainWindow makeKeyAndOrderFront:self];
	
	return NO;
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
	[playlistLoader addURLs:[NSArray arrayWithObject:[NSURL fileURLWithPath:filename]] sort:NO];

	return YES;
}

- (void)application:(NSApplication *)theApplication openFiles:(NSArray *)filenames
{
	//Need to convert to urls
	NSMutableArray *urls = [NSMutableArray array];
	NSEnumerator *e = [filenames objectEnumerator];
	NSString *filename;
	
	while (filename = [e nextObject])
	{
		[urls addObject:[NSURL fileURLWithPath:filename]];
	}
	[playlistLoader addURLs:urls sort:YES];
	
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

	[userDefaultsValuesDict setObject:@"http://cogx.org/appcast/stable.xml" forKey:@"SUFeedURL"];

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
		[fileTreeDataSource setRootPath:[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"fileDrawerRootPath"]];
	}
	else if ([keyPath isEqualToString:@"values.remoteEnabled"] || [keyPath isEqualToString:@"values.remoteOnlyOnActive"]) {
		if([[NSUserDefaults standardUserDefaults] boolForKey:@"remoteEnabled"]) {
			BOOL onlyOnActive = [[NSUserDefaults standardUserDefaults] boolForKey:@"remoteOnlyOnActive"];
			if (!onlyOnActive || [NSApp isActive]) {
				[remote startListening: self];
			}
			if (onlyOnActive && ![NSApp isActive]) { //Setting a preference without being active? *shrugs*
				[remote stopListening: self]; 
			}
		}
		else {
			[remote stopListening: self]; 
		}
	}
}

- (void)registerHotKeys
{
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
	[prevButton performClick:nil];
}

- (void)clickNext
{
	[nextButton performClick:nil];
}


@end
