//
//  PlaylistLoader.m
//  Cog
//
//  Created by Vincent Spader on 3/05/07.
//  Copyright 2007 Vincent Spader All rights reserved.
//

#include <objc/runtime.h>

#include <mach/semaphore.h>

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

@implementation PlaylistLoader

- (id)init {
	self = [super init];
	if(self) {
		[self initDefaults];

		queue = [[NSOperationQueue alloc] init];
		[queue setMaxConcurrentOperationCount:8];
	}

	return self;
}

- (void)initDefaults {
	NSDictionary *defaultsDictionary = @{@"readCueSheetsInFolders": [NSNumber numberWithBool:YES]};

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
		NSString *path = [self relativePathFrom:filename toURL:[pe URL]];
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
		NSString *path = [self relativePathFrom:filename toURL:[pe URL]];
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

		NSString *path = [self relativePathFrom:filename toURL:[pe URL]];

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
		[queueList addObject:[NSNumber numberWithInteger:pe.index]];
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

	NSArray *subpaths = [manager subpathsAtPath:path];

	for(NSString *subpath in subpaths) {
		NSString *absoluteSubpath = [NSString pathWithComponents:@[path, subpath]];

		BOOL isDir;
		if([manager fileExistsAtPath:absoluteSubpath isDirectory:&isDir] && isDir == NO) {
			if([[absoluteSubpath pathExtension] caseInsensitiveCompare:@"cue"] != NSOrderedSame ||
			   [[NSUserDefaults standardUserDefaults] boolForKey:@"readCueSheetsInFolders"]) {
				[urls addObject:[NSURL fileURLWithPath:absoluteSubpath]];
			}
		}
	}

	NSSortDescriptor *sd_path = [[NSSortDescriptor alloc] initWithKey:@"path" ascending:YES];
	[urls sortUsingDescriptors:@[sd_path]];

	return urls;
}

static inline void dispatch_sync_reentrant(dispatch_queue_t queue, dispatch_block_t block) {
	if(dispatch_queue_get_label(queue) == dispatch_queue_get_label(DISPATCH_CURRENT_QUEUE_LABEL)) {
		block();
	} else {
		dispatch_sync(queue, block);
	}
}

- (void)setProgressBarStatus:(double)status {
	dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
		[self->playbackController setProgressBarStatus:status];
	});
}

