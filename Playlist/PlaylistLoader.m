//
//  PlaylistLoader.m
//  Cog
//
//  Created by Vincent Spader on 3/05/07.
//  Copyright 2007 Vincent Spader All rights reserved.
//

#include <objc/runtime.h>

#include <mach/semaphore.h>

#import "Cog-Swift.h"
#import <CoreData/CoreData.h>

#import "AppController.h"
#import "PlaylistController.h"
#import "PlaylistEntry.h"
#import "PlaylistLoader.h"

#import "NSFileHandle+CreateFile.h"

#import "CogAudio/AudioContainer.h"
#import "CogAudio/AudioMetadataReader.h"
#import "CogAudio/AudioPlayer.h"
#import "CogAudio/AudioPropertiesReader.h"

#import "XmlContainer.h"

#import "NSData+MD5.h"

#import "NSString+FinderCompare.h"

#import "SQLiteStore.h"

#import "Logging.h"

#import "NSDictionary+Merge.h"

#import "RedundantPlaylistDataStore.h"

#import "SandboxBroker.h"

@import Sentry;

extern NSMutableDictionary<NSString *, AlbumArtwork *> *kArtworkDictionary;

@implementation PlaylistLoader

- (id)init {
	self = [super init];
	if(self) {
		[self initDefaults];

		containerQueue = [[NSOperationQueue alloc] init];
		[containerQueue setMaxConcurrentOperationCount:8];

		queue = [[NSOperationQueue alloc] init];
		[queue setMaxConcurrentOperationCount:8];

		queuedURLs = [[NSMutableDictionary alloc] init];
	}

	return self;
}

- (void)initDefaults {
	NSDictionary *defaultsDictionary = @{ @"readCueSheetsInFolders": @NO,
		                                  @"readPlaylistsInFolders": @NO,
		                                  @"addOtherFilesInFolders": @NO };

	[[NSUserDefaults standardUserDefaults] registerDefaults:defaultsDictionary];
}

- (BOOL)save:(NSString *)filename {
	NSString *ext = [filename pathExtension];
	if([ext isEqualToString:@"pls"]) {
		return [self save:filename asType:kPlaylistPls];
	} else {
		return [self save:filename asType:kPlaylistM3u];
	}
}

- (BOOL)save:(NSString *)filename asType:(PlaylistType)type {
	if(type == kPlaylistM3u) {
		return [self saveM3u:filename];
	} else if(type == kPlaylistPls) {
		return [self savePls:filename];
	} else if(type == kPlaylistXml) {
		return [self saveXml:filename];
	}

	return NO;
}

- (NSString *)relativePathFrom:(NSString *)filename toURL:(NSURL *)entryURL {
	NSString *basePath = [[[filename stringByStandardizingPath] stringByDeletingLastPathComponent] stringByAppendingString:@"/"];

	if([entryURL isFileURL]) {
		// We want relative paths.
		NSMutableString *entryPath = [[[entryURL path] stringByStandardizingPath] mutableCopy];

		[entryPath replaceOccurrencesOfString:basePath withString:@"" options:(NSAnchoredSearch | NSLiteralSearch | NSCaseInsensitiveSearch) range:NSMakeRange(0, [entryPath length])];
		if([entryURL fragment]) {
			[entryPath appendString:@"#"];
			[entryPath appendString:[entryURL fragment]];
		}

		return entryPath;
	} else {
		// Write [entryURL absoluteString] to file
		return [entryURL absoluteString];
	}
}

- (BOOL)saveM3u:(NSString *)filename {
	NSFileHandle *fileHandle = [NSFileHandle fileHandleForWritingAtPath:filename createFile:YES];
	if(!fileHandle) {
		ALog(@"Error saving m3u!");
		return NO;
	}
	[fileHandle truncateFileAtOffset:0];
	[fileHandle writeData:[@"#\n" dataUsingEncoding:NSUTF8StringEncoding]];

	for(PlaylistEntry *pe in [playlistController arrangedObjects]) {
		NSString *path = [self relativePathFrom:filename toURL:pe.url];
		[fileHandle writeData:[[path stringByAppendingString:@"\n"] dataUsingEncoding:NSUTF8StringEncoding]];
	}

	[fileHandle closeFile];

	return YES;
}

- (BOOL)savePls:(NSString *)filename {
	NSFileHandle *fileHandle = [NSFileHandle fileHandleForWritingAtPath:filename createFile:YES];
	if(!fileHandle) {
		return NO;
	}
	[fileHandle truncateFileAtOffset:0];

	[fileHandle writeData:[[NSString stringWithFormat:@"[playlist]\nnumberOfEntries=%lu\n\n", (unsigned long)[[playlistController content] count]] dataUsingEncoding:NSUTF8StringEncoding]];

	int i = 1;
	for(PlaylistEntry *pe in [playlistController arrangedObjects]) {
		NSString *path = [self relativePathFrom:filename toURL:pe.url];
		NSString *entry = [NSString stringWithFormat:@"File%i=%@\n", i, path];

		[fileHandle writeData:[entry dataUsingEncoding:NSUTF8StringEncoding]];
		i++;
	}

	[fileHandle writeData:[@"\nVERSION=2" dataUsingEncoding:NSUTF8StringEncoding]];
	[fileHandle closeFile];

	return YES;
}

