#import "AppController.h"
#import "FileTreeController.h"
#import "FileTreeViewController.h"
#import "FileTreeOutlineView.h"
#import "PlaybackController.h"
#import "PlaylistController.h"
#import "PlaylistView.h"
#import "PlaylistEntry.h"
#import <NDHotKey/NDHotKeyEvent.h>
#import "PlaylistLoader.h"
#import "OpenURLPanel.h"
#import "SpotlightWindowController.h"
#import "StringToURLTransformer.h"
#import "FontSizetoLineHeightTransformer.h"
#import "PathNode.h"
#import <CogAudio/Status.h>

#import "Logging.h"
#import "MiniModeMenuTitleTransformer.h"
#import "DualWindow.h"

@implementation AppController

+ (void)initialize
{
    // Register transformers
	NSValueTransformer *stringToURLTransformer = [[StringToURLTransformer alloc] init];
    [NSValueTransformer setValueTransformer:stringToURLTransformer
                                    forName:@"StringToURLTransformer"];
                                
    NSValueTransformer *fontSizetoLineHeightTransformer = 
        [[FontSizetoLineHeightTransformer alloc] init];
    [NSValueTransformer setValueTransformer:fontSizetoLineHeightTransformer
                                    forName:@"FontSizetoLineHeightTransformer"];

    NSValueTransformer *miniModeMenuTitleTransformer = [[MiniModeMenuTitleTransformer alloc] init];
    [NSValueTransformer setValueTransformer:miniModeMenuTitleTransformer
                                    forName:@"MiniModeMenuTitleTransformer"];
}


- (id)init
{
	self = [super init];
	if (self)
	{
		[self initDefaults];
				
        queue = [[NSOperationQueue alloc]init];
	}
	
	return self; 
}

- (IBAction)openFiles:(id)sender
{
	NSOpenPanel *p;
	
	p = [NSOpenPanel openPanel];
	
    [p setAllowedFileTypes:[playlistLoader acceptableFileTypes]];
	[p setCanChooseDirectories:YES];
	[p setAllowsMultipleSelection:YES];
    [p setResolvesAliases:YES];
	
	[p beginSheetModalForWindow:mainWindow completionHandler:^(NSInteger result) {
        if ( result == NSFileHandlingPanelOKButton ) {
            [playlistLoader willInsertURLs:[p URLs] origin:URLOriginInternal];
            [playlistLoader didInsertURLs:[playlistLoader addURLs:[p URLs] sort:YES] origin:URLOriginInternal];
        } else {
            [p close];
        }
    }];
}

