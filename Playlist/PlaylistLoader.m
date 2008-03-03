//
//  PlaylistLoader.m
//  Cog
//
//  Created by Vincent Spader on 3/05/07.
//  Copyright 2007 Vincent Spader All rights reserved.
//

#import "PlaylistLoader.h"
#import "PlaylistController.h"
#import "PlaylistEntry.h"
#import "AppController.h"

#import "NSFileHandle+CreateFile.h"

#import "CogAudio/AudioPlayer.h"
#import "CogAudio/AudioContainer.h"
#import "CogAudio/AudioPropertiesReader.h"
#import "CogAudio/AudioMetadataReader.h"

@implementation PlaylistLoader

- (BOOL)save:(NSString *)filename
{
	NSString *ext = [filename pathExtension];
	if ([ext isEqualToString:@"pls"])
	{
		return [self save:filename asType:kPlaylistPls];
	}
	else
	{
		return [self save:filename asType:kPlaylistM3u];
	}
}	

- (BOOL)save:(NSString *)filename asType:(PlaylistType)type
{
	if (type == kPlaylistM3u)
	{
		return [self saveM3u:filename];
	}
	else if (type == kPlaylistPls)
	{
		return [self savePls:filename];
	}

	return NO;
}

- (NSString *)relativePathFrom:(NSString *)filename toURL:(NSURL *)entryURL
{
	NSString *basePath = [[[filename stringByStandardizingPath] stringByDeletingLastPathComponent] stringByAppendingString:@"/"];

	if ([entryURL isFileURL]) {
		//We want relative paths.
		NSMutableString *entryPath = [[[[entryURL path] stringByStandardizingPath] mutableCopy] autorelease];

		[entryPath replaceOccurrencesOfString:basePath withString:@"" options:(NSAnchoredSearch | NSLiteralSearch | NSCaseInsensitiveSearch) range:NSMakeRange(0, [entryPath length])];
		if ([entryURL fragment])
		{
			[entryPath appendString:@"#"];
			[entryPath appendString:[entryURL fragment]];
		}

		return entryPath;		
	}
	else {
		//Write [entryURL absoluteString] to file
		return [entryURL absoluteString];
	}
}

- (BOOL)saveM3u:(NSString *)filename
{
	NSFileHandle *fileHandle = [NSFileHandle fileHandleForWritingAtPath:filename createFile:YES];
	if (!fileHandle) {
		NSLog(@"Error saving m3u!");
		return NO;
	}
	[fileHandle truncateFileAtOffset:0];
	
	for (PlaylistEntry *pe in [playlistController content])
	{
		NSString *path = [self relativePathFrom:filename toURL:[pe URL]];
		[fileHandle writeData:[[path stringByAppendingString:@"\n"] dataUsingEncoding:NSUTF8StringEncoding]];
	}

	[fileHandle closeFile];

	return YES;
}

- (BOOL)savePls:(NSString *)filename
{
	NSFileHandle *fileHandle = [NSFileHandle fileHandleForWritingAtPath:filename createFile:YES];
	if (!fileHandle) {
		return NO;
	}
	[fileHandle truncateFileAtOffset:0];

	[fileHandle writeData:[[NSString stringWithFormat:@"[playlist]\nnumberOfEntries=%i\n\n",[[playlistController content] count]] dataUsingEncoding:NSUTF8StringEncoding]];

	int i = 1;
	for (PlaylistEntry *pe in [playlistController content])
	{
		NSString *path = [self relativePathFrom:filename toURL:[pe URL]];
		NSString *entry = [NSString stringWithFormat:@"File%i=%@\n",i,path];

		[fileHandle writeData:[entry dataUsingEncoding:NSUTF8StringEncoding]];
		i++;
	}

	[fileHandle writeData:[@"\nVERSION=2" dataUsingEncoding:NSUTF8StringEncoding]];
	[fileHandle closeFile];

	return YES;
}