NSMutableDictionary *dictionaryWithPropertiesOfObject(id obj, NSArray *filterList) {
	NSMutableDictionary *dict = [NSMutableDictionary dictionary];

	Class class = [obj class];

	do {
		unsigned count;
		objc_property_t *properties = class_copyPropertyList(class, &count);

		for(int i = 0; i < count; i++) {
			NSString *key = [NSString stringWithUTF8String:property_getName(properties[i])];
			if([filterList containsObject:key]) continue;

			Class classObject = NSClassFromString([key capitalizedString]);
			if(classObject) {
				id subObj = dictionaryWithPropertiesOfObject([obj valueForKey:key], filterList);
				[dict setObject:subObj forKey:key];
			} else {
				id value = [obj valueForKey:key];
				if(value) [dict setObject:value forKey:key];
			}
		}

		free(properties);

		if(count) break;

		class = [class superclass];
	} while(class);

	return dict;
}

- (BOOL)saveXml:(NSString *)filename {
	NSFileHandle *fileHandle = [NSFileHandle fileHandleForWritingAtPath:filename createFile:YES];
	if(!fileHandle) {
		return NO;
	}
	[fileHandle truncateFileAtOffset:0];

	NSArray *filterList = @[@"display", @"length", @"path", @"filename", @"status", @"statusMessage", @"spam", @"lengthText", @"positionText", @"stopAfter", @"shuffleIndex", @"index", @"current", @"queued", @"currentPosition", @"queuePosition", @"error", @"removed", @"URL", @"albumArt"];

	NSMutableDictionary *albumArtSet = [[NSMutableDictionary alloc] init];

	NSMutableArray *topLevel = [[NSMutableArray alloc] init];

	for(PlaylistEntry *pe in [playlistController arrangedObjects]) {
		BOOL error = [pe error];

		NSMutableDictionary *dict = dictionaryWithPropertiesOfObject(pe, filterList);

		NSString *path = [self relativePathFrom:filename toURL:pe.url];

		[dict setObject:path forKey:@"URL"];
		NSData *albumArt = [dict objectForKey:@"albumArtInternal"];
		if(albumArt) {
			[dict removeObjectForKey:@"albumArtInternal"];
			NSString *hash = [albumArt MD5];
			if(![albumArtSet objectForKey:hash])
				[albumArtSet setObject:albumArt forKey:hash];
			[dict setObject:hash forKey:@"albumArt"];
		}

		if(error)
			[dict removeObjectForKey:@"metadataLoaded"];

		[topLevel addObject:dict];
	}

	NSMutableArray *queueList = [[NSMutableArray alloc] init];

	for(PlaylistEntry *pe in [playlistController queueList]) {
		[queueList addObject:@(pe.index)];
	}

	NSDictionary *dictionary = @{@"albumArt": albumArtSet, @"queue": queueList, @"items": topLevel};

	NSError *err;

	NSData *data = [NSPropertyListSerialization dataWithPropertyList:dictionary format:NSPropertyListXMLFormat_v1_0 options:0 error:&err];

	[fileHandle writeData:data];

	[fileHandle closeFile];

	return YES;
}

- (NSArray *)fileURLsAtPath:(NSString *)path {
	NSFileManager *manager = [NSFileManager defaultManager];

	NSMutableArray *urls = [NSMutableArray array];

	const void *sbHandle = [[SandboxBroker sharedSandboxBroker] beginFolderAccess:[NSURL fileURLWithPath:path]];
	NSArray *subpaths = [manager subpathsAtPath:path];
	[[SandboxBroker sharedSandboxBroker] endFolderAccess:sbHandle];

	BOOL readCueSheets = [[NSUserDefaults standardUserDefaults] boolForKey:@"readCueSheetsInFolders"];
	BOOL readPlaylists = [[NSUserDefaults standardUserDefaults] boolForKey:@"readPlaylistsInFolders"];

	for(NSString *subpath in subpaths) {
		NSString *absoluteSubpath = [NSString pathWithComponents:@[path, subpath]];

		BOOL isDir;
		if([manager fileExistsAtPath:absoluteSubpath isDirectory:&isDir] && isDir == NO) {
			BOOL readFile = YES;
			NSString *ext = [absoluteSubpath pathExtension];
			if([ext caseInsensitiveCompare:@"cue"] == NSOrderedSame) {
				readFile = readCueSheets;
			} else if([ext caseInsensitiveCompare:@"m3u"] == NSOrderedSame ||
					  [ext caseInsensitiveCompare:@"m3u8"] == NSOrderedSame ||
					  [ext caseInsensitiveCompare:@"pls"] == NSOrderedSame) {
				readFile = readPlaylists;
			}
			if(readFile) {
				[urls addObject:[NSURL fileURLWithPath:absoluteSubpath]];
			}
		}
	}

	return urls;
}

static inline void dispatch_sync_reentrant(dispatch_queue_t queue, dispatch_block_t block) {
	if(dispatch_queue_get_label(queue) == dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL)) {
		block();
	} else {
		dispatch_sync(queue, block);
	}
}

- (void)beginProgress:(NSString *)localizedDescription {
	while(playbackController.progressOverall) {
		[[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.01]];
	}
	dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
		self->playbackController.progressOverall = [NSProgress progressWithTotalUnitCount:100000];
		self->playbackController.progressOverall.localizedDescription = localizedDescription;
	});
}

- (void)completeProgress {
	dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
		if(self->playbackController.progressJob) {
			[self->playbackController.progressJob setCompletedUnitCount:100000];
			self->playbackController.progressJob = nil;
		}
		[self->playbackController.progressOverall setCompletedUnitCount:100000];
		self->playbackController.progressOverall = nil;
	});
}