- (IBAction)savePlaylist:(id)sender
{
	NSSavePanel *p;
	
	p = [NSSavePanel savePanel];
	
	[p beginSheetModalForWindow:mainWindow completionHandler:^(NSInteger result) {
        if ( result == NSFileHandlingPanelOKButton ) {
            [playlistLoader save:[[p URL] path]];
        } else {
            [p close];
        }
    }];
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
		[playlistLoader willInsertURLs:[NSArray arrayWithObject:[panel url]] origin:URLOriginExternal];
		[playlistLoader didInsertURLs:[playlistLoader addURLs:[NSArray arrayWithObject:[panel url]] sort:NO] origin:URLOriginExternal];
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

- (void)awakeFromNib
{
	[[totalTimeField cell] setBackgroundStyle:NSBackgroundStyleRaised];
	
	[[playbackButtons cell] setToolTip:NSLocalizedString(@"PlayButtonTooltip", @"") forSegment: 1];
	[[playbackButtons cell] setToolTip:NSLocalizedString(@"PrevButtonTooltip", @"") forSegment: 0];
	[[playbackButtons cell] setToolTip:NSLocalizedString(@"NextButtonTooltip", @"") forSegment: 2];
	[infoButton setToolTip:NSLocalizedString(@"InfoButtonTooltip", @"")];
	[shuffleButton setToolTip:NSLocalizedString(@"ShuffleButtonTooltip", @"")];
	[repeatButton setToolTip:NSLocalizedString(@"RepeatButtonTooltip", @"")];
    [randomizeButton setToolTip:NSLocalizedString(@"RandomizeButtonTooltip", @"")];
	[fileButton setToolTip:NSLocalizedString(@"FileButtonTooltip", @"")];
	
	[self registerHotKeys];
	
    (void) [spotlightWindowController init];
	
	[[playlistController undoManager] disableUndoRegistration];
	NSString *basePath = [@"~/Library/Application Support/Cog/" stringByExpandingTildeInPath];
    NSString *oldFilename = @"Default.m3u";
    NSString *newFilename = @"Default.xml";
    
    if ([[NSFileManager defaultManager] fileExistsAtPath:[basePath stringByAppendingPathComponent:newFilename]])
    {
        [playlistLoader addURL:[NSURL fileURLWithPath:[basePath stringByAppendingPathComponent:newFilename]]];
    }
    else
    {
        [playlistLoader addURL:[NSURL fileURLWithPath:[basePath stringByAppendingPathComponent:oldFilename]]];
    }
    
	[[playlistController undoManager] enableUndoRegistration];
    
    int lastStatus = (int) [[NSUserDefaults standardUserDefaults] integerForKey:@"lastPlaybackStatus"];
    int lastIndex = (int) [[NSUserDefaults standardUserDefaults] integerForKey:@"lastTrackPlaying"];
    
    if (lastStatus != kCogStatusStopped && lastIndex >= 0)
    {
        [playbackController playEntryAtIndex:lastIndex startPaused:(lastStatus == kCogStatusPaused)];
        [playbackController seek:[NSNumber numberWithDouble:[[NSUserDefaults standardUserDefaults] floatForKey:@"lastTrackPosition"]]];
    }
    

    // Restore mini mode
    [self setMiniMode:[[NSUserDefaults standardUserDefaults] boolForKey:@"miniMode"]];

    // We need file tree view to restore its state here
    // so attempt to access file tree view controller's root view
    // to force it to read nib and create file tree view for us
    //
    // TODO: there probably is a more elegant way to do all this
    //       but i'm too stupid/tired to figure it out now
    [fileTreeViewController view];
    
    FileTreeOutlineView* outlineView = [fileTreeViewController outlineView];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(nodeExpanded:) name:NSOutlineViewItemDidExpandNotification object:outlineView];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(nodeCollapsed:) name:NSOutlineViewItemDidCollapseNotification object:outlineView];
    
    NSArray *expandedNodesArray = [[NSUserDefaults standardUserDefaults] valueForKey:@"fileTreeViewExpandedNodes"];
    
    if (expandedNodesArray)
    {
        expandedNodes = [[NSMutableSet alloc] initWithArray:expandedNodesArray];
    }
    else
    {
        expandedNodes = [[NSMutableSet alloc] init];
    }
    
    DLog(@"Nodes to expand: %@", [expandedNodes description]);
    
    DLog(@"Num of rows: %ld", [outlineView numberOfRows]);
    
    if (!outlineView)
    {
        DLog(@"outlineView is NULL!");
    }

    [outlineView reloadData];

    for (NSInteger i=0; i<[outlineView numberOfRows]; i++)
    {
        PathNode *pn = [outlineView itemAtRow:i];
        NSString *str = [[pn URL] absoluteString];
        
        if ([expandedNodes containsObject:str])
        {
            [outlineView expandItem:pn];
        }
    }
}

- (void)nodeExpanded:(NSNotification*)notification
{
    PathNode* node = [[notification userInfo] objectForKey:@"NSObject"];
    NSString* url = [[node URL] absoluteString];
    
    [expandedNodes addObject:url];
}

