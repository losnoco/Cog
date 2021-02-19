#import "AppController.h"
#import "FileTreeController.h"
#import "FileTreeViewController.h"
#import "FileTreeOutlineView.h"
#import "PlaybackController.h"
#import "PlaylistController.h"
#import "PlaylistView.h"
#import "PlaylistEntry.h"
#import "PlaylistLoader.h"
#import "OpenURLPanel.h"
#import "SpotlightWindowController.h"
#import "StringToURLTransformer.h"
#import "FontSizetoLineHeightTransformer.h"
#import "Cog-Swift.h"
#import "PathNode.h"
#import <CogAudio/Status.h>

#import "Logging.h"
#import "MiniModeMenuTitleTransformer.h"
#import "DualWindow.h"

#import <MASShortcut/Shortcut.h>
#import "Shortcuts.h"

void* kAppControllerContext = &kAppControllerContext;


@implementation AppController {
    BOOL _isFullToolbarStyle;
}

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
        if ( result == NSModalResponseOK ) {
            [self->playlistLoader willInsertURLs:[p URLs] origin:URLOriginInternal];
            [self->playlistLoader didInsertURLs:[self->playlistLoader addURLs:[p URLs] sort:YES] origin:URLOriginInternal];
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
        if ( result == NSModalResponseOK ) {
            [self->playlistLoader save:[[p URL] path]];
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
	if (returnCode == NSModalResponseOK)
	{
		[playlistLoader willInsertURLs:[NSArray arrayWithObject:[panel url]] origin:URLOriginInternal];
		[playlistLoader didInsertURLs:[playlistLoader addURLs:[NSArray arrayWithObject:[panel url]] sort:NO] origin:URLOriginInternal];
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
    [self.infoButton setToolTip:NSLocalizedString(@"InfoButtonTooltip", @"")];
    [self.infoButtonMini setToolTip:NSLocalizedString(@"InfoButtonTooltip", @"")];
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
    
    if ([[NSUserDefaults standardUserDefaults] boolForKey:@"resumePlaybackOnStartup"])
    {
        int lastStatus = (int) [[NSUserDefaults standardUserDefaults] integerForKey:@"lastPlaybackStatus"];
        int lastIndex = (int) [[NSUserDefaults standardUserDefaults] integerForKey:@"lastTrackPlaying"];
    
        if (lastStatus != CogStatusStopped && lastIndex >= 0)
        {
            [playbackController playEntryAtIndex:lastIndex startPaused:(lastStatus == CogStatusPaused)];
            [playbackController seek:[NSNumber numberWithDouble:[[NSUserDefaults standardUserDefaults] floatForKey:@"lastTrackPosition"]]];
        }
    }

    // Restore mini mode
    [self setMiniMode:[[NSUserDefaults standardUserDefaults] boolForKey:@"miniMode"]];

    [self setToolbarStyle:[[NSUserDefaults standardUserDefaults] boolForKey:@"toolbarStyleFull"]];

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

    [self addObserver:self
           forKeyPath:@"playlistController.currentEntry"
              options:NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew
              context:kAppControllerContext];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary<NSKeyValueChangeKey,id> *)change
                       context:(void *)context {
    if (context != kAppControllerContext) {
        return;
    }

    if ([keyPath isEqualToString:@"playlistController.currentEntry"]) {
        PlaylistEntry *entry = playlistController.currentEntry;
        if (!entry) {
            miniWindow.title = @"Cog";
            mainWindow.title = @"Cog";
            if (@available(macOS 11.0, *)) {
                miniWindow.subtitle = @"";
                mainWindow.subtitle = @"";
            }

            self.infoButton.imageScaling = NSImageScaleNone;
            self.infoButton.image = [NSImage imageNamed:@"infoTemplate"];
            self.infoButtonMini.imageScaling = NSImageScaleNone;
            self.infoButtonMini.image = [NSImage imageNamed:@"infoTemplate"];
        }

        if (@available(macOS 11.0, *)) {
            NSString *title = @"Cog";
            if (entry.title) {
                title = entry.title;
            }
            miniWindow.title = title;
            mainWindow.title = title;

            NSString *subtitle = @"";
            NSMutableArray<NSString *> *subtitleItems = [NSMutableArray array];
            if (entry.album && ![entry.album isEqualToString:@""]) {
                [subtitleItems addObject:entry.album];
            }
            if (entry.artist && ![entry.artist isEqualToString:@""]) {
                [subtitleItems addObject:entry.artist];
            }

            if ([subtitleItems count]) {
                subtitle = [subtitleItems componentsJoinedByString:@" - "];
            }

            miniWindow.subtitle = subtitle;
            mainWindow.subtitle = subtitle;
        } else {
            NSString *title = @"Cog";
            if (entry.display) {
                title = entry.display;
            }
            miniWindow.title = title;
            mainWindow.title = title;
        }

        if (entry.albumArt) {
            self.infoButton.imageScaling = NSImageScaleProportionallyUpOrDown;
            self.infoButton.image = playlistController.currentEntry.albumArt;
            self.infoButtonMini.imageScaling = NSImageScaleProportionallyUpOrDown;
            self.infoButtonMini.image = playlistController.currentEntry.albumArt;
        } else {
            self.infoButton.imageScaling = NSImageScaleNone;
            self.infoButton.image = [NSImage imageNamed:@"infoTemplate"];
            self.infoButtonMini.imageScaling = NSImageScaleNone;
            self.infoButtonMini.image = [NSImage imageNamed:@"infoTemplate"];
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
    CogStatus currentStatus = [playbackController playbackStatus];
    int lastTrackPlaying = -1;
    double lastTrackPosition = 0;
    
    if (currentStatus == CogStatusStopping)
        currentStatus = CogStatusStopped;
    
    [[NSUserDefaults standardUserDefaults] setInteger:currentStatus forKey:@"lastPlaybackStatus"];
    
    if (currentStatus != CogStatusStopped)
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
    // Workaround window not restoring it's size and position.
    [miniWindow setContentSize:NSMakeSize(miniWindow.frame.size.width, 1)];
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

- (IBAction)openLiberapayPage:(id)sender
{
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"https://liberapay.com/kode54"]];
}

- (IBAction)openPaypalPage:(id)sender
{
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"https://www.paypal.com/paypalme/kode54"]];
}

- (IBAction)openBitcoinPage:(id)sender
{
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"https://kode54.net/donateBitcoin"]];
}