- (void)beginProgressJob:(NSString *)localizedDescription percentOfTotal:(double)percent {
	dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
		NSUInteger jobCount = (NSUInteger)ceil(1000.0 * percent);
		self->playbackController.progressJob = [NSProgress progressWithTotalUnitCount:100000];
		self->playbackController.progressJob.localizedDescription = localizedDescription;
		[self->playbackController.progressOverall addChild:self->playbackController.progressJob withPendingUnitCount:jobCount];
	});
}

- (void)completeProgressJob {
	dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
		[self->playbackController.progressJob setCompletedUnitCount:100000];
		self->playbackController.progressJob = nil;
	});
}

- (void)setProgressStatus:(double)status {
	if(status >= 0) {
		dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
			NSUInteger jobCount = (NSUInteger)ceil(1000.0 * status);
			[self->playbackController.progressOverall setCompletedUnitCount:jobCount];
		});
	} else {
		[self completeProgress];
	}
}

- (void)setProgressJobStatus:(double)status {
	if(status >= 0) {
		dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
			NSUInteger jobCount = (NSUInteger)ceil(1000.0 * status);
			[self->playbackController.progressJob setCompletedUnitCount:jobCount];
		});
	} else {
		[self completeProgressJob];
	}
}

+ (NSString *)keyForPath:(NSString *)path {
	return [path stringByReplacingOccurrencesOfString:@"." withString:@"%2E"];
}