- (void)nodeCollapsed:(NSNotification*)notification
{
    PathNode* node = [[notification userInfo] objectForKey:@"NSObject"];
    NSString* url = [[node URL] absoluteString];
    
    [expandedNodes removeObject:url];
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
    int currentStatus = [playbackController playbackStatus];
    int lastTrackPlaying = -1;
    double lastTrackPosition = 0;
    
    [[NSUserDefaults standardUserDefaults] setInteger:currentStatus forKey:@"lastPlaybackStatus"];
    
    if (currentStatus != kCogStatusStopped)
    {
        PlaylistEntry * pe = [playlistController currentEntry];
        lastTrackPlaying = [pe index];
        lastTrackPosition = [pe currentPosition];
    }

    [[NSUserDefaults standardUserDefaults] setInteger:lastTrackPlaying forKey:@"lastTrackPlaying"];
    [[NSUserDefaults standardUserDefaults] setDouble:lastTrackPosition forKey:@"lastTrackPosition"];
    
	[playbackController stop:self];
	
	NSFileManager *fileManager = [NSFileManager defaultManager];
	NSString *folder = @"~/Library/Application Support/Cog/";
	
	folder = [folder stringByExpandingTildeInPath];
	
	if ([fileManager fileExistsAtPath: folder] == NO)
	{
		[fileManager createDirectoryAtPath: folder withIntermediateDirectories:NO attributes:nil error:nil];
	}
	
    NSString * fileName = @"Default.xml";
    
    [playlistLoader saveXml:[folder stringByAppendingPathComponent: fileName]];
    
    fileName = @"Default.m3u";
    
    NSError *error;
    [[NSFileManager defaultManager] removeItemAtPath:[folder stringByAppendingPathComponent:fileName] error:&error];

    DLog(@"Saving expanded nodes: %@", [expandedNodes description]);

    [[NSUserDefaults standardUserDefaults] setValue:[expandedNodes allObjects] forKey:@"fileTreeViewExpandedNodes"];
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag
{
	if (flag == NO)
		[mainWindow makeKeyAndOrderFront:self];	// TODO: do we really need this? We never close the main window.

	for(NSWindow* win in [NSApp windows])	// Maximizing all windows
		if([win isMiniaturized])
			[win deminiaturize:self];
	
	return NO;
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
	NSArray* urls = [NSArray arrayWithObject:[NSURL fileURLWithPath:filename]];
	[playlistLoader willInsertURLs:urls origin:URLOriginExternal];
	[playlistLoader didInsertURLs:[playlistLoader addURLs:urls sort:NO] origin:URLOriginExternal];
	return YES;
}

- (void)application:(NSApplication *)theApplication openFiles:(NSArray *)filenames
{
	//Need to convert to urls
	NSMutableArray *urls = [NSMutableArray array];
	
	for (NSString *filename in filenames)
	{
		[urls addObject:[NSURL fileURLWithPath:filename]];
	}
	[playlistLoader willInsertURLs:urls origin:URLOriginExternal];
	[playlistLoader didInsertURLs:[playlistLoader addURLs:urls sort:YES] origin:URLOriginExternal];
	[theApplication replyToOpenOrPrint:NSApplicationDelegateReplySuccess];
}

- (IBAction)toggleInfoDrawer:(id)sender
{
	[mainWindow makeKeyAndOrderFront:self];
	
	[infoDrawer toggle:self];
}

- (void)drawerDidOpen:(NSNotification *)notification
{
	if ([notification object] == infoDrawer) {
		[infoButton setState:NSOnState];
	}
}

- (void)drawerDidClose:(NSNotification *)notification
{
	if ([notification object] == infoDrawer) {
		[infoButton setState:NSOffState];
	}
}

- (IBAction)donate:(id)sender
{
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"https://pledgie.com/campaigns/31642"]];
}