- (IBAction)openPatreonPage:(id)sender
{
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"https://patreon.com/kode54"]];
}

- (IBAction)openKofiPage:(id)sender
{
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"https://ko-fi.com/kode54"]];
}


- (IBAction)feedback:(id)sender
{
    NSString *version = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"];

    NSArray<NSURLQueryItem *> *query = @[
        [NSURLQueryItem queryItemWithName:@"labels" value:@"bug"],
        [NSURLQueryItem queryItemWithName:@"template" value:@"bug_report.md"],
        [NSURLQueryItem queryItemWithName:@"title"
                                    value:[NSString stringWithFormat:@"[Cog %@] ", version]]
    ];
    NSURLComponents *components =
        [NSURLComponents componentsWithString:@"https://github.com/losnoco/Cog/issues/new"];

    components.queryItems = query;

    [[NSWorkspace sharedWorkspace] openURL:components.URL];
}

- (void)initDefaults
{
	NSMutableDictionary *userDefaultsValuesDict = [NSMutableDictionary dictionary];
	
    // Font defaults
    float fFontSize = [NSFont systemFontSizeForControlSize:NSControlSizeSmall];
    NSNumber *fontSize = [NSNumber numberWithFloat:fFontSize];
    [userDefaultsValuesDict setObject:fontSize forKey:@"fontSize"];

    NSString *feedURLdefault = @"https://f.losno.co/cog/mercury.xml";
    [userDefaultsValuesDict setObject:feedURLdefault forKey:@"SUFeedURL"];

	[userDefaultsValuesDict setObject:@"clearAndPlay" forKey:@"openingFilesBehavior"];
	[userDefaultsValuesDict setObject:@"enqueue" forKey:@"openingFilesAlteredBehavior"];
    
    [userDefaultsValuesDict setObject:@"albumGainWithPeak" forKey:@"volumeScaling"];

    [userDefaultsValuesDict setObject:@"cubic" forKey:@"resampling"];
    
    [userDefaultsValuesDict setObject:[NSNumber numberWithInteger:CogStatusStopped] forKey:@"lastPlaybackStatus"];
    [userDefaultsValuesDict setObject:[NSNumber numberWithInteger:-1] forKey:@"lastTrackPlaying"];
    [userDefaultsValuesDict setObject:[NSNumber numberWithDouble:0] forKey:@"lastTrackPosition"];
    
    [userDefaultsValuesDict setObject:@"dls appl" forKey:@"midi.plugin"];
    
    [userDefaultsValuesDict setObject:@"default" forKey:@"midi.flavor"];
    
    [userDefaultsValuesDict setObject:[NSNumber numberWithBool:NO] forKey:@"resumePlaybackOnStartup"];
    
	//Register and sync defaults
	[[NSUserDefaults standardUserDefaults] registerDefaults:userDefaultsValuesDict];
	[[NSUserDefaults standardUserDefaults] synchronize];

    //And if the existing feed URL is broken due to my ineptitude with the above defaults, fix it
    NSSet<NSString *> *brokenFeedURLs = [NSSet setWithObjects:
                                          @"https://kode54.net/cog/stable.xml",
                                          @"https://kode54.net/cog/mercury.xml"
                                          @"https://www.kode54.net/cog/mercury.xml",
                                         nil];
    NSString *feedURL = [[NSUserDefaults standardUserDefaults] stringForKey:@"SUFeedURL"];
    if ([brokenFeedURLs containsObject:feedURL]) {
        [[NSUserDefaults standardUserDefaults] setValue:feedURLdefault forKey:@"SUFeedURL"];
    }
}