- (NSArray *)insertURLs:(NSArray *)urls atIndex:(NSInteger)index sort:(BOOL)sort {
	__block NSMutableSet *uniqueURLs = [NSMutableSet set];

	__block NSMutableDictionary *expandedURLs = [[NSMutableDictionary alloc] init];
	__block NSMutableDictionary *loadedURLs = [[NSMutableDictionary alloc] init];
	__block NSMutableArray *fileURLs = [[NSMutableArray alloc] init];
	NSMutableArray *validURLs = [[NSMutableArray alloc] init];
	NSMutableArray *folderURLs = [[NSMutableArray alloc] init];
	NSMutableArray *dependencyURLs = [[NSMutableArray alloc] init];
	__block NSDictionary *xmlData = nil;

	BOOL addOtherFilesInFolder = [[NSUserDefaults standardUserDefaults] boolForKey:@"addOtherFilesInFolders"];

	double progress;

	if(!urls) {
		[self completeProgress];
		return @[];
	}

	[self beginProgress:NSLocalizedString(@"ProgressActionLoader", @"")];

	[self beginProgressJob:NSLocalizedString(@"ProgressSubActionLoaderListingFiles", @"") percentOfTotal:20.0];

	if(index < 0)
		index = 0;

	progress = 0.0;

	double progressstep = [urls count] ? 100.0 / (double)([urls count]) : 0;

	id<SentrySpan> mainTask = [SentrySDK startTransactionWithName:@"Loading playlist entries" operation:@"Main task"];

	id<SentrySpan> sandboxTask = [mainTask startChildWithOperation:@"Initial Sandbox sweep" description:@"Attempt load the files into the Sandbox storage, or locate them if they're already in storage"];

	NSURL *url;
	for(url in urls) {
		id<SentrySpan> pathTask = [sandboxTask startChildWithOperation:@"Process one folder" description:[NSString stringWithFormat:@"Processing file or folder: %@", url]];
		@try {
			if(!url) continue;
			if([url isFileURL]) {
				if(![url path]) continue;
				BOOL isDir;
				if([[NSFileManager defaultManager] fileExistsAtPath:[url path] isDirectory:&isDir]) {
					if(isDir == YES) {
						// Get subpaths
						[[SandboxBroker sharedSandboxBroker] addFolderIfMissing:url];
						NSArray *pathURLs = [self fileURLsAtPath:[url path]];
						for(NSURL *url in pathURLs) {
							[expandedURLs setValue:url forKey:[PlaylistLoader keyForPath:[url absoluteString]]];
						}
					} else if(addOtherFilesInFolder) {
						NSURL *folderUrl = [url URLByDeletingLastPathComponent];
						if(![folderURLs containsObject:folderUrl]) {
							[[SandboxBroker sharedSandboxBroker] requestFolderForFile:url];
							NSArray *pathURLs = [self fileURLsAtPath:[folderUrl path]];
							for(NSURL *url in pathURLs) {
								[expandedURLs setValue:url forKey:[PlaylistLoader keyForPath:[url absoluteString]]];
							}
							[folderURLs addObject:folderUrl];
						}
					} else {
						[[SandboxBroker sharedSandboxBroker] addFileIfMissing:url];
						[expandedURLs setValue:url forKey:[PlaylistLoader keyForPath:[url absoluteString]]];
					}
				}
			} else {
				// Non-file URL..
				if(![url absoluteString]) continue;
				[expandedURLs setValue:url forKey:[PlaylistLoader keyForPath:[url absoluteString]]];
			}
			
			[pathTask finish];
		}
		@catch(NSException *e) {
			DLog(@"Exception caught while processing path: %@", e);
			if(e) {
				[SentrySDK captureException:e];
			} else {
				[SentrySDK captureMessage:[NSString stringWithFormat:@"Null exception when processing path: %@", url]];
			}
			[pathTask finishWithStatus:kSentrySpanStatusInternalError];
		}

		progress += progressstep;

		[self setProgressJobStatus:progress];
	}
	
	[sandboxTask finish];

	[self completeProgressJob];

	progress = 0.0;

	DLog(@"Expanded urls: %@", expandedURLs);

	[self beginProgressJob:NSLocalizedString(@"ProgressSubActionLoaderFilteringContainerFiles", @"") percentOfTotal:20.0];

	progressstep = [expandedURLs count] ? 100.0 / (double)([expandedURLs count]) : 0;

	if([expandedURLs count]) {
		__block id<SentrySpan> containerTask = [mainTask startChildWithOperation:@"Process paths for containers"];

		__block NSLock *lock = [[NSLock alloc] init];
		
		__block NSArray *acceptableContainerTypes = [self acceptableContainerTypes];
		
		__block double weakProgress = progress;
		__block double weakProgressstep = progressstep;

		// Container vs non-container url
		[expandedURLs enumerateKeysAndObjectsUsingBlock:^(id  _Nonnull key, id  _Nonnull obj, BOOL * _Nonnull stop) {
			NSBlockOperation *op = [[NSBlockOperation alloc] init];
			
			[op addExecutionBlock:^{
				id<SentrySpan> pathTask = nil;
				id<SentrySpan> innerTask = nil;
				NSURL *url = nil;
				@try {
					url = obj;

					if(containerTask) {
						pathTask = [containerTask startChildWithOperation:@"Process path as container" description:[NSString stringWithFormat:@"Checking if file is container: %@", url]];
					}
					[lock lock];
					if([uniqueURLs containsObject:url]) {
						[lock unlock];
						return;
					}
					[lock unlock];

					if([acceptableContainerTypes containsObject:[[url pathExtension] lowercaseString]]) {
						if(pathTask) {
							innerTask = [pathTask startChildWithOperation:@"Container, processing"];
						}

						NSArray *urls = [AudioContainer urlsForContainerURL:url];

						if(urls != nil && [urls count] != 0) {
							[lock lock];
							[loadedURLs setValue:urls forKey:key];
							[lock unlock];

							// Make sure the container isn't added twice.
							[lock lock];
							[uniqueURLs addObject:url];
							[lock unlock];

							// Find the dependencies
							NSArray *depURLs = [AudioContainer dependencyUrlsForContainerURL:url];

							BOOL localFound = NO;
							for(NSURL *u in urls) {
								if([u isFileURL]) {
									localFound = YES;
									break;
								}
							}
							if(depURLs) {
								[lock lock];
								[dependencyURLs addObjectsFromArray:depURLs];
								[lock unlock];

								for(NSURL *u in depURLs) {
									if([u isFileURL]) {
										localFound = YES;
										break;
									}
								}
							}
							if(localFound) {
								[[SandboxBroker sharedSandboxBroker] requestFolderForFile:url];
							}
						} else {
							/* Fall back on adding the raw file if all container parsers have failed. */
							[lock lock];
							[loadedURLs setValue:url forKey:key];
							[lock unlock];
						}
						if(innerTask) {
							[innerTask finish];
							innerTask = nil;
						}
					} else if([[[url pathExtension] lowercaseString] isEqualToString:@"xml"]) {
						[lock lock];
						xmlData = [XmlContainer entriesForContainerURL:url];
						[lock unlock];
					} else {
						[lock lock];
						[loadedURLs setValue:url forKey:key];
						[lock unlock];
					}
					if(pathTask) {
						[pathTask finish];
						pathTask = nil;
					}
				}
				@catch(NSException *e) {
					DLog(@"Exception caught while processing for containers: %@", e);
					if(e) {
						[SentrySDK captureException:e];
					} else {
						[SentrySDK captureMessage:[NSString stringWithFormat:@"Null exception caught while processing containers for URL: %@", url]];
					}
					if(innerTask) {
						[innerTask finishWithStatus:kSentrySpanStatusInternalError];
					}
					if(pathTask) {
						[pathTask finishWithStatus:kSentrySpanStatusInternalError];
					}
				}

				[lock lock];
				weakProgress += weakProgressstep;
				[self setProgressJobStatus:weakProgress];
				[lock unlock];
			}];

			[self->containerQueue addOperation:op];
		}];

		[containerQueue waitUntilAllOperationsAreFinished];

		progress = weakProgress;
		[self setProgressJobStatus:progress];

		[containerTask finish];
	}

	progress = 0.0;
	[self completeProgressJob];

	if([loadedURLs count] > 0) {
		[self beginProgressJob:NSLocalizedString(@"ProgressSubActionLoaderFilteringFiles", @"") percentOfTotal:20.0];
	} else {
		[self setProgressStatus:60.0];
	}

	NSArray *fileTypes = [AudioPlayer fileTypes];

	id<SentrySpan> filterTask = [mainTask startChildWithOperation:@"Filtering URLs for dupes and supported tracks"];

	NSArray *keys = [loadedURLs allKeys];
	if(sort) {
		keys = [keys sortedArrayUsingSelector:@selector(finderCompare:)];
	}
	NSArray *objs = [loadedURLs objectsForKeys:keys notFoundMarker:[NSNull null]];
	/* Pass 1: Collect unique URLs
	 * v2: from containers only
	 */
	for(id obj in objs) {
		/*if([obj isKindOfClass:[NSURL class]]) {
		} else*/
		if([obj isKindOfClass:[NSArray class]]) {
			for(NSURL *url in obj) {
				if(![uniqueURLs containsObject:url]) {
					[uniqueURLs addObject:url];
				}
			}
		}
	}

	/* Pass 2: Only add outer URLs that are unique, but add all contained URLs
	 * v2: only add outer URLs to unique list here, otherwise they don't get added at all :D
	 * Technically doing it for outer paths here isn't necessary, as the expanded URLs
	 * dictionary will end up deduplicating input paths anyway. We just don't want it
	 * happening to playlist or container contents
	 */
	for(id obj in objs) {
		if([obj isKindOfClass:[NSURL class]]) {
			if(![uniqueURLs containsObject:obj]) {
				[fileURLs addObject:obj];
				[uniqueURLs addObject:obj];
			}
		} else if([obj isKindOfClass:[NSArray class]]) {
			[fileURLs addObjectsFromArray:obj];
		}
	}

	DLog(@"File urls: %@", fileURLs);

	progressstep = [fileURLs count] ? 100.0 / (double)([fileURLs count]) : 0;

	for(url in fileURLs) {
		id<SentrySpan> fileTask = nil;

		@try {
			fileTask = [filterTask startChildWithOperation:@"Filtering individual path" description:[NSString stringWithFormat:@"File path: %@", url]];
			
			progress += progressstep;
			
			if(![[AudioPlayer schemes] containsObject:[url scheme]])
				continue;
			
			NSString *ext = [[url pathExtension] lowercaseString];
			
			// Need a better way to determine acceptable file types than basing it on extensions.
			if([url isFileURL] && ![fileTypes containsObject:ext])
				continue;
			
			[validURLs addObject:url];

			[fileTask finish];
		}
		@catch(NSException *e) {
			DLog(@"Exception caught while filtering paths: %@", e);
			if(e) {
				[SentrySDK captureException:e];
			} else {
				[SentrySDK captureMessage:[NSString stringWithFormat:@"Null exception caught when filtering paths for URL: %@", url]];
			}
			if(fileTask) {
				[fileTask finishWithStatus:kSentrySpanStatusInternalError];
			}
		}

		[self setProgressJobStatus:progress];
	}

	[filterTask finish];

	progress = 0.0;

	if([fileURLs count] > 0) {
		[self completeProgressJob];
	}
	
	DLog(@"Valid urls: %@", validURLs);

	// Create actual entries
	int count = (int)[validURLs count];
	if(xmlData) count += [[xmlData objectForKey:@"entries"] count];

	// no valid URLs, or they use an unsupported URL scheme
	if(!count) {
		[self completeProgress];
		return @[];
	}

	[self beginProgressJob:NSLocalizedString(@"ProgressSubActionLoaderAddingEntries", @"") percentOfTotal:20.0];

	progressstep = 100.0 / (double)(count);

	__block id<SentrySpan> addTask = [mainTask startChildWithOperation:@"Add entries to playlist" description:[NSString stringWithFormat:@"Adding %lu entries to the playlist", [validURLs count]]];

	NSInteger i = 0;
	__block NSMutableArray *entries = [NSMutableArray arrayWithCapacity:count];
	for(NSURL *url in validURLs) {
		__block PlaylistEntry *pe;

		dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
			id<SentrySpan> addItemTask = nil;
			if(addTask) {
				addItemTask = [addTask startChildWithOperation:@"Add individual item on main queue" description:[NSString stringWithFormat:@"Track URL: %@", url]];
			}
			pe = [NSEntityDescription insertNewObjectForEntityForName:@"PlaylistEntry" inManagedObjectContext:self->playlistController.persistentContainer.viewContext];
			pe.url = url;
			pe.index = index + i;
			pe.rawTitle = [[url path] lastPathComponent];
			pe.queuePosition = -1;
			[addItemTask finish];
		});

		[entries addObject:pe];

		++i;

		progress += progressstep;
		[self setProgressJobStatus:progress];
	}

	[addTask finish];

	NSInteger j = index + i;

	if(xmlData) {
		for(NSDictionary *entry in [xmlData objectForKey:@"entries"]) {
			__block PlaylistEntry *pe;

			dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
				pe = [NSEntityDescription insertNewObjectForEntityForName:@"PlaylistEntry" inManagedObjectContext:self->playlistController.persistentContainer.viewContext];
				[pe setValuesForKeysWithDictionary:entry];
				pe.index = index + i;
				pe.queuePosition = -1;
			});

			[entries addObject:pe];

			++i;
		}
	}

	[self completeProgress];

	NSIndexSet *is = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(index, [entries count])];

	dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
		[self->playlistController insertObjects:entries atArrangedObjectIndexes:is];
	});

	if(xmlData && [[xmlData objectForKey:@"queue"] count]) {
		dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
			[self->playlistController emptyQueueList:self];

			long i = 0;
			for(NSNumber *index in [xmlData objectForKey:@"queue"]) {
				NSInteger indexVal = [index intValue] + j;
				PlaylistEntry *pe = [entries objectAtIndex:indexVal];
				pe.queuePosition = i;
				pe.queued = YES;

				[[self->playlistController queueList] addObject:pe];

				++i;
			}
		});
	}
	
	[mainTask finish];

	// Clear the selection
	dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
		[self->playlistController setSelectionIndexes:[NSIndexSet indexSet]];
	});

	{
		NSArray *arrayFirst = @[[entries objectAtIndex:0]];
		NSMutableArray *arrayRest = [entries mutableCopy];
		[arrayRest removeObjectAtIndex:0];

		metadataLoadInProgress = YES;

		[self beginProgress:NSLocalizedString(@"ProgressActionLoadingMetadata", @"")];
		[self beginProgressJob:NSLocalizedString(@"ProgressSubActionLoadingMetadata", @"") percentOfTotal:50.0];

		[self performSelectorOnMainThread:@selector(syncLoadInfoForEntries:) withObject:arrayFirst waitUntilDone:YES];

		progressstep = 100.0 / (double)([entries count]);
		progress = progressstep;
		[self setProgressJobStatus:progress];

		if([arrayRest count])
			[self performSelectorInBackground:@selector(loadInfoForEntries:) withObject:arrayRest];
		else {
			dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
				[self->playlistController commitPersistentStore];
			});
			[self completeProgress];
		}
		return entries;
	}
}