- (NSArray *)insertURLs:(NSArray *)urls atIndex:(NSInteger)index sort:(BOOL)sort {
	NSMutableSet *uniqueURLs = [NSMutableSet set];

	NSMutableArray *expandedURLs = [NSMutableArray array];
	NSMutableArray *containedURLs = [NSMutableArray array];
	NSMutableArray *fileURLs = [NSMutableArray array];
	NSMutableArray *validURLs = [NSMutableArray array];
	NSDictionary *xmlData = nil;

	double progress = 0.0;

	if(!urls) {
		[self setProgressBarStatus:-1];
		return @[];
	}

	if(index < 0)
		index = 0;

	[self setProgressBarStatus:progress];

	double progressstep = [urls count] ? 20.0 / (double)([urls count]) : 0;

	NSURL *url;
	for(url in urls) {
		if([url isFileURL]) {
			BOOL isDir;
			if([[NSFileManager defaultManager] fileExistsAtPath:[url path] isDirectory:&isDir]) {
				if(isDir == YES) {
					// Get subpaths
					[expandedURLs addObjectsFromArray:[self fileURLsAtPath:[url path]]];
				} else {
					[expandedURLs addObject:[NSURL fileURLWithPath:[url path]]];
				}
			}
		} else {
			// Non-file URL..
			[expandedURLs addObject:url];
		}

		progress += progressstep;

		[self setProgressBarStatus:progress];
	}

	progress = 20.0;

	[self setProgressBarStatus:progress];

	DLog(@"Expanded urls: %@", expandedURLs);

	NSArray *sortedURLs;
	if(sort == YES) {
		sortedURLs = [expandedURLs sortedArrayUsingSelector:@selector(finderCompare:)];
		//		sortedURLs = [expandedURLs sortedArrayUsingSelector:@selector(compareTrackNumbers:)];
	} else {
		sortedURLs = expandedURLs;
	}

	progressstep = [sortedURLs count] ? 20.0 / (double)([sortedURLs count]) : 0;

	for(url in sortedURLs) {
		// Container vs non-container url
		if([[self acceptableContainerTypes] containsObject:[[url pathExtension] lowercaseString]]) {
			NSArray *urls = [AudioContainer urlsForContainerURL:url];

			if(urls != nil && [urls count] != 0) {
				[containedURLs addObjectsFromArray:urls];

				// Make sure the container isn't added twice.
				[uniqueURLs addObject:url];
			} else {
				/* Fall back on adding the raw file if all container parsers have failed. */
				[fileURLs addObject:url];
			}
		} else if([[[url pathExtension] lowercaseString] isEqualToString:@"xml"]) {
			xmlData = [XmlContainer entriesForContainerURL:url];
		} else {
			[fileURLs addObject:url];
		}

		progress += progressstep;
		[self setProgressBarStatus:progress];
	}

	progress = 40.0;
	[self setProgressBarStatus:progress];

	DLog(@"File urls: %@", fileURLs);

	DLog(@"Contained urls: %@", containedURLs);

	progressstep = [fileURLs count] ? 20.0 / (double)([fileURLs count]) : 0;

	for(url in fileURLs) {
		progress += progressstep;

		if(![[AudioPlayer schemes] containsObject:[url scheme]])
			continue;

		NSString *ext = [[url pathExtension] lowercaseString];

		// Need a better way to determine acceptable file types than basing it on extensions.
		if([url isFileURL] && ![[AudioPlayer fileTypes] containsObject:ext])
			continue;

		if(![uniqueURLs containsObject:url]) {
			[validURLs addObject:url];

			[uniqueURLs addObject:url];
		}

		[self setProgressBarStatus:progress];
	}

	progress = 60.0;

	[self setProgressBarStatus:progress];

	DLog(@"Valid urls: %@", validURLs);

	progressstep = [containedURLs count] ? 20.0 / (double)([containedURLs count]) : 0;

	for(url in containedURLs) {
		progress += progressstep;

		if(![[AudioPlayer schemes] containsObject:[url scheme]])
			continue;

		// Need a better way to determine acceptable file types than basing it on extensions.
		if([url isFileURL] && ![[AudioPlayer fileTypes] containsObject:[[url pathExtension] lowercaseString]])
			continue;

		[validURLs addObject:url];

		[self setProgressBarStatus:progress];
	}

	progress = 80.0;

	[self setProgressBarStatus:progress];

	// Create actual entries
	int count = (int)[validURLs count];
	if(xmlData) count += [[xmlData objectForKey:@"entries"] count];

	// no valid URLs, or they use an unsupported URL scheme
	if(!count) {
		[self setProgressBarStatus:-1];
		return @[];
	}

	progressstep = 20.0 / (double)(count);

	NSInteger i = 0;
	NSMutableArray *entries = [NSMutableArray arrayWithCapacity:count];
	for(NSURL *url in validURLs) {
		PlaylistEntry *pe;
		pe = [[PlaylistEntry alloc] init];

		pe.URL = url;
		pe.index = index + i;
		pe.title = [[url path] lastPathComponent];
		pe.queuePosition = -1;
		[entries addObject:pe];

		++i;

		progress += progressstep;
		[self setProgressBarStatus:progress];
	}

	NSInteger j = index + i;

	if(xmlData) {
		for(NSDictionary *entry in [xmlData objectForKey:@"entries"]) {
			PlaylistEntry *pe;
			pe = [[PlaylistEntry alloc] init];

			[pe setValuesForKeysWithDictionary:entry];
			pe.index = index + i;
			pe.queuePosition = -1;
			[entries addObject:pe];

			++i;
		}
	}

	progress = 100.0;
	[self setProgressBarStatus:progress];

	NSIndexSet *is = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(index, [entries count])];

	[playlistController insertObjects:entries atArrangedObjectIndexes:is];

	if(xmlData && [[xmlData objectForKey:@"queue"] count]) {
		[playlistController emptyQueueList:self];

		i = 0;
		for(NSNumber *index in [xmlData objectForKey:@"queue"]) {
			NSInteger indexVal = [index intValue] + j;
			PlaylistEntry *pe = [entries objectAtIndex:indexVal];
			pe.queuePosition = i;
			pe.queued = YES;

			[[playlistController queueList] addObject:pe];

			++i;
		}
	}

	// Clear the selection
	[playlistController setSelectionIndexes:[NSIndexSet indexSet]];

	{
		NSArray *arrayFirst = @[[entries objectAtIndex:0]];
		NSMutableArray *arrayRest = [entries mutableCopy];
		[arrayRest removeObjectAtIndex:0];

		progress = 0.0;
		[self setProgressBarStatus:progress];

		[self performSelectorOnMainThread:@selector(syncLoadInfoForEntries:) withObject:arrayFirst waitUntilDone:YES];

		progressstep = 100.0 / (double)([entries count]);
		progress += progressstep;
		[self setProgressBarStatus:progress];

		if([arrayRest count])
			[self performSelectorInBackground:@selector(loadInfoForEntries:) withObject:arrayRest];
		else
			[self setProgressBarStatus:-1];
		return entries;
	}
}