- (NSArray *)fileURLsAtPath:(NSString *)path
{
	NSFileManager *manager = [NSFileManager defaultManager];
	
	NSMutableArray *urls = [NSMutableArray array];
		
	NSArray *subpaths = [manager subpathsAtPath:path];

	for (NSString *subpath in subpaths)
	{
		NSString *absoluteSubpath = [NSString pathWithComponents:[NSArray arrayWithObjects:path,subpath,nil]];
		
		BOOL isDir;
		if ( [manager fileExistsAtPath:absoluteSubpath isDirectory:&isDir] && isDir == NO)
		{
			[urls addObject:[NSURL fileURLWithPath:absoluteSubpath]];
		}
	}
	
	return urls;
}

- (void)insertURLs:(NSArray *)urls atIndex:(int)index sort:(BOOL)sort
{
	NSMutableSet *uniqueURLs = [NSMutableSet set];
	
	NSMutableArray *expandedURLs = [NSMutableArray array];
	NSMutableArray *containedURLs = [NSMutableArray array];
	NSMutableArray *fileURLs = [NSMutableArray array];
	NSMutableArray *validURLs = [NSMutableArray array];
	
	if (!urls)
		return;
	
	if (index < 0)
		index = 0;
	
	NSURL *url;
	for (url in urls)
	{
		if ([url isFileURL]) {
			BOOL isDir;
			if ([[NSFileManager defaultManager] fileExistsAtPath:[url path] isDirectory:&isDir])
			{
				if (isDir == YES)
				{
					//Get subpaths
					[expandedURLs addObjectsFromArray:[self fileURLsAtPath:[url path]]];
				}
				else
				{
					[expandedURLs addObject:url];
				}
			}
		}
		else
		{
			//Non-file URL..
			[expandedURLs addObject:url];
		}
	}
	
	NSLog(@"Expanded urls: %@", expandedURLs);

	NSArray *sortedURLs;
	if (sort == YES)
	{
		NSSortDescriptor *sortDescriptor = [[NSSortDescriptor alloc] initWithKey:@"absoluteString" ascending:YES];

		sortedURLs = [expandedURLs sortedArrayUsingDescriptors:[NSArray arrayWithObject:sortDescriptor]];

		[sortDescriptor release];
	}
	else
	{
		sortedURLs = [expandedURLs copy];
	}

	for (url in sortedURLs)
	{
		//Container vs non-container url
		if ([[self acceptableContainerTypes] containsObject:[[[url path] pathExtension] lowercaseString]]) {
			[containedURLs addObjectsFromArray:[AudioContainer urlsForContainerURL:url]];

			//Make sure the container isn't added twice.
			[uniqueURLs addObjectsFromArray:containedURLs];
		}
		else
		{
			[fileURLs addObject:url];
		}
	}

	NSLog(@"File urls: %@", fileURLs);

	NSLog(@"Contained urls: %@", containedURLs);

	for (url in fileURLs)
	{
		if (![[AudioPlayer schemes] containsObject:[url scheme]])
			continue;

		//Need a better way to determine acceptable file types than basing it on extensions.
		if ([url isFileURL] && ![[AudioPlayer fileTypes] containsObject:[[[url path] pathExtension] lowercaseString]])
			continue;
		
		if (![uniqueURLs containsObject:url])
		{
			[validURLs addObject:url];
			
			[uniqueURLs addObject:url];
		}
	}
	
	NSLog(@"Valid urls: %@", validURLs);

	for (url in containedURLs)
	{
		if (![[AudioPlayer schemes] containsObject:[url scheme]])
			continue;

		//Need a better way to determine acceptable file types than basing it on extensions.
		if (![[AudioPlayer fileTypes] containsObject:[[[url path] pathExtension] lowercaseString]])
			continue;

		[validURLs addObject:url];
	}
	
	//Create actual entries
	int i;
	NSMutableArray *entries = [NSMutableArray arrayWithCapacity:[validURLs count]];
	for (i = 0; i < [validURLs count]; i++)
	{
		PlaylistEntry *pe = [[PlaylistEntry alloc] init];
		NSURL *url = [validURLs objectAtIndex:i];

		pe.URL = url;
		pe.index = index+i;
		pe.title = [[url path] lastPathComponent];
		pe.queuePosition = -1;
		[entries addObject:pe];

		[pe release];
	}
	
	NSIndexSet *is = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(index, [entries count])];
	
	[playlistController insertObjects:entries atArrangedObjectIndexes:is];
	
	//Select the first entry in the group that was just added
	[playlistController setSelectionIndex:index];
	
    NSOperationQueue *queue;
    queue = [[[NSApplication sharedApplication] delegate] sharedOperationQueue];
    
    for (PlaylistEntry *pe in entries)
    {
        NSInvocationOperation *readEntryInfoOperation;
        readEntryInfoOperation = [[NSInvocationOperation alloc]
                                    initWithTarget:self
                                          selector:@selector(readEntryInfo:)
                                            object:pe];

        [readEntryInfoOperation addObserver:self
                                 forKeyPath:@"isFinished"
                                    options:NSKeyValueObservingOptionNew
                                    context:NULL];
        [queue addOperation:readEntryInfoOperation];
		
		[readEntryInfoOperation release];
    }

	return;
}