NSURL *_Nullable urlForPath(NSString *_Nullable path);

- (void)loadInfoForEntries:(NSArray *)entries {
	NSMutableDictionary *queueThisJob = [[NSMutableDictionary alloc] init];
	for(PlaylistEntry *pe in entries) {
		if(!pe || !pe.urlString || ![pe.urlString length] || pe.deLeted || pe.metadataLoaded) continue;

		NSString *path = pe.urlString;
		NSMutableArray *entrySet = [queueThisJob objectForKey:path];
		if(!entrySet) {
			entrySet = [[NSMutableArray alloc] init];
			[entrySet addObject:pe];
			[queueThisJob setObject:entrySet forKey:path];
		} else {
			[entrySet addObject:pe];
		}
	}

	@synchronized(queuedURLs) {
		if([queuedURLs count]) {
			for(NSString *key in [queueThisJob allKeys]) {
				if([queuedURLs objectForKey:key]) {
					[queueThisJob removeObjectForKey:key];
				}
			}
		}
	}

	if(![queueThisJob count]) {
		size_t count;
		@synchronized (queuedURLs) {
			count = [queuedURLs count];
		}
		if(!count) {
			[playlistController performSelectorOnMainThread:@selector(updateTotalTime) withObject:nil waitUntilDone:NO];
			[self completeProgress];
			metadataLoadInProgress = NO;
		}
		return;
	}

	size_t count = 0;
	do {
		@synchronized(queuedURLs) {
			count = [queuedURLs count];
		}
		if(count) usleep(5000);
	} while(count > 0);

	@synchronized(queuedURLs) {
		[queuedURLs addEntriesFromDictionary:queueThisJob];
	}

	NSMutableIndexSet *update_indexes = [[NSMutableIndexSet alloc] init];

	__block double progress = 0.0;

	double progressstep;

	if(metadataLoadInProgress) {
		progressstep = 100.0 / (double)([queueThisJob count] + 1);
		progress = progressstep;
	} else {
		[self beginProgress:NSLocalizedString(@"ProgressActionLoadingMetadata", @"")];
		[self beginProgressJob:NSLocalizedString(@"ProgressSubActionLoadingMetadata", @"") percentOfTotal:50.0];

		progressstep = 100.0 / (double)([entries count]);
		progress = 0.0;
	}

	NSLock *outLock = [[NSLock alloc] init];
	NSMutableArray *outArray = [[NSMutableArray alloc] init];
	id dataStoreClass = NSClassFromString(@"RedundantPlaylistDataStore"); // CogAudio
	RedundantPlaylistDataStore *dataStore = [[dataStoreClass alloc] init];

	__block NSLock *weakLock = outLock;
	__block NSMutableArray *weakArray = outArray;
	__block RedundantPlaylistDataStore *weakDataStore = dataStore;

	__block NSMutableDictionary *uniquePathsEntries = [[NSMutableDictionary alloc] init];

	__block id<SentrySpan> mainTask = [SentrySDK startTransactionWithName:@"Loading tags" operation:@"Main tag operation"];

	{
		__block NSLock *blockLock = [[NSLock alloc] init];
		__block NSMutableArray *blockInputs = [[queueThisJob allKeys] mutableCopy];
		for(size_t i = 0, j = [blockInputs count]; i < j; ++i) {
			NSBlockOperation *op = [[NSBlockOperation alloc] init];
			[op addExecutionBlock:^{
				@autoreleasepool {
					[blockLock lock];
					NSString *key = [blockInputs objectAtIndex:0];
					[blockInputs removeObjectAtIndex:0];
					[blockLock unlock];

					NSURL *url = urlForPath(key);

					NSString *message = [NSString stringWithFormat:@"Loading metadata for %@", url];
					DLog(@"%@", message);
					id<SentrySpan> childTask = nil;
					if(mainTask) {
						childTask = [mainTask startChildWithOperation:@"Load single tag" description:message];
					}

					@try {
						NSDictionary *entryProperties = [AudioPropertiesReader propertiesForURL:url];
						if(entryProperties == nil)
							return;
						
						NSDictionary *entryMetadata = [AudioMetadataReader metadataForURL:url];
						
						NSDictionary *entryInfo = [NSDictionary dictionaryByMerging:entryProperties with:entryMetadata];
						
						[weakLock lock];
						@autoreleasepool {
							entryInfo = [weakDataStore coalesceEntryInfo:entryInfo];
						}
						[weakArray addObject:key];
						[weakArray addObject:entryInfo];
						[uniquePathsEntries setObject:[[NSMutableArray alloc] init] forKey:key];
						progress += progressstep;
						[self setProgressJobStatus:progress];
						[weakLock unlock];

						if(childTask) {
							[childTask finish];
						}
					}
					@catch(NSException *e) {
						DLog(@"Exception thrown while reading tags: %@", e);
						if(e) {
							[SentrySDK captureException:e];
						} else {
							[SentrySDK captureMessage:[NSString stringWithFormat:@"Null exception caught while reading tags for URL: %@", url]];
						}
						if(childTask) {
							[childTask finishWithStatus:kSentrySpanStatusInternalError];
						}
					}
				}
			}];

			[queue addOperation:op];
		}
	}

	[queue waitUntilAllOperationsAreFinished];

	progress = 0.0;
	[self completeProgressJob];

	[self beginProgressJob:NSLocalizedString(@"ProgressSubActionMetadataApply", @"") percentOfTotal:50.0];

	id<SentrySpan> finalTask = [mainTask startChildWithOperation:@"Apply tags to storage"];

	progressstep = 200.0 / (double)([outArray count]);

	NSManagedObjectContext *moc = playlistController.persistentContainer.viewContext;

	NSPredicate *hasUrlPredicate = [NSPredicate predicateWithFormat:@"urlString != nil && urlString != %@", @""];
	NSPredicate *deletedPredicate = [NSPredicate predicateWithFormat:@"deLeted == NO || deLeted == nil"];

	NSCompoundPredicate *predicate = [NSCompoundPredicate andPredicateWithSubpredicates:@[deletedPredicate, hasUrlPredicate]];

	[moc performBlockAndWait:^{
		NSFetchRequest *request = [NSFetchRequest fetchRequestWithEntityName:@"PlaylistEntry"];
		request.predicate = predicate;

		NSError *error;
		NSArray *results = [moc executeFetchRequest:request error:&error];

		if(results && [results count] > 0) {
			for(PlaylistEntry *pe in results) {
				NSMutableArray *entrySet = [uniquePathsEntries objectForKey:pe.urlString];
				if(entrySet) {
					[entrySet addObject:pe];
				}
			}
		}
	}];

	for(size_t i = 0, j = [outArray count]; i < j; i += 2) {
		__block NSString *entryKey = [outArray objectAtIndex:i];
		__block NSDictionary *entryInfo = [outArray objectAtIndex:i + 1];
		__block NSMutableIndexSet *weakUpdateIndexes = update_indexes;
		PlaylistController *playlistController = self->playlistController;
		dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
			NSArray *entrySet = [uniquePathsEntries objectForKey:entryKey];
			if(entrySet) {
				if([entrySet count] > 0) {
					[playlistController firstSawTrack:entrySet[0]];
				}
				for(PlaylistEntry *pe in entrySet) {
					[pe setMetadata:entryInfo];
					if(pe.index >= 0 && pe.index < NSNotFound) {
						[weakUpdateIndexes addIndex:pe.index];
					}
				}
			}

			progress += progressstep;
			[self setProgressJobStatus:progress];
		});
	}

	dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
		[self->playlistController commitPersistentStore];
	});

	[playlistController performSelectorOnMainThread:@selector(updateTotalTime) withObject:nil waitUntilDone:NO];

	{
		__block NSScrollView *weakPlaylistView = playlistView;
		__block NSIndexSet *weakIndexSet = update_indexes;
		dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
			unsigned long columns = [[[weakPlaylistView documentView] tableColumns] count];
			[weakPlaylistView.documentView reloadDataForRowIndexes:weakIndexSet columnIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, columns)]];
		});
	}

	@synchronized(queuedURLs) {
		[queuedURLs removeObjectsForKeys:[queueThisJob allKeys]];
	}

	[self completeProgress];
	metadataLoadInProgress = NO;
	
	[finalTask finish];
	[mainTask finish];
}