- (void)initDefaults
{
	NSMutableDictionary *userDefaultsValuesDict = [NSMutableDictionary dictionary];
	
    // Font defaults
    float fFontSize = [NSFont systemFontSizeForControlSize:NSSmallControlSize];
    NSNumber *fontSize = [NSNumber numberWithFloat:fFontSize];
    [userDefaultsValuesDict setObject:fontSize forKey:@"fontSize"];
	
	[userDefaultsValuesDict setObject:[NSNumber numberWithInt:35] forKey:@"hotKeyPlayKeyCode"];
	[userDefaultsValuesDict setObject:[NSNumber numberWithInt:(NSEventModifierFlagControl|NSEventModifierFlagCommand)] forKey:@"hotKeyPlayModifiers"];
	
	[userDefaultsValuesDict setObject:[NSNumber numberWithInt:45] forKey:@"hotKeyNextKeyCode"];
	[userDefaultsValuesDict setObject:[NSNumber numberWithInt:(NSEventModifierFlagControl|NSEventModifierFlagCommand)] forKey:@"hotKeyNextModifiers"];
	
	[userDefaultsValuesDict setObject:[NSNumber numberWithInt:15] forKey:@"hotKeyPreviousKeyCode"];
	[userDefaultsValuesDict setObject:[NSNumber numberWithInt:(NSEventModifierFlagControl|NSEventModifierFlagCommand)] forKey:@"hotKeyPreviousModifiers"];
    
    [userDefaultsValuesDict setObject:[NSNumber numberWithInt:8] forKey:@"hotKeySpamKeyCode"];
    [userDefaultsValuesDict setObject:[NSNumber numberWithInt:(NSEventModifierFlagControl|NSEventModifierFlagCommand)] forKey:@"hotKeySpamModifiers"];

    NSString * feedURLdefault = @"https://f.losno.co/cog/mercury.xml";
    NSString * feedURLbroken = @"https://kode54.net/cog/stable.xml";
    NSString * feedURLbroken2 = @"https://kode54.net/cog/mercury.xml";
    NSString * feedURLbroken3 = @"https://www.kode54.net/cog/mercury.xml";
	[userDefaultsValuesDict setObject:feedURLdefault forKey:@"SUFeedURL"];


	[userDefaultsValuesDict setObject:@"clearAndPlay" forKey:@"openingFilesBehavior"];
	[userDefaultsValuesDict setObject:@"enqueue" forKey:@"openingFilesAlteredBehavior"];
    
    [userDefaultsValuesDict setObject:@"albumGainWithPeak" forKey:@"volumeScaling"];

    [userDefaultsValuesDict setObject:@"cubic" forKey:@"resampling"];
    
    [userDefaultsValuesDict setObject:[NSNumber numberWithInteger:kCogStatusStopped] forKey:@"lastPlaybackStatus"];
    [userDefaultsValuesDict setObject:[NSNumber numberWithInteger:-1] forKey:@"lastTrackPlaying"];
    [userDefaultsValuesDict setObject:[NSNumber numberWithDouble:0] forKey:@"lastTrackPosition"];
    
    [userDefaultsValuesDict setObject:@"dls appl" forKey:@"midi.plugin"];
    
    [userDefaultsValuesDict setObject:@"sc55" forKey:@"midi.flavor"];
    
	//Register and sync defaults
	[[NSUserDefaults standardUserDefaults] registerDefaults:userDefaultsValuesDict];
	[[NSUserDefaults standardUserDefaults] synchronize];
    
    //And if the existing feed URL is broken due to my ineptitude with the above defaults, fix it
    if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"SUFeedURL"] isEqualToString:feedURLbroken] ||
        [[[NSUserDefaults standardUserDefaults] stringForKey:@"SUFeedURL"] isEqualToString:feedURLbroken2] ||
        [[[NSUserDefaults standardUserDefaults] stringForKey:@"SUFeedURL"] isEqualToString:feedURLbroken3])
        [[NSUserDefaults standardUserDefaults] setValue:feedURLdefault forKey:@"SUFeedURL"];
	
	//Add observers
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.hotKeyPlayKeyCode"		options:0 context:nil];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.hotKeyPreviousKeyCode"	options:0 context:nil];
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.hotKeyNextKeyCode"		options:0 context:nil];
    [[NSUserDefaultsController sharedUserDefaultsController] addObserver:self
        forKeyPath:@"values.hotKeySpamKeyCode"      options:0 context:nil];
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
    else if ([keyPath isEqualToString:@"values.hotKeySpamKeyCode"]) {
        [self registerHotKeys];
    }
}

