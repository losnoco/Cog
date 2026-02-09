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
#import "RubberbandEngineTransformer.h"
#import "SQLiteStore.h"
#import "SandboxBroker.h"
#import "SpotlightWindowController.h"
#import "StringToURLTransformer.h"
#import <CogAudio/Status.h>

#import "DualWindow.h"
#import "Logging.h"
#import "MiniModeMenuTitleTransformer.h"

#import "ColorToValueTransformer.h"
#import "MaybeSecureValueDataTransformer.h"
#import "TotalTimeTransformer.h"

#import "Shortcuts.h"
#import <MASShortcut/Shortcut.h>
#import <MASShortcut/MASDictionaryTransformer.h>

#import <Sparkle/Sparkle.h>

#import "PreferencesController.h"

#import "FeedbackController.h"

@import Sentry;

void *kAppControllerContext = &kAppControllerContext;

BOOL kAppControllerShuttingDown = NO;

static AppController *kAppController = nil;

@interface SparkleBridge : NSObject
+ (SPUStandardUpdaterController *)sharedStandardUpdaterController;
@end

@implementation SparkleBridge

+ (SPUStandardUpdaterController *)sharedStandardUpdaterController {
	static SPUStandardUpdaterController *sharedStandardUpdaterController_ = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		sharedStandardUpdaterController_ = [[SPUStandardUpdaterController alloc] initWithUpdaterDelegate: nil userDriverDelegate: nil];
	});
	return sharedStandardUpdaterController_;
}
@end

@implementation AppController {
	BOOL _isFullToolbarStyle;
}

@synthesize mainWindow;
@synthesize miniWindow;

+ (void)initialize {
	// Register transformers
	NSValueTransformer *stringToURLTransformer = [StringToURLTransformer new];
	[NSValueTransformer setValueTransformer:stringToURLTransformer
	                                forName:@"StringToURLTransformer"];

	NSValueTransformer *fontSizetoLineHeightTransformer =
	[FontSizetoLineHeightTransformer new];
	[NSValueTransformer setValueTransformer:fontSizetoLineHeightTransformer
	                                forName:@"FontSizetoLineHeightTransformer"];

	NSValueTransformer *miniModeMenuTitleTransformer = [MiniModeMenuTitleTransformer new];
	[NSValueTransformer setValueTransformer:miniModeMenuTitleTransformer
	                                forName:@"MiniModeMenuTitleTransformer"];

	NSValueTransformer *colorToValueTransformer = [ColorToValueTransformer new];
	[NSValueTransformer setValueTransformer:colorToValueTransformer
	                                forName:@"ColorToValueTransformer"];

	NSValueTransformer *totalTimeTransformer = [TotalTimeTransformer new];
	[NSValueTransformer setValueTransformer:totalTimeTransformer
	                                forName:@"TotalTimeTransformer"];
	
	NSValueTransformer *numberHertzToStringTransformer = [NumberHertzToStringTransformer new];
	[NSValueTransformer setValueTransformer:numberHertzToStringTransformer
									forName:@"NumberHertzToStringTransformer"];

	NSValueTransformer *rubberbandEngineEnabledTransformer = [RubberbandEngineEnabledTransformer new];
	[NSValueTransformer setValueTransformer:rubberbandEngineEnabledTransformer
									forName:@"RubberbandEngineEnabledTransformer"];

	NSValueTransformer *rubberbandEngineHiddenTransformer = [RubberbandEngineHiddenTransformer new];
	[NSValueTransformer setValueTransformer:rubberbandEngineHiddenTransformer
									forName:@"RubberbandEngineHiddenTransformer"];

	NSValueTransformer *maybeSecureValueDataTransformer = [MaybeSecureValueDataTransformer new];
	[NSValueTransformer setValueTransformer:maybeSecureValueDataTransformer
									forName:@"MaybeSecureValueDataTransformer"];
}
- (id)init {
	self = [super init];
	if(self) {
		[self initDefaults];

		queue = [NSOperationQueue new];

		kAppController = self;
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
			          NSDictionary *loadEntryData = @{@"entries": [p URLs],
				                                      @"sort": @(YES),
				                                      @"origin": @(URLOriginExternal)};
			          [self->playlistController performSelectorInBackground:@selector(addURLsInBackground:) withObject:loadEntryData];
		          } else {
			          [p close];
		          }
	          }];
}