// To be called on main thread only
- (void)syncLoadInfoForEntries:(NSArray *)entries {
	NSMutableIndexSet *update_indexes = [[NSMutableIndexSet alloc] init];
	long i, j;
	NSMutableIndexSet *load_info_indexes = [[NSMutableIndexSet alloc] init];

	i = 0;
	j = 0;
	for(PlaylistEntry *pe in entries) {
		long idx = j++;

		if([pe metadataLoaded]) continue;

		[update_indexes addIndex:pe.index];
		[load_info_indexes addIndex:idx];

		++i;
	}

	if(!i) {
		[self->playlistController updateTotalTime];
		return;
	}
	
	__block id<SentrySpan> mainTask = [SentrySDK startTransactionWithName:@"Load tags synchronously" operation:@"Main task"];

	[load_info_indexes enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL *_Nonnull stop) {
		PlaylistEntry *pe = [entries objectAtIndex:idx];

		DLog(@"Loading metadata for %@", pe.url);
		id<SentrySpan> childTask = nil;
		if(mainTask) {
			childTask = [mainTask startChildWithOperation:@"Load single tag" description:[NSString stringWithFormat:@"Loading tag for: %@", pe.urlString]];
		}

		@try {
			NSDictionary *entryProperties = [AudioPropertiesReader propertiesForURL:pe.url];
			if(entryProperties == nil)
				return;
			
			NSDictionary *entryInfo = [NSDictionary dictionaryByMerging:entryProperties with:[AudioMetadataReader metadataForURL:pe.url]];
			
			[pe setMetadata:entryInfo];
			[playlistController firstSawTrack:pe];
			
			if(childTask) {
				[childTask finish];
			}
		}
		@catch(NSException *e) {
			DLog(@"Exception thrown while reading tag synchronously: %@", e);
			if(e) {
				[SentrySDK captureException:e];
			} else {
				[SentrySDK captureMessage:[NSString stringWithFormat:@"Null exception caught while reading tags for URL: %@", pe.url]];
			}
			if(childTask) {
				[childTask finishWithStatus:kSentrySpanStatusInternalError];
			}
		}
	}];
	
	[mainTask finish];

	[self->playlistController updateTotalTime];

	{
		unsigned long columns = [[[self->playlistView documentView] tableColumns] count];
		[self->playlistView.documentView reloadDataForRowIndexes:update_indexes columnIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, columns)]];
	}
}