- (void)registerHotKeys
{
    if ([[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"hotKeyPlayModifiers"] intValue]) {
	playHotKey = [[NDHotKeyEvent alloc]
		initWithKeyCode: [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"hotKeyPlayKeyCode"] intValue]
		  modifierFlags: [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"hotKeyPlayModifiers"] intValue]
		];
        [playHotKey setTarget:self selector:@selector(clickPlay)];
        [playHotKey setEnabled:YES];
    }
	
    if ([[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"hotKeyPreviousModifiers"] intValue]) {
	prevHotKey = [[NDHotKeyEvent alloc]
		  initWithKeyCode: [[NSUserDefaults standardUserDefaults] integerForKey:@"hotKeyPreviousKeyCode"]
			modifierFlags: [[NSUserDefaults standardUserDefaults] integerForKey:@"hotKeyPreviousModifiers"]
		];
        [prevHotKey setTarget:self selector:@selector(clickPrev)];
        [prevHotKey setEnabled:YES];
    }
	
    if ([[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"hotKeyNextModifiers"] intValue]) {
	nextHotKey = [[NDHotKeyEvent alloc]
		initWithKeyCode: [[NSUserDefaults standardUserDefaults] integerForKey:@"hotKeyNextKeyCode"]
			modifierFlags: [[NSUserDefaults standardUserDefaults] integerForKey:@"hotKeyNextModifiers"]
		];
        [nextHotKey setTarget:self selector:@selector(clickNext)];
        [nextHotKey setEnabled:YES];
    }

    if ([[[[NSUserDefaultsController sharedUserDefaultsController] defaults] objectForKey:@"hotKeySpamModifiers"] intValue]) {
        spamHotKey = [[NDHotKeyEvent alloc]
                      initWithKeyCode: [[NSUserDefaults standardUserDefaults] integerForKey:@"hotKeySpamKeyCode"]
                      modifierFlags: [[NSUserDefaults standardUserDefaults] integerForKey:@"hotKeySpamModifiers"]
                      ];
        [spamHotKey setTarget:self selector:@selector(clickSpam)];
        [spamHotKey setEnabled:YES];
    }
}

- (void)windowDidEnterFullScreen:(NSNotification *)notification
{
    DLog(@"Entering fullscreen");
    if (nil == nowPlaying)
    {
        nowPlaying = [[NowPlayingBarController alloc] init];
        
        NSView *contentView = [mainWindow contentView];
        NSRect contentRect = [contentView frame];
        const NSSize windowSize = [contentView convertSize:[mainWindow frame].size fromView: nil];
        
        NSRect nowPlayingFrame = [[nowPlaying view] frame];
        nowPlayingFrame.size.width = windowSize.width;
        [[nowPlaying view] setFrame: nowPlayingFrame];
        
        [contentView addSubview: [nowPlaying view]];
        [[nowPlaying view] setFrameOrigin: NSMakePoint(0.0, NSMaxY(contentRect) - nowPlayingFrame.size.height)];
        
        NSRect mainViewFrame = [mainView frame];
        mainViewFrame.size.height -= nowPlayingFrame.size.height;
        [mainView setFrame:mainViewFrame];
        
        [[nowPlaying text] bind:@"value" toObject:currentEntryController withKeyPath:@"content.display" options:nil];
    }
}

- (void)windowDidExitFullScreen:(NSNotification *)notification
{
    DLog(@"Exiting fullscreen");
    if (nowPlaying)
    {
        NSRect nowPlayingFrame = [[nowPlaying view] frame];
        NSRect mainViewFrame = [mainView frame];
        mainViewFrame.size.height += nowPlayingFrame.size.height;
        [mainView setFrame:mainViewFrame];
        //        [mainView setFrameOrigin:NSMakePoint(0.0, 0.0)];
        
        [[nowPlaying view] removeFromSuperview];
        nowPlaying = nil;
    }
}

- (void)clickPlay
{
	[playbackController playPauseResume:self];
}

- (void)clickPrev
{
	[playbackController prev:nil];
}

- (void)clickNext
{
	[playbackController next:nil];
}

- (void)clickSpam
{
    [playbackController spam];
}

- (void)changeFontSize:(float)size
{
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    float fCurrentSize = [defaults floatForKey:@"fontSize"];
    NSNumber *newSize = [NSNumber numberWithFloat:(fCurrentSize + size)];
    [defaults setObject:newSize forKey:@"fontSize"];
}

- (IBAction)increaseFontSize:(id)sender
{
	[self changeFontSize:1];
}

- (IBAction)decreaseFontSize:(id)sender
{
	[self changeFontSize:-1];
	
} 

- (IBAction)toggleMiniMode:(id)sender
{
    [self setMiniMode:(!miniMode)];
}

- (BOOL)miniMode
{
    return miniMode;
}

- (void)setMiniMode:(BOOL)newMiniMode
{
    miniMode = newMiniMode;
    [[NSUserDefaults standardUserDefaults] setBool:miniMode forKey:@"miniMode"];
    
    NSWindow *windowToShow = miniMode ? miniWindow : mainWindow;
    NSWindow *windowToHide = miniMode ? mainWindow : miniWindow;
    [windowToHide close];
    [windowToShow makeKeyAndOrderFront:self];
}

@end