- (NSDictionary *)readEntryInfo:(PlaylistEntry *)pe
{
    // Just setting this to 20 for now...
    NSMutableDictionary *entryInfo = [NSMutableDictionary dictionaryWithCapacity:20];
    NSDictionary *entryProperties;
    entryProperties = [AudioPropertiesReader propertiesForURL:pe.URL];
    if (entryProperties == nil)
        return nil;
        
    [entryInfo addEntriesFromDictionary:entryProperties];
    [entryInfo addEntriesFromDictionary:[AudioMetadataReader metadataForURL:pe.URL]];
    return entryInfo;
}

- (void)processEntryInfo:(NSInvocationOperation *)operation
{
    NSDictionary *entryInfo = [operation result];
    PlaylistEntry *pe;
    // get the playlist entry that the thread was inspecting
    [[operation invocation]getArgument:&pe atIndex:2];
    if (entryInfo == nil)
    {
        pe.error = YES;
        pe.errorMessage = @"Unable to retrieve properties.";
    }
    else
    {
        [pe setValuesForKeysWithDictionary:entryInfo];
    }
    return;
}

- (void)observeValueForKeyPath:(NSString *)keyPath
                      ofObject:(id)object
                        change:(NSDictionary *)change
                       context:(void *)context
{
    // We finished reading the info for a playlist entry
    if ([keyPath isEqualToString:@"isFinished"] && [object isFinished])
    {
        // stop observing
        [object removeObserver:self forKeyPath:keyPath];
        [self performSelectorOnMainThread:@selector(processEntryInfo:) withObject:object waitUntilDone:NO];  
		
    }
    else
    {
        [super observeValueForKeyPath:keyPath
                             ofObject:object
                               change:change
                              context:context];
    }
}

- (void)addURLs:(NSArray *)urls sort:(BOOL)sort
{
	[self insertURLs:urls atIndex:[[playlistController content] count] sort:sort];
}

- (void)addURL:(NSURL *)url
{
	[self insertURLs:[NSArray arrayWithObject:url] atIndex:[[playlistController content] count] sort:NO];
}

- (NSArray *)acceptableFileTypes
{
	return [[self acceptableContainerTypes] arrayByAddingObjectsFromArray:[AudioPlayer fileTypes]];
}

- (NSArray *)acceptablePlaylistTypes
{
	return [NSArray arrayWithObjects:@"m3u", @"pls", nil];
}

- (NSArray *)acceptableContainerTypes
{
	return [AudioPlayer containerTypes];
}

@end
