#import "AppController.h"
#import "Cog-Swift.h"
#import "FileTreeController.h"
#import "FileTreeOutlineView.h"
#import "FileTreeViewController.h"
#import "FontSizetoLineHeightTransformer.h"
#import "OpenURLPanel.h"
#import "PathNode.h"
#import "PlaybackController.h"
#import "PlaylistController.h"
#import "PlaylistEntry.h"
#import "PlaylistLoader.h"
#import "PlaylistView.h"
#import "SQLiteStore.h"
#import "SpotlightWindowController.h"
#import "StringToURLTransformer.h"
#import <CogAudio/Status.h>

#import "DualWindow.h"
#import "Logging.h"
#import "MiniModeMenuTitleTransformer.h"

#import "ColorToValueTransformer.h"

#import "TotalTimeTransformer.h"

#import "Shortcuts.h"
#import <MASShortcut/Shortcut.h>

#import <Sparkle/Sparkle.h>

@import Firebase;
#ifdef DEBUG
@import FirebaseAppCheck;
#endif

void *kAppControllerContext = &kAppControllerContext;

@implementation AppController {
	BOOL _isFullToolbarStyle;
}

@synthesize mainWindow;
@synthesize miniWindow;

+ (void)initialize {
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

	NSValueTransformer *colorToValueTransformer = [[ColorToValueTransformer alloc] init];
	[NSValueTransformer setValueTransformer:colorToValueTransformer
	                                forName:@"ColorToValueTransformer"];

	NSValueTransformer *totalTimeTransformer = [[TotalTimeTransformer alloc] init];
	[NSValueTransformer setValueTransformer:totalTimeTransformer
	                                forName:@"TotalTimeTransformer"];
}
- (id)init {
	self = [super init];
	if(self) {
		[self initDefaults];

		queue = [[NSOperationQueue alloc] init];
	}

	return self;
}

- (IBAction)openFiles:(id)sender {
	NSOpenPanel *p;

	p = [NSOpenPanel openPanel];

	[p setAllowedFileTypes:[playlistLoader acceptableFileTypes]];
	[p setCanChooseDirectories:YES];
	[p setAllowsMultipleSelection:YES];
	[p setResolvesAliases:YES];

	[p beginSheetModalForWindow:mainWindow
	          completionHandler:^(NSInteger result) {
		          if(result == NSModalResponseOK) {
			          [self->playlistLoader willInsertURLs:[p URLs] origin:URLOriginExternal];
			          [self->playlistLoader didInsertURLs:[self->playlistLoader addURLs:[p URLs] sort:YES] origin:URLOriginExternal];
		          } else {
			          [p close];
		          }
	          }];
}

- (IBAction)savePlaylist:(id)sender {
	NSSavePanel *p;

	p = [NSSavePanel savePanel];

	/* Yes, this is deprecated. Yes, this is required to give the dialog
	 * a default set of filename extensions to save, including adding an
	 * extension if the user does not supply one. */
	[p setAllowedFileTypes:@[@"m3u", @"pls"]];

	[p beginSheetModalForWindow:mainWindow
	          completionHandler:^(NSInteger result) {
		          if(result == NSModalResponseOK) {
			          [self->playlistLoader save:[[p URL] path]];
		          } else {
			          [p close];
		          }
	          }];
}

- (IBAction)openURL:(id)sender {
	OpenURLPanel *p;

	p = [OpenURLPanel openURLPanel];

	[p beginSheetWithWindow:mainWindow delegate:self didEndSelector:@selector(openURLPanelDidEnd:returnCode:contextInfo:) contextInfo:nil];
}

- (void)openURLPanelDidEnd:(OpenURLPanel *)panel returnCode:(int)returnCode contextInfo:(void *)contextInfo {
	if(returnCode == NSModalResponseOK) {
		[playlistLoader willInsertURLs:@[[panel url]] origin:URLOriginExternal];
		[playlistLoader didInsertURLs:[playlistLoader addURLs:@[[panel url]] sort:NO] origin:URLOriginExternal];
	}
}

- (IBAction)delEntries:(id)sender {
	[playlistController remove:self];
}

- (PlaylistEntry *)currentEntry {
	return [playlistController currentEntry];
}

- (BOOL)application:(NSApplication *)sender delegateHandlesKey:(NSString *)key {
	return [key isEqualToString:@"currentEntry"];
}