- (void)loadInfoForEntries:(NSArray *)entries {
	NSMutableIndexSet *update_indexes = [[NSMutableIndexSet alloc] init];
	long i, j;
	NSMutableIndexSet *load_info_indexes = [[NSMutableIndexSet alloc] init];

	SQLiteStore *store = [SQLiteStore sharedStore];

	__block double progress = [playbackController progressBarStatus];

	if(progress < 0 || progress >= 100)
		progress = 0;

	double progressRemaining = 100.0 - progress;

	// 50% for properties reading, 50% for applying them to the main thread
	const double progressstep = [entries count] ? (progressRemaining / 2.0) / [entries count] : 0;

	progressRemaining = progress + (progressRemaining / 2.0);

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
		[playlistController performSelectorOnMainThread:@selector(updateTotalTime) withObject:nil waitUntilDone:NO];
		return;
	}

	NSLock *outLock = [[NSLock alloc] init];
	NSMutableArray *outArray = [[NSMutableArray alloc] init];

	__block NSLock *weakLock = outLock;
	__block NSMutableArray *weakArray = outArray;

	{
		[load_info_indexes enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL *_Nonnull stop) {
			__block PlaylistEntry *weakPe = [entries objectAtIndex:idx];

			NSBlockOperation *op = [[NSBlockOperation alloc] init];

			[op addExecutionBlock:^{
				[weakLock lock];
				progress += progressstep;
				[weakLock unlock];

				DLog(@"Loading metadata for %@", weakPe.URL);

				NSMutableDictionary *entryInfo = [NSMutableDictionary dictionaryWithCapacity:32];

				NSDictionary *entryProperties = [AudioPropertiesReader propertiesForURL:weakPe.URL];
				if(entryProperties == nil)
					return;

				[entryInfo addEntriesFromDictionary:entryProperties];
				[entryInfo addEntriesFromDictionary:[AudioMetadataReader metadataForURL:weakPe.URL]];

				[weakLock lock];
				[weakArray addObject:weakPe];
				[weakArray addObject:entryInfo];
				[self setProgressBarStatus:progress];
				[weakLock unlock];
			}];

			[queue addOperation:op];
		}];
	}

	[queue waitUntilAllOperationsAreFinished];

	progress = progressRemaining;
	[self setProgressBarStatus:progress];

	for(i = 0, j = [outArray count]; i < j; i += 2) {
		__block PlaylistEntry *weakPe = [outArray objectAtIndex:i];
		__block NSDictionary *entryInfo = [outArray objectAtIndex:i + 1];
		dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
			[weakPe setMetadata:entryInfo];
			[store trackUpdate:weakPe];
			progress += progressstep;
			[self setProgressBarStatus:progress];
		});
	}

	[playlistController performSelectorOnMainThread:@selector(updateTotalTime) withObject:nil waitUntilDone:NO];

	{
		__block NSScrollView *weakPlaylistView = playlistView;
		__block NSIndexSet *weakIndexSet = update_indexes;
		dispatch_sync_reentrant(dispatch_get_main_queue(), ^{
			unsigned long columns = [[[weakPlaylistView documentView] tableColumns] count];
			[weakPlaylistView.documentView reloadDataForRowIndexes:weakIndexSet columnIndexes:[NSIndexSet indexSetWithIndexesInRange:NSMakeRange(0, columns)]];
		});
	}

	[self setProgressBarStatus:-1];
}
// To be called on main thread only
- (void)syncLoadInfoForEntries:(NSArray *)entries {
	NSMutableIndexSet *update_indexes = [[NSMutableIndexSet alloc] init];
	long i, j;
	NSMutableIndexSet *load_info_indexes = [[NSMutableIndexSet alloc] init];

	SQLiteStore *store = [SQLiteStore sharedStore];

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

	[load_info_indexes enumerateIndexesUsingBlock:^(NSUInteger idx, BOOL *_Nonnull stop) {
		PlaylistEntry *pe = [entries objectAtIndex:idx];

		DLog(@"Loading metadata for %@", pe.URL);

		NSMutableDictionary *entryInfo = [NSMutableDictionary dictionaryWithCapacity:32];

		NSDictionary *entryProperties = [AudioPropertiesReader propertiesForURL:pe.URL];
		if(entryProperties == nil)
			return;

		[entryInfo addEntriesFromDictionary:entryProperties];
		[entryInfo addEntriesFromDictionary:[AudioMetadataReader metadataForURL:pe.URL]];

		[pe setMetadata:entryInfo];
		[store trackUpdate:pe];
	}];

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
	return [self insertURLs:@[url] atIndex:(int)[[playlistController content] count] sort:NO];
}

- (NSArray *)addDatabase {
	SQLiteStore *store = [SQLiteStore sharedStore];

	int64_t count = [store playlistGetCount];

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

@end