- (void)clear:(id)sender {
	[playlistController clear:sender];
}

- (NSArray *)addURLs:(NSArray *)urls sort:(BOOL)sort {
	return [self insertURLs:urls atIndex:(int)[[playlistController content] count] sort:sort];
}

- (NSArray *)addURL:(NSURL *)url {
	if(!url) return [NSArray array];
	return [self insertURLs:@[url] atIndex:(int)[[playlistController content] count] sort:NO];
}

- (BOOL)addDataStore {
	BOOL dataMigrated = [[NSUserDefaults standardUserDefaults] boolForKey:@"metadataMigrated"];

	NSPersistentContainer *pc = playlistController.persistentContainer;
	if(pc) {
		NSManagedObjectContext *moc = pc.viewContext;

		NSFetchRequest *request = [NSFetchRequest fetchRequestWithEntityName:@"AlbumArtwork"];
		NSError *error = nil;
		NSArray *results = [moc executeFetchRequest:request error:&error];
		if(!results) {
			ALog(@"Error fetching AlbumArtwork objects: %@\n%@", [error localizedDescription], [error userInfo]);
			abort();
		}

		for(AlbumArtwork *art in results) {
			[kArtworkDictionary setObject:art forKey:art.artHash];
		}

		request = [NSFetchRequest fetchRequestWithEntityName:@"PlaylistEntry"];

		NSSortDescriptor *sortDescriptor = [[NSSortDescriptor alloc] initWithKey:@"index" ascending:YES];

		request.sortDescriptors = @[sortDescriptor];

		results = [moc executeFetchRequest:request error:&error];
		if(!results) {
			ALog(@"Error fetching PlaylistEntry objects: %@\n%@", [error localizedDescription], [error userInfo]);
			abort();
		}

		if([results count] == 0) {
			return NO;
		}

		NSMutableArray *resultsCopy = [results mutableCopy];

		NSMutableIndexSet *pruneSet = [[NSMutableIndexSet alloc] init];
		NSUInteger index = 0;
		for(PlaylistEntry *pe in resultsCopy) {
			if(pe.deLeted || !pe.urlString || ![pe.urlString length]) {
				[pruneSet addIndex:index];
				[moc deleteObject:pe];
			}
			++index;
		}
		[resultsCopy removeObjectsAtIndexes:pruneSet];

		if(!dataMigrated) {
			for(PlaylistEntry *pe in resultsCopy) {
				pe.metadataLoaded = NO;
			}
		}

		if([pruneSet count] || !dataMigrated) {
			[playlistController commitPersistentStore];
		}

		results = [NSArray arrayWithArray:resultsCopy];

		{
			NSIndexSet *is = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [results count])];

			[playlistController insertObjectsUnsynced:results atArrangedObjectIndexes:is];
		}

		[playlistController readQueueFromDataStore];
		[playlistController readShuffleListFromDataStore];

		if(!dataMigrated && [results count]) {
			[self performSelectorInBackground:@selector(loadInfoForEntries:) withObject:results];
		}

		[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"metadataMigrated"];

		return YES;
	}
	return NO;
}