- (void)awakeFromNib {
	[[NSUserDefaults standardUserDefaults] registerDefaults:@{ @"NSApplicationCrashOnExceptions": @(YES) }];
	
#ifdef DEBUG
	FIRAppCheckDebugProviderFactory *providerFactory =
		  [[FIRAppCheckDebugProviderFactory alloc] init];
	[FIRAppCheck setAppCheckProviderFactory:providerFactory];
#endif
	
	[FIRApp configure];

	/* Evil startup synchronous crash log submitter, because apparently, there
	 * are some startup crashes that need diagnosing, and they're not getting
	 * sent, because the asynchronous defaults are not kicking in before the
	 * ensuing startup crash that happens somewhere later in this function. */
	__block BOOL submitCompleted = NO;
	ALog(@"Checking for unsent reports...");
	[[FIRCrashlytics crashlytics] checkForUnsentReportsWithCompletion:^(BOOL hasReports) {
		if(hasReports) {
			ALog(@"Unsent reports found, sending...");
			[[FIRCrashlytics crashlytics] sendUnsentReports];
			ALog(@"Reports sent, continuing...");
		} else {
			ALog(@"No reports found, continuing...");
		}
		submitCompleted = YES;
	}];
	while(!submitCompleted) {
		[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
	}
	
	[FIRAnalytics setAnalyticsCollectionEnabled:YES];

#ifdef DEBUG
	// Prevent updates automatically in debug builds
	[updater setAutomaticallyChecksForUpdates:NO];
#endif

	[[totalTimeField cell] setBackgroundStyle:NSBackgroundStyleRaised];

	[self.infoButton setToolTip:NSLocalizedString(@"InfoButtonTooltip", @"")];
	[self.infoButtonMini setToolTip:NSLocalizedString(@"InfoButtonTooltip", @"")];
	[shuffleButton setToolTip:NSLocalizedString(@"ShuffleButtonTooltip", @"")];
	[repeatButton setToolTip:NSLocalizedString(@"RepeatButtonTooltip", @"")];
	[randomizeButton setToolTip:NSLocalizedString(@"RandomizeButtonTooltip", @"")];
	[fileButton setToolTip:NSLocalizedString(@"FileButtonTooltip", @"")];

	[self registerHotKeys];

	(void)[spotlightWindowController init];

	[[playlistController undoManager] disableUndoRegistration];
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
	NSString *basePath = [[paths firstObject] stringByAppendingPathComponent:@"Cog"];

	NSString *dbFilename = @"Default.sqlite";

	NSString *oldFilename = @"Default.m3u";
	NSString *newFilename = @"Default.xml";

	BOOL dataStorePresent = [playlistLoader addDataStore];

	if(!dataStorePresent) {
		if([[NSFileManager defaultManager] fileExistsAtPath:[basePath stringByAppendingPathComponent:dbFilename]]) {
			[playlistLoader addDatabase];
		} else if([[NSFileManager defaultManager] fileExistsAtPath:[basePath stringByAppendingPathComponent:newFilename]]) {
			[playlistLoader addURL:[NSURL fileURLWithPath:[basePath stringByAppendingPathComponent:newFilename]]];
		} else {
			[playlistLoader addURL:[NSURL fileURLWithPath:[basePath stringByAppendingPathComponent:oldFilename]]];
		}
	}

	[[playlistController undoManager] enableUndoRegistration];

	if([[NSUserDefaults standardUserDefaults] boolForKey:@"resumePlaybackOnStartup"]) {
		int lastStatus = (int)[[NSUserDefaults standardUserDefaults] integerForKey:@"lastPlaybackStatus"];
		int lastIndex = (int)[[NSUserDefaults standardUserDefaults] integerForKey:@"lastTrackPlaying"];

		if(lastStatus != CogStatusStopped && lastIndex >= 0) {
			[playbackController playEntryAtIndex:lastIndex startPaused:(lastStatus == CogStatusPaused) andSeekTo:@([[NSUserDefaults standardUserDefaults] doubleForKey:@"lastTrackPosition"])];
		}
	}

	// Restore mini mode
	[self setMiniMode:[[NSUserDefaults standardUserDefaults] boolForKey:@"miniMode"]];

	[self setToolbarStyle:[[NSUserDefaults standardUserDefaults] boolForKey:@"toolbarStyleFull"]];

	[self setFloatingMiniWindow:[[NSUserDefaults standardUserDefaults]
	                            boolForKey:@"floatingMiniWindow"]];

	// We need file tree view to restore its state here
	// so attempt to access file tree view controller's root view
	// to force it to read nib and create file tree view for us
	//
	// TODO: there probably is a more elegant way to do all this
	//       but i'm too stupid/tired to figure it out now
	[fileTreeViewController view];

	FileTreeOutlineView *outlineView = [fileTreeViewController outlineView];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(nodeExpanded:) name:NSOutlineViewItemDidExpandNotification object:outlineView];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(nodeCollapsed:) name:NSOutlineViewItemDidCollapseNotification object:outlineView];

	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateDockMenu:) name:CogPlaybackDidBeginNotficiation object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateDockMenu:) name:CogPlaybackDidStopNotficiation object:nil];

	[self updateDockMenu:nil];

	NSArray *expandedNodesArray = [[NSUserDefaults standardUserDefaults] valueForKey:@"fileTreeViewExpandedNodes"];

	if(expandedNodesArray) {
		expandedNodes = [[NSMutableSet alloc] initWithArray:expandedNodesArray];
	} else {
		expandedNodes = [[NSMutableSet alloc] init];
	}

	DLog(@"Nodes to expand: %@", [expandedNodes description]);

	DLog(@"Num of rows: %ld", [outlineView numberOfRows]);

	if(!outlineView) {
		DLog(@"outlineView is NULL!");
	}

	[outlineView reloadData];

	for(NSInteger i = 0; i < [outlineView numberOfRows]; i++) {
		PathNode *pn = [outlineView itemAtRow:i];
		NSString *str = [[pn URL] absoluteString];

		if([expandedNodes containsObject:str]) {
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
                        change:(NSDictionary<NSKeyValueChangeKey, id> *)change
                       context:(void *)context {
	if(context != kAppControllerContext) {
		return;
	}

	if([keyPath isEqualToString:@"playlistController.currentEntry"]) {
		PlaylistEntry *entry = playlistController.currentEntry;
		NSString *appTitle = NSLocalizedString(@"CogTitle", @"Cog");
		if(!entry) {
			miniWindow.title = appTitle;
			mainWindow.title = appTitle;
			if(@available(macOS 11.0, *)) {
				miniWindow.subtitle = @"";
				mainWindow.subtitle = @"";
			}

			self.infoButton.imageScaling = NSImageScaleNone;
			self.infoButton.image = [NSImage imageNamed:@"infoTemplate"];
			self.infoButtonMini.imageScaling = NSImageScaleNone;
			self.infoButtonMini.image = [NSImage imageNamed:@"infoTemplate"];
		}

		if(entry.albumArt) {
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

		if(@available(macOS 11.0, *)) {
			NSString *title = appTitle;
			if(entry.title) {
				title = entry.title;
			}
			miniWindow.title = title;
			mainWindow.title = title;

			NSString *subtitle = @"";
			NSMutableArray<NSString *> *subtitleItems = [NSMutableArray array];
			if(entry.album && ![entry.album isEqualToString:@""]) {
				[subtitleItems addObject:entry.album];
			}
			if(entry.artist && ![entry.artist isEqualToString:@""]) {
				[subtitleItems addObject:entry.artist];
			}

			if([subtitleItems count]) {
				subtitle = [subtitleItems componentsJoinedByString:@" - "];
			}

			miniWindow.subtitle = subtitle;
			mainWindow.subtitle = subtitle;
		} else {
			NSString *title = appTitle;
			if(entry.display) {
				title = entry.display;
			}
			miniWindow.title = title;
			mainWindow.title = title;
		}
	} else if([keyPath isEqualToString:@"finished"]) {
		NSProgress *progress = (NSProgress *)object;
		if([progress isFinished]) {
			playbackController.progressOverall = nil;
			[NSApp terminate:nil];
		}
	}
}

- (void)nodeExpanded:(NSNotification *)notification {
	PathNode *node = [[notification userInfo] objectForKey:@"NSObject"];
	NSString *url = [[node URL] absoluteString];

	[expandedNodes addObject:url];
}

- (void)nodeCollapsed:(NSNotification *)notification {
	PathNode *node = [[notification userInfo] objectForKey:@"NSObject"];
	NSString *url = [[node URL] absoluteString];

	[expandedNodes removeObject:url];
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
	if(playbackController.progressOverall) {
		[playbackController.progressOverall addObserver:self forKeyPath:@"finished" options:0 context:kAppControllerContext];
		return NSTerminateLater;
	} else {
		return NSTerminateNow;
	}
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
	CogStatus currentStatus = [playbackController playbackStatus];
	NSInteger lastTrackPlaying = -1;
	double lastTrackPosition = 0;

	if(currentStatus == CogStatusStopping)
		currentStatus = CogStatusStopped;

	if(currentStatus != CogStatusStopped) {
		PlaylistEntry *pe = [playlistController currentEntry];
		lastTrackPlaying = [pe index];
		lastTrackPosition = [pe currentPosition];
	}

	[[NSUserDefaults standardUserDefaults] setInteger:lastTrackPlaying forKey:@"lastTrackPlaying"];
	[[NSUserDefaults standardUserDefaults] setDouble:lastTrackPosition forKey:@"lastTrackPosition"];

	[playbackController stop:self];

	[[NSUserDefaults standardUserDefaults] setInteger:currentStatus forKey:@"lastPlaybackStatus"];

	NSFileManager *fileManager = [NSFileManager defaultManager];
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
	NSString *folder = [[paths firstObject] stringByAppendingPathComponent:@"Cog"];

	if([fileManager fileExistsAtPath:folder] == NO) {
		[fileManager createDirectoryAtPath:folder withIntermediateDirectories:NO attributes:nil error:nil];
	}

	[playlistController clearFilterPredicate:self];

	NSMutableDictionary<NSString *, AlbumArtwork *> *artLeftovers = [playlistController.persistentArtStorage mutableCopy];

	NSManagedObjectContext *moc = playlistController.persistentContainer.viewContext;

	for(PlaylistEntry *pe in playlistController.arrangedObjects) {
		if(pe.deLeted) {
			[moc deleteObject:pe];
			continue;
		}
		if([artLeftovers objectForKey:pe.artHash]) {
			[artLeftovers removeObjectForKey:pe.artHash];
		}
	}

	for(NSString *key in artLeftovers) {
		[moc deleteObject:[artLeftovers objectForKey:key]];
	}

	[playlistController commitPersistentStore];

	if([SQLiteStore databaseStarted]) {
		[[SQLiteStore sharedStore] shutdown];
	}

	NSError *error;
	NSString *fileName = @"Default.sqlite";

	[[NSFileManager defaultManager] removeItemAtPath:[folder stringByAppendingPathComponent:fileName] error:&error];

	fileName = @"Default.xml";

	[[NSFileManager defaultManager] removeItemAtPath:[folder stringByAppendingPathComponent:fileName] error:&error];

	fileName = @"Default.m3u";

	[[NSFileManager defaultManager] removeItemAtPath:[folder stringByAppendingPathComponent:fileName] error:&error];

	DLog(@"Saving expanded nodes: %@", [expandedNodes description]);

	[[NSUserDefaults standardUserDefaults] setValue:[expandedNodes allObjects] forKey:@"fileTreeViewExpandedNodes"];
	// Workaround window not restoring it's size and position.
	[miniWindow setContentSize:NSMakeSize(miniWindow.frame.size.width, 1)];
	[miniWindow saveFrameUsingName:@"Mini Window"];
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag {
	if(flag == NO)
		[mainWindow makeKeyAndOrderFront:self]; // TODO: do we really need this? We never close the main window.

	for(NSWindow *win in [NSApp windows]) // Maximizing all windows
		if([win isMiniaturized])
			[win deminiaturize:self];

	return NO;
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename {
	NSArray *urls = @[[NSURL fileURLWithPath:filename]];
	[playlistLoader willInsertURLs:urls origin:URLOriginExternal];
	[playlistLoader didInsertURLs:[playlistLoader addURLs:urls sort:NO] origin:URLOriginExternal];
	return YES;
}

- (void)application:(NSApplication *)theApplication openFiles:(NSArray *)filenames {
	// Need to convert to urls
	NSMutableArray *urls = [NSMutableArray array];

	for(NSString *filename in filenames) {
		[urls addObject:[NSURL fileURLWithPath:filename]];
	}
	[playlistLoader willInsertURLs:urls origin:URLOriginExternal];
	[playlistLoader didInsertURLs:[playlistLoader addURLs:urls sort:YES] origin:URLOriginExternal];
	[theApplication replyToOpenOrPrint:NSApplicationDelegateReplySuccess];
}

- (IBAction)openLiberapayPage:(id)sender {
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"https://liberapay.com/kode54"]];
}

- (IBAction)openPaypalPage:(id)sender {
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"https://www.paypal.com/paypalme/kode54"]];
}

- (IBAction)openKofiPage:(id)sender {
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"https://ko-fi.com/kode54"]];
}