- (void)savePlaylistBase:(BOOL)selection {
	NSSavePanel *p;

	p = [NSSavePanel savePanel];

	/* Yes, this is deprecated. Yes, this is required to give the dialog
	 * a default set of filename extensions to save, including adding an
	 * extension if the user does not supply one. */
	[p setAllowedFileTypes:@[@"m3u", @"pls"]];

	[p beginSheetModalForWindow:mainWindow
	          completionHandler:^(NSInteger result) {
		          if(result == NSModalResponseOK) {
			          [self->playlistLoader save:[[p URL] path] onlySelection:selection];
		          } else {
			          [p close];
		          }
	          }];
}

- (IBAction)savePlaylist:(id)sender {
	[self savePlaylistBase:NO];
}

- (IBAction)savePlaylistFromSelection:(id)sender {
	[self savePlaylistBase:YES];
}

- (IBAction)openURL:(id)sender {
	OpenURLPanel *p;

	p = [OpenURLPanel openURLPanel];

	[p beginSheetWithWindow:mainWindow delegate:self didEndSelector:@selector(openURLPanelDidEnd:returnCode:contextInfo:) contextInfo:nil];
}

- (void)openURLPanelDidEnd:(OpenURLPanel *)panel returnCode:(int)returnCode contextInfo:(void *)contextInfo {
	if(returnCode == NSModalResponseOK) {
		NSDictionary *loadEntriesData = @{ @"entries": @[[panel url]],
			                               @"sort": @(NO),
			                               @"origin": @(URLOriginExternal) };
		[playlistController performSelectorInBackground:@selector(addURLsInBackground:) withObject:loadEntriesData];
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

static BOOL consentLastEnabled = NO;

- (void)awakeFromNib {
	[[NSUserDefaults standardUserDefaults] registerDefaults:@{ @"sentryConsented": @(NO),
															   @"sentryAskedConsent": @(NO) }];
	
	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.sentryConsented" options:(NSKeyValueObservingOptionInitial | NSKeyValueObservingOptionNew) context:kAppControllerContext];

#ifdef DEBUG
	// Prevent updates automatically in debug builds
	[[[SparkleBridge sharedStandardUpdaterController] updater] setAutomaticallyChecksForUpdates:NO];
#endif
	[[[SparkleBridge sharedStandardUpdaterController] updater] setUpdateCheckInterval:3600];

	[[totalTimeField cell] setBackgroundStyle:NSBackgroundStyleRaised];

	[self.infoButton setToolTip:NSLocalizedString(@"InfoButtonTooltip", @"")];
	[self.infoButtonMini setToolTip:NSLocalizedString(@"InfoButtonTooltip", @"")];
	[shuffleButton setToolTip:NSLocalizedString(@"ShuffleButtonTooltip", @"")];
	[repeatButton setToolTip:NSLocalizedString(@"RepeatButtonTooltip", @"")];
	[randomizeButton setToolTip:NSLocalizedString(@"RandomizeButtonTooltip", @"")];
	[fileButton setToolTip:NSLocalizedString(@"FileButtonTooltip", @"")];

	[self registerDefaultHotKeys];

	[self migrateHotKeys];

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
		} else if([[NSFileManager defaultManager] fileExistsAtPath:[basePath stringByAppendingPathComponent:oldFilename]]){
			/* Without the above check, it appears the code was retrieving a nil NSURL from the nonexistent path
			 * Then adding it to the playlist and crashing further down the line
			 * Nobody on a new setup should be seeing this open anything, so it should fall through to the
			 * notice below.
			 */
			[playlistLoader addURL:[NSURL fileURLWithPath:[basePath stringByAppendingPathComponent:oldFilename]]];
		} else {
			ALog(@"No playlist found, leaving it empty.");
		}
	}

	SandboxBroker *sandboxBroker = [SandboxBroker sharedSandboxBroker];
	if(!sandboxBroker) {
		ALog(@"Sandbox broker init failed.");
	}
	[SandboxBroker cleanupFolderAccess];

	[[playlistController undoManager] enableUndoRegistration];

	int lastStatus = (int)[[NSUserDefaults standardUserDefaults] integerForKey:@"lastPlaybackStatus"];

	if(lastStatus != CogStatusStopped) {
		NSPredicate *hasUrlPredicate = [NSPredicate predicateWithFormat:@"urlString != nil && urlString != %@", @""];
		NSPredicate *deletedPredicate = [NSPredicate predicateWithFormat:@"deLeted == NO || deLeted == nil"];
		NSPredicate *currentPredicate = [NSPredicate predicateWithFormat:@"current == YES"];

		NSCompoundPredicate *predicate = [NSCompoundPredicate andPredicateWithSubpredicates:@[deletedPredicate, hasUrlPredicate, currentPredicate]];

		NSFetchRequest *request = [NSFetchRequest fetchRequestWithEntityName:@"PlaylistEntry"];
		request.predicate = predicate;

		NSError *error = nil;
		NSArray *results = [playlistController.persistentContainer.viewContext executeFetchRequest:request error:&error];

		if(results && [results count] > 0) {
			PlaylistEntry *pe = results[0];
			// Select this track
			[playlistView selectRowIndexes:[NSIndexSet indexSetWithIndex:pe.index] byExtendingSelection:NO];
			if([[NSUserDefaults standardUserDefaults] boolForKey:@"resumePlaybackOnStartup"]) {
				// And play it
				[playbackController playEntryAtIndex:pe.index startPaused:(lastStatus == CogStatusPaused) andSeekTo:@(pe.currentPosition)];
			}
			// Bug fix
			if([results count] > 1) {
				for(size_t i = 1; i < [results count]; ++i) {
					PlaylistEntry *pe = results[i];
					[pe setCurrent:NO];
				}
				[playlistController commitPersistentStore];
			}
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

	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateDockMenu:) name:CogPlaybackDidBeginNotificiation object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(updateDockMenu:) name:CogPlaybackDidStopNotificiation object:nil];

	[self updateDockMenu:nil];

	NSArray *expandedNodesArray = [[NSUserDefaults standardUserDefaults] valueForKey:@"fileTreeViewExpandedNodes"];

	if(expandedNodesArray) {
		expandedNodes = [[NSMutableSet alloc] initWithArray:expandedNodesArray];
	} else {
		expandedNodes = [NSMutableSet new];
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
	
	if([keyPath isEqualToString:@"values.sentryConsented"]) {
		BOOL enabled = [[NSUserDefaults standardUserDefaults] boolForKey:@"sentryConsented"];
		if(enabled != consentLastEnabled) {
			if(enabled) {
				[SentrySDK startWithConfigureOptions:^(SentryOptions *options) {
					options.dsn = @"https://d70ca316b053af0573f4b48f742d4d8e@cog-analytics.losno.co/5";
					options.debug = YES; // Enabled debug when first installing is always helpful

					// Disable hang detection, lots of false positives still
					options.enableAppHangTracking = NO;

					// Enable logging
					options.enableLogs = YES;

					options.configureProfiling = ^(SentryProfileOptions * _Nonnull profiling) {
						profiling.sessionSampleRate = 1.f;
						profiling.lifecycle = SentryProfileLifecycleTrace;
					};

					// And now to set up user feedback prompting
					options.onCrashedLastRun = ^void(SentryEvent * _Nonnull event) {
						// capture user feedback
						FeedbackController *fbcon = [FeedbackController new];
						[fbcon performSelectorOnMainThread:@selector(showWindow:) withObject:nil waitUntilDone:YES];
						if([fbcon waitForCompletion]) {
							SentryFeedback *feedback = [[SentryFeedback alloc] initWithMessage:[fbcon comments] name:[fbcon name] email:[fbcon email] source:SentryFeedbackSourceCustom associatedEventId:event.eventId attachments:nil];

							[SentrySDK captureFeedback:feedback];
						}
					};
				}];
			} else {
				if([SentrySDK isEnabled]) {
					[SentrySDK close];
				}
			}
			consentLastEnabled = enabled;
		}
	} else if([keyPath isEqualToString:@"playlistController.currentEntry"]) {
		PlaylistEntry *entry = playlistController.currentEntry;
		NSString *appTitle = NSLocalizedString(@"CogTitle", @"");
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
			// currently 16x16
			//NSSize frameSize = [NSImage imageNamed:@"infoTemplate"].size;
			//const CGFloat sizeX = frameSize.width;
			//const CGFloat sizeY = frameSize.height;
			const CGFloat sizeX = 16.0;
			const CGFloat sizeY = 16.0;

			NSImage *newButtonImage = [[NSImage alloc] initWithSize:NSMakeSize(sizeX, sizeY)];
			NSSize artSize = entry.albumArt.size;
			CGFloat maxDim = MAX(artSize.width, artSize.height);
			CGFloat ratioX = artSize.width / maxDim;
			CGFloat ratioY = artSize.height / maxDim;
			CGFloat posX = (maxDim - artSize.width) / 2.0;
			CGFloat posY = (maxDim - artSize.height) / 2.0;
			posX = posX * sizeX / maxDim;
			posY = posY * sizeY / maxDim;

			[newButtonImage lockFocus];
			[entry.albumArt drawInRect:NSMakeRect(posX, posY, sizeX * ratioX, sizeY * ratioY)
							  fromRect:NSMakeRect(0, 0, artSize.width, artSize.height)
							 operation:NSCompositingOperationSourceOver
							  fraction:1.0];
			[newButtonImage unlockFocus];

			self.infoButton.imageScaling = NSImageScaleProportionallyUpOrDown;
			self.infoButton.image = newButtonImage;
			self.infoButtonMini.imageScaling = NSImageScaleProportionallyUpOrDown;
			self.infoButtonMini.image = newButtonImage;
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
	kAppControllerShuttingDown = YES;

	CogStatus currentStatus = [playbackController playbackStatus];

	if(currentStatus == CogStatusStopping)
		currentStatus = CogStatusStopped;

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

	// How the heck are people getting this in their playlists
	for(PlaylistEntry *pe in playlistController.arrangedObjects) {
		if(pe.deLeted || !pe.urlString || [pe.urlString isEqualToString:@""] || !pe.url) {
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

	DLog(@"Shutting down sandbox broker");
	[[SandboxBroker sharedSandboxBroker] shutdown];

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
	NSDictionary *loadEntriesData = @{ @"entries": urls,
		                               @"sort": @(NO),
		                               @"origin": @(URLOriginExternal) };
	[playlistController performSelectorInBackground:@selector(addURLsInBackground:) withObject:loadEntriesData];
	return YES;
}

- (void)application:(NSApplication *)theApplication openFiles:(NSArray *)filenames {
	// Need to convert to urls
	NSMutableArray *urls = [NSMutableArray array];

	for(NSString *filename in filenames) {
		NSURL *url = nil;
		if([[NSFileManager defaultManager] fileExistsAtPath:filename]) {
			url = [NSURL fileURLWithPath:filename];
		} else {
			if([filename hasPrefix:@"/http/::"] ||
			   [filename hasPrefix:@"/https/::"]) {
				// Stupid Carbon bodge for AppleScript
				NSString *method = nil;
				NSString *server = nil;
				NSString *path = nil;

				NSScanner *objScanner = [NSScanner scannerWithString:filename];

				if(![objScanner scanString:@"/" intoString:nil] ||
				   ![objScanner scanUpToString:@"/" intoString:&method] ||
				   ![objScanner scanString:@"/::" intoString:nil] ||
				   ![objScanner scanUpToString:@":" intoString:&server] ||
				   ![objScanner scanString:@":" intoString:nil]) {
					continue;
				}
				[objScanner scanUpToCharactersFromSet:[NSCharacterSet illegalCharacterSet] intoString:&path];
				// Colons in server were converted to shashes, convert back
				NSString *convertedServer = [server stringByReplacingOccurrencesOfString:@"/" withString:@":"];
				// Slashes in path were converted to colons, convert back
				NSString *convertedPath = [path stringByReplacingOccurrencesOfString:@":" withString:@"/"];
				url = [NSURL URLWithString:[NSString stringWithFormat:@"%@://%@/%@", method, convertedServer, convertedPath]];
			}
		}
		if(url) {
			[urls addObject:url];
		}
	}

	NSDictionary *loadEntriesData = @{ @"entries": urls,
		                               @"sort": @(YES),
		                               @"origin": @(URLOriginExternal) };

	[playlistController performSelectorInBackground:@selector(addURLsInBackground:) withObject:loadEntriesData];

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

- (IBAction)privacyPolicy:(id)sender {
    [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:NSLocalizedString(@"PrivacyPolicyURL", @"Privacy policy URL from Iubenda.")]];
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
	float fFontSize = [NSFont systemFontSizeForControlSize:NSControlSizeRegular];
	NSNumber *fontSize = @(fFontSize);
	[userDefaultsValuesDict setObject:fontSize forKey:@"fontSize"];

	[userDefaultsValuesDict setObject:@"enqueueAndPlay" forKey:@"openingFilesBehavior"];
	[userDefaultsValuesDict setObject:@"enqueue" forKey:@"openingFilesAlteredBehavior"];

	[userDefaultsValuesDict setObject:@"albumGainWithPeak" forKey:@"volumeScaling"];

	[userDefaultsValuesDict setObject:@"cubic" forKey:@"resampling"];

	[userDefaultsValuesDict setObject:@(CogStatusStopped) forKey:@"lastPlaybackStatus"];

	[userDefaultsValuesDict setObject:@"BASSMIDI" forKey:@"midiPlugin"];

	[userDefaultsValuesDict setObject:@"default" forKey:@"midi.flavor"];

	[userDefaultsValuesDict setObject:@(NO) forKey:@"resumePlaybackOnStartup"];

	[userDefaultsValuesDict setObject:@(NO) forKey:@"quitOnNaturalStop"];

	[userDefaultsValuesDict setObject:@(NO) forKey:@"spectrumFreqMode"];
	[userDefaultsValuesDict setObject:@(YES) forKey:@"spectrumProjectionMode"];

	NSValueTransformer *colorToValueTransformer = [NSValueTransformer valueTransformerForName:@"ColorToValueTransformer"];

	NSData *barColor = [colorToValueTransformer reverseTransformedValue:[NSColor colorWithSRGBRed:1.0 green:0.5 blue:0 alpha:1.0]];
	NSData *dotColor = [colorToValueTransformer reverseTransformedValue:[NSColor systemRedColor]];

	[userDefaultsValuesDict setObject:@(YES) forKey:@"spectrumSceneKit"];
	[userDefaultsValuesDict setObject:barColor forKey:@"spectrumBarColor"];
	[userDefaultsValuesDict setObject:dotColor forKey:@"spectrumDotColor"];

	[userDefaultsValuesDict setObject:@(150.0) forKey:@"synthDefaultSeconds"];
	[userDefaultsValuesDict setObject:@(8.0) forKey:@"synthDefaultFadeSeconds"];
	[userDefaultsValuesDict setObject:@(2) forKey:@"synthDefaultLoopCount"];
	[userDefaultsValuesDict setObject:@(44100) forKey:@"synthSampleRate"];

	[userDefaultsValuesDict setObject:@NO forKey:@"alwaysStopAfterCurrent"];
	[userDefaultsValuesDict setObject:@NO forKey:@"selectionFollowsPlayback"];

	// Register and sync defaults
	[[NSUserDefaults standardUserDefaults] registerDefaults:userDefaultsValuesDict];
	[[NSUserDefaults standardUserDefaults] synchronize];

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

	NSString *midiPlugin = [[NSUserDefaults standardUserDefaults] stringForKey:@"midiPlugin"];
	if([midiPlugin length] == 8 && [[midiPlugin substringFromIndex:4] isEqualToString:@"appl"]) {
		[[NSUserDefaults standardUserDefaults] setObject:@"BASSMIDI" forKey:@"midiPlugin"];
	}
}

MASShortcut *shortcutWithMigration(NSString *oldKeyCodePrefName,
								   NSString *oldKeyModifierPrefName,
								   NSString *newShortcutPrefName,
								   NSInteger newDefaultKeyCode) {
	NSEventModifierFlags defaultModifiers = NSEventModifierFlagControl | NSEventModifierFlagCommand;
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	if([defaults objectForKey:oldKeyCodePrefName]) {
		NSInteger oldKeyCode = [defaults integerForKey:oldKeyCodePrefName];
		NSEventModifierFlags oldKeyModifiers = [defaults integerForKey:oldKeyModifierPrefName];
		// Should we consider temporarily save these values for further migration?
		[defaults removeObjectForKey:oldKeyCodePrefName];
		[defaults removeObjectForKey:oldKeyModifierPrefName];
		return [MASShortcut shortcutWithKeyCode:oldKeyCode modifierFlags:oldKeyModifiers];
	} else {
		return [MASShortcut shortcutWithKeyCode:newDefaultKeyCode modifierFlags:defaultModifiers];
	}
}

static NSDictionary *shortcutDefaults = nil;

- (void)registerDefaultHotKeys {
	MASShortcut *playShortcut = shortcutWithMigration(@"hotKeyPlayKeyCode",
													  @"hotKeyPlayModifiers",
													  CogPlayShortcutKey,
													  kVK_ANSI_P);
	MASShortcut *nextShortcut = shortcutWithMigration(@"hotKeyNextKeyCode",
													  @"hotKeyNextModifiers",
													  CogNextShortcutKey,
													  kVK_ANSI_N);
	MASShortcut *prevShortcut = shortcutWithMigration(@"hotKeyPreviousKeyCode",
													  @"hotKeyPreviousModifiers",
													  CogPrevShortcutKey,
													  kVK_ANSI_R);
	MASShortcut *spamShortcut = [MASShortcut shortcutWithKeyCode:kVK_ANSI_C
												   modifierFlags:NSEventModifierFlagControl | NSEventModifierFlagCommand];
	MASShortcut *fadeShortcut = [MASShortcut shortcutWithKeyCode:kVK_ANSI_O
												   modifierFlags:NSEventModifierFlagControl | NSEventModifierFlagCommand];
	MASShortcut *seekBkwdShortcut = [MASShortcut shortcutWithKeyCode:kVK_LeftArrow
													   modifierFlags:NSEventModifierFlagControl | NSEventModifierFlagCommand];
	MASShortcut *seekFwdShortcut = [MASShortcut shortcutWithKeyCode:kVK_RightArrow
													  modifierFlags:NSEventModifierFlagControl | NSEventModifierFlagCommand];

	MASDictionaryTransformer *transformer = [MASDictionaryTransformer new];
	NSDictionary *playShortcutDict = [transformer reverseTransformedValue:playShortcut];
	NSDictionary *nextShortcutDict = [transformer reverseTransformedValue:nextShortcut];
	NSDictionary *prevShortcutDict = [transformer reverseTransformedValue:prevShortcut];
	NSDictionary *spamShortcutDict = [transformer reverseTransformedValue:spamShortcut];
	NSDictionary *fadeShortcutDict = [transformer reverseTransformedValue:fadeShortcut];
	NSDictionary *seekBkwdShortcutDict = [transformer reverseTransformedValue:seekBkwdShortcut];
	NSDictionary *seekFwdShortcutDict = [transformer reverseTransformedValue:seekFwdShortcut];

	// Register default values to be used for the first app start
	NSDictionary<NSString *, NSDictionary *> *defaultShortcuts = @{
		CogPlayShortcutKey: playShortcutDict,
		CogNextShortcutKey: nextShortcutDict,
		CogPrevShortcutKey: prevShortcutDict,
		CogSpamShortcutKey: spamShortcutDict,
		CogFadeShortcutKey: fadeShortcutDict,
		CogSeekBackwardShortcutKey: seekBkwdShortcutDict,
		CogSeekForwardShortcutKey: seekFwdShortcutDict
	};

	shortcutDefaults = defaultShortcuts;

	[[NSUserDefaults standardUserDefaults] registerDefaults:defaultShortcuts];
}

- (IBAction)resetHotkeys:(id)sender {
	[shortcutDefaults enumerateKeysAndObjectsUsingBlock:^(id  _Nonnull key, id  _Nonnull obj, BOOL * _Nonnull stop) {
		[[NSUserDefaults standardUserDefaults] setObject:obj forKey:key];
	}];
}

- (void)migrateHotKeys {
	NSArray *inKeys = @[CogPlayShortcutKeyV1, CogNextShortcutKeyV1, CogPrevShortcutKeyV1, CogSpamShortcutKeyV1, CogFadeShortcutKeyV1, CogSeekBackwardShortcutKeyV1, CogSeekForwardShortcutKeyV1];
	NSArray *outKeys = @[CogPlayShortcutKey, CogNextShortcutKey, CogPrevShortcutKey, CogSpamShortcutKey, CogFadeShortcutKey, CogSeekBackwardShortcutKey, CogSeekForwardShortcutKey];
	for(size_t i = 0, j = [inKeys count]; i < j; ++i) {
		NSString *inKey = inKeys[i];
		NSString *outKey = outKeys[i];
		id value = [[NSUserDefaults standardUserDefaults] objectForKey:inKey];
		if(value && value != [NSNull null]) {
			[[NSUserDefaults standardUserDefaults] setObject:value forKey:outKey];
			[[NSUserDefaults standardUserDefaults] removeObjectForKey:inKey];
		}
	}
}

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

	[binder bindShortcutWithDefaultsKey:CogFadeShortcutKey
	                           toAction:^{
	                               [self clickFade];
	                           }];

	[binder bindShortcutWithDefaultsKey:CogSeekBackwardShortcutKey
	                           toAction:^{
	                               [self clickSeekBack];
	                           }];

	[binder bindShortcutWithDefaultsKey:CogSeekForwardShortcutKey
	                           toAction:^{
	                               [self clickSeekForward];
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

- (void)clickFade {
	[playbackController fade:nil];
}

- (void)clickSeek:(NSTimeInterval)position {
	[playbackController seek:self toTime:position];
}

- (void)clickSeekBack {
	[playbackController seekBackward:10.0];
}

- (void)clickSeekForward {
	[playbackController seekForward:10.0];
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

	if([[notification name] isEqualToString:CogPlaybackDidStopNotificiation] || !pe || ![pe artist] || [[pe artist] isEqualToString:@""])
		hideItem = YES;

	if(hideItem && [dockMenu indexOfItem:currentArtistItem] == 0) {
		[dockMenu removeItem:currentArtistItem];
	} else if(!hideItem && [dockMenu indexOfItem:currentArtistItem] < 0) {
		[dockMenu insertItem:currentArtistItem atIndex:0];
	}
}

- (BOOL)pathSuggesterEmpty {
	return [playlistController pathSuggesterEmpty];
}

+ (BOOL)globalPathSuggesterEmpty {
	return [kAppController pathSuggesterEmpty];
}

- (void)showPathSuggester {
	[preferencesController showPathSuggester:self];
}

+ (void)globalShowPathSuggester {
	[kAppController showPathSuggester];
}

- (void)showRubberbandSettings:(id)sender {
	[preferencesController showRubberbandSettings:sender];
}

+ (void)globalShowRubberbandSettings {
	[kAppController showRubberbandSettings:kAppController];
}

- (IBAction)checkForUpdates:(id)sender {
	[[SparkleBridge sharedStandardUpdaterController] checkForUpdates:[[NSApplication sharedApplication] delegate]];
}

- (void)selectTrack:(id)sender {
	PlaylistEntry *pe = (PlaylistEntry *)sender;
	@try {
		[playlistView selectRowIndexes:[NSIndexSet indexSetWithIndex:pe.index] byExtendingSelection:NO];
	}
	@catch(NSException *e) {
	}
}

@end