- (NSArray *)addDatabase {
	SQLiteStore *store = [SQLiteStore sharedStore];

	int64_t count = [store playlistGetCountCached];

	NSInteger i = 0;
	NSMutableArray *entries = [NSMutableArray arrayWithCapacity:count];

	for(i = 0; i < count; ++i) {
		PlaylistEntry *pe = [store playlistGetCachedItem:i];

		pe.queuePosition = -1;

		[entries addObject:pe];
	}

	NSIndexSet *is = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, [entries count])];

	[playlistController insertObjectsUnsynced:entries atArrangedObjectIndexes:is];

	count = [store queueGetCount];

	if(count) {
		NSMutableIndexSet *refreshSet = [[NSMutableIndexSet alloc] init];

		[playlistController emptyQueueListUnsynced];

		for(i = 0; i < count; ++i) {
			NSInteger indexVal = [store queueGetEntry:i];
			PlaylistEntry *pe = [entries objectAtIndex:indexVal];
			pe.queuePosition = i;
			pe.queued = YES;

			[[playlistController queueList] addObject:pe];

			[refreshSet addIndex:[pe index]];
		}

		// Refresh entire row to refresh tooltips
		unsigned long columns = [[playlistView.documentView tableColumns] count];
		[playlistView.documentView reloadDataForRowIndexes:refreshSet columnIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, columns)]];
	}

	// Clear the selection
	[playlistController setSelectionIndexes:[NSIndexSet indexSet]];

	if([entries count]) {
		[self performSelectorInBackground:@selector(loadInfoForEntries:) withObject:entries];
	}

	return entries;
}

- (NSArray *)acceptableFileTypes {
	return [[self acceptableContainerTypes] arrayByAddingObjectsFromArray:[AudioPlayer fileTypes]];
}

- (NSArray *)acceptablePlaylistTypes {
	return @[@"m3u", @"pls"];
}

- (NSArray *)acceptableContainerTypes {
	return [AudioPlayer containerTypes];
}

- (void)willInsertURLs:(NSArray *)urls origin:(URLOrigin)origin {
	[playlistController willInsertURLs:urls origin:origin];
}
- (void)didInsertURLs:(NSArray *)urls origin:(URLOrigin)origin {
	[playlistController didInsertURLs:urls origin:origin];
}

- (void)addURLsInBackground:(NSDictionary *)input {
	[playlistController addURLsInBackground:input];
}

- (void)insertURLsInBackground:(NSDictionary *)input {
	[playlistController insertURLsInBackground:input];
}

@end