- (IBAction)openPatreonPage:(id)sender {
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"https://www.patreon.com/kode54"]];
}

- (IBAction)feedback:(id)sender {
	NSString *version = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"];

	NSArray<NSURLQueryItem *> *query = @[
		[NSURLQueryItem queryItemWithName:@"labels"
		                            value:@"bug"],
		[NSURLQueryItem queryItemWithName:@"template"
		                            value:@"bug_report.md"],
		[NSURLQueryItem queryItemWithName:@"title"
		                            value:[NSString stringWithFormat:@"[Cog %@] ", version]]
	];
	NSURLComponents *components =
	[NSURLComponents componentsWithString:@"https://github.com/losnoco/Cog/issues/new"];

	components.queryItems = query;

	[[NSWorkspace sharedWorkspace] openURL:components.URL];
}

- (void)initDefaults {
	NSMutableDictionary *userDefaultsValuesDict = [NSMutableDictionary dictionary];

	// Font defaults
	float fFontSize = [NSFont systemFontSizeForControlSize:NSControlSizeSmall];
	NSNumber *fontSize = @(fFontSize);
	[userDefaultsValuesDict setObject:fontSize forKey:@"fontSize"];

	NSString *feedURLdefault = @"https://cogcdn.cog.losno.co/mercury.xml";
	[userDefaultsValuesDict setObject:feedURLdefault forKey:@"SUFeedURL"];

	[userDefaultsValuesDict setObject:@"clearAndPlay" forKey:@"openingFilesBehavior"];
	[userDefaultsValuesDict setObject:@"enqueue" forKey:@"openingFilesAlteredBehavior"];

	[userDefaultsValuesDict setObject:@"albumGainWithPeak" forKey:@"volumeScaling"];

	[userDefaultsValuesDict setObject:@"cubic" forKey:@"resampling"];

	[userDefaultsValuesDict setObject:@(CogStatusStopped) forKey:@"lastPlaybackStatus"];
	[userDefaultsValuesDict setObject:@(-1) forKey:@"lastTrackPlaying"];
	[userDefaultsValuesDict setObject:@(0.0) forKey:@"lastTrackPosition"];

	[userDefaultsValuesDict setObject:@"dls appl" forKey:@"midiPlugin"];

	[userDefaultsValuesDict setObject:@"default" forKey:@"midi.flavor"];

	[userDefaultsValuesDict setObject:@(NO) forKey:@"resumePlaybackOnStartup"];

	[userDefaultsValuesDict setObject:@(NO) forKey:@"quitOnNaturalStop"];

	[userDefaultsValuesDict setObject:@(NO) forKey:@"spectrumFreqMode"];
	[userDefaultsValuesDict setObject:@(YES) forKey:@"spectrumProjectionMode"];

	NSValueTransformer *colorToValueTransformer = [NSValueTransformer valueTransformerForName:@"ColorToValueTransformer"];

	NSData *barColor = [colorToValueTransformer reverseTransformedValue:[NSColor colorWithSRGBRed:1.0 green:0.5 blue:0 alpha:1.0]];
	NSData *dotColor = [colorToValueTransformer reverseTransformedValue:[NSColor systemRedColor]];

	[userDefaultsValuesDict setObject:barColor forKey:@"spectrumBarColor"];
	[userDefaultsValuesDict setObject:dotColor forKey:@"spectrumDotColor"];

	// Register and sync defaults
	[[NSUserDefaults standardUserDefaults] registerDefaults:userDefaultsValuesDict];
	[[NSUserDefaults standardUserDefaults] synchronize];

	// And if the existing feed URL is broken due to my ineptitude with the above defaults, fix it
	NSSet<NSString *> *brokenFeedURLs = [NSSet setWithObjects:
	                                           @"https://kode54.net/cog/stable.xml",
	                                           @"https://kode54.net/cog/mercury.xml"
	                                           @"https://www.kode54.net/cog/mercury.xml",
	                                           @"https://f.losno.co/cog/mercury.xml",
	                                           nil];
	NSString *feedURL = [[NSUserDefaults standardUserDefaults] stringForKey:@"SUFeedURL"];
	if([brokenFeedURLs containsObject:feedURL]) {
		[[NSUserDefaults standardUserDefaults] setValue:feedURLdefault forKey:@"SUFeedURL"];
	}

	NSString *oldMidiPlugin = [[NSUserDefaults standardUserDefaults] stringForKey:@"midi.plugin"];
	if(oldMidiPlugin) {
		[[NSUserDefaults standardUserDefaults] setValue:oldMidiPlugin forKey:@"midiPlugin"];
		[[NSUserDefaults standardUserDefaults] removeObjectForKey:@"midi.plugin"];
	}

	// if([[[NSUserDefaults standardUserDefaults] stringForKey:@"midiPlugin"] isEqualToString:@"BASSMIDI"]) {
	//	[[NSUserDefaults standardUserDefaults] setValue:@"FluidSynth" forKey:@"midiPlugin"];
	// }
	if([[[NSUserDefaults standardUserDefaults] stringForKey:@"midiPlugin"] isEqualToString:@"FluidSynth"]) {
		[[NSUserDefaults standardUserDefaults] setValue:@"BASSMIDI" forKey:@"midiPlugin"];
	}
}