/* Unassign previous handler first, so dealloc can unregister it from the global map before the new instances are assigned */
- (void)registerHotKeys
{
    MASShortcutBinder *binder = [MASShortcutBinder sharedBinder];
    [binder bindShortcutWithDefaultsKey:CogPlayShortcutKey toAction:^{
        [self clickPlay];
    }];

    [binder bindShortcutWithDefaultsKey:CogNextShortcutKey toAction:^{
        [self clickNext];
    }];

    [binder bindShortcutWithDefaultsKey:CogPrevShortcutKey toAction:^{
        [self clickPrev];
    }];

    [binder bindShortcutWithDefaultsKey:CogSpamShortcutKey toAction:^{
        [self clickSpam];
    }];
}

- (void)clickPlay
{
	[playbackController playPauseResume:self];
}

- (void)clickPause
{
    [playbackController pause:self];
}

- (void)clickStop
{
    [playbackController stop:self];
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

- (void)clickSeek:(NSTimeInterval)position
{
    [playbackController seek:self toTime:position];
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

- (IBAction)toggleToolbarStyle:(id)sender {
    [self setToolbarStyle:!_isFullToolbarStyle];
}

- (void)setToolbarStyle:(BOOL)full {
    _isFullToolbarStyle = full;
    [[NSUserDefaults standardUserDefaults] setBool:full forKey:@"toolbarStyleFull"];
    DLog("Changed toolbar style: %@", (full ? @"full" : @"compact"));

    if (@available(macOS 11.0, *)) {
        NSWindowToolbarStyle style =
            full ? NSWindowToolbarStyleExpanded : NSWindowToolbarStyleUnified;
        mainWindow.toolbarStyle = style;
        miniWindow.toolbarStyle = style;
    } else {
        NSWindowTitleVisibility titleVisibility = full ? NSWindowTitleVisible : NSWindowTitleHidden;
        mainWindow.titleVisibility = titleVisibility;
        miniWindow.titleVisibility = titleVisibility;
    }

    // Fix empty area after changing toolbar style in mini window as it has no content view
    [miniWindow setContentSize:NSMakeSize(miniWindow.frame.size.width, 0)];
}

@end