/* Unassign previous handler first, so dealloc can unregister it from the global map before the new instances are assigned */
- (void)registerHotKeys {
	MASShortcutBinder *binder = [MASShortcutBinder sharedBinder];
	[binder bindShortcutWithDefaultsKey:CogPlayShortcutKey
	                           toAction:^{
		                           [self clickPlay];
	                           }];

	[binder bindShortcutWithDefaultsKey:CogNextShortcutKey
	                           toAction:^{
		                           [self clickNext];
	                           }];

	[binder bindShortcutWithDefaultsKey:CogPrevShortcutKey
	                           toAction:^{
		                           [self clickPrev];
	                           }];

	[binder bindShortcutWithDefaultsKey:CogSpamShortcutKey
	                           toAction:^{
		                           [self clickSpam];
	                           }];
}

- (void)clickPlay {
	[playbackController playPauseResume:self];
}

- (void)clickPause {
	[playbackController pause:self];
}

- (void)clickStop {
	[playbackController stop:self];
}

- (void)clickPrev {
	[playbackController prev:nil];
}

- (void)clickNext {
	[playbackController next:nil];
}

- (void)clickSpam {
	[playbackController spam:nil];
}

- (void)clickSeek:(NSTimeInterval)position {
	[playbackController seek:self toTime:position];
}

- (void)changeFontSize:(float)size {
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	float fCurrentSize = [defaults floatForKey:@"fontSize"];
	NSNumber *newSize = @(fCurrentSize + size);
	[defaults setObject:newSize forKey:@"fontSize"];
}

- (IBAction)increaseFontSize:(id)sender {
	[self changeFontSize:1];
}

- (IBAction)decreaseFontSize:(id)sender {
	[self changeFontSize:-1];
}

- (IBAction)toggleMiniMode:(id)sender {
	[self setMiniMode:(!miniMode)];
}

- (BOOL)miniMode {
	return miniMode;
}

- (void)setMiniMode:(BOOL)newMiniMode {
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

	if(@available(macOS 11.0, *)) {
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

- (void)setFloatingMiniWindow:(BOOL)floatingMiniWindow {
	_floatingMiniWindow = floatingMiniWindow;
	[[NSUserDefaults standardUserDefaults] setBool:floatingMiniWindow forKey:@"floatingMiniWindow"];
	NSWindowLevel level = floatingMiniWindow ? NSFloatingWindowLevel : NSNormalWindowLevel;
	[miniWindow setLevel:level];
}

- (void)updateDockMenu:(NSNotification *)notification {
	PlaylistEntry *pe = [playlistController currentEntry];

	BOOL hideItem = NO;

	if([[notification name] isEqualToString:CogPlaybackDidStopNotficiation] || !pe || ![pe artist] || [[pe artist] isEqualToString:@""])
		hideItem = YES;

	if(hideItem && [dockMenu indexOfItem:currentArtistItem] == 0) {
		[dockMenu removeItem:currentArtistItem];
	} else if(!hideItem && [dockMenu indexOfItem:currentArtistItem] < 0) {
		[dockMenu insertItem:currentArtistItem atIndex:0];
	}
}

@end
