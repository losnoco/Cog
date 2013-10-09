//
//  PlaylistLoader.m
//  Cog
//
//  Created by Vincent Spader on 3/05/07.
//  Copyright 2007 Vincent Spader All rights reserved.
//

#include <objc/runtime.h>

#import "PlaylistLoader.h"
#import "PlaylistController.h"
#import "PlaylistEntry.h"
#import "FilePlaylistEntry.h"
#import "AppController.h"

#import "NSFileHandle+CreateFile.h"

#import "CogAudio/AudioPlayer.h"
#import "CogAudio/AudioContainer.h"
#import "CogAudio/AudioPropertiesReader.h"
#import "CogAudio/AudioMetadataReader.h"

#import "XMlContainer.h"

@implementation PlaylistLoader

- (id)init
{
	self = [super init];
	if (self)
	{
		[self initDefaults];
		
		queue = [[NSOperationQueue alloc] init];
		[queue setMaxConcurrentOperationCount:1];
	}
	
	return self; 
}

- (void)initDefaults
{
	NSDictionary *defaultsDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
										[NSNumber numberWithBool:YES], @"readCueSheetsInFolders",
										nil];
	
	[[NSUserDefaults standardUserDefaults] registerDefaults:defaultsDictionary];
}

- (void)dealloc
{
	[queue release];
	
	[super dealloc];
}

- (BOOL)save:(NSString *)filename
{
	NSString *ext = [filename pathExtension];
	if ([ext isEqualToString:@"pls"])
	{
		return [self save:filename asType:kPlaylistPls];
	}
    else if ([ext isEqualToString:@"xml"])
    {
        return [self save:filename asType:kPlaylistXml];
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
    else if (type == kPlaylistXml)
    {
        return [self saveXml:filename];
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

	[fileHandle writeData:[[NSString stringWithFormat:@"[playlist]\nnumberOfEntries=%lu\n\n",(unsigned long)[[playlistController content] count]] dataUsingEncoding:NSUTF8StringEncoding]];

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

NSMutableDictionary * dictionaryWithPropertiesOfObject(id obj, NSArray * filterList)
{
    NSMutableDictionary *dict = [NSMutableDictionary dictionary];

    Class class = [obj class];
    
    do {
        unsigned count;
        objc_property_t *properties = class_copyPropertyList(class, &count);
        
        for (int i = 0; i < count; i++) {
            NSString *key = [NSString stringWithUTF8String:property_getName(properties[i])];
            if ([filterList containsObject:key]) continue;
            
            Class classObject = NSClassFromString([key capitalizedString]);
            if (classObject) {
                id subObj = dictionaryWithPropertiesOfObject([obj valueForKey:key], filterList);
                [dict setObject:subObj forKey:key];
            }
            else
            {
                id value = [obj valueForKey:key];
                if(value) [dict setObject:value forKey:key];
            }
        }
        
        free(properties);
        
        class = [class superclass];
    } while (class);
    
    return dict;
}

- (BOOL)saveXml:(NSString *)filename
{
	NSFileHandle *fileHandle = [NSFileHandle fileHandleForWritingAtPath:filename createFile:YES];
	if (!fileHandle) {
		return NO;
	}
	[fileHandle truncateFileAtOffset:0];
    
    NSArray * filterList = [NSArray arrayWithObjects:@"display", @"length", @"path", @"filename", @"status", @"statusMessage", @"spam", @"stopAfter", @"shuffleIndex", @"index", @"current", @"queued", @"currentPosition", @"queuePosition", @"error", @"removed", @"URL", @"albumArt", nil];
    
    NSMutableArray * topLevel = [[NSMutableArray alloc] init];
    
	for (PlaylistEntry *pe in [playlistController content])
	{
        NSMutableDictionary * dict = dictionaryWithPropertiesOfObject(pe, filterList);

		NSString *path = [self relativePathFrom:filename toURL:[pe URL]];
        
        [dict setObject:path forKey:@"URL"];
        NSData * albumArt = [dict objectForKey:@"albumArtInternal"];
        if (albumArt)
        {
            [dict setObject:albumArt forKey:@"albumArt"];
            [dict removeObjectForKey:@"albumArtInternal"];
        }
        
        [topLevel addObject:dict];
        
        [dict release];
	}
    
    NSData * data = [NSPropertyListSerialization dataWithPropertyList:topLevel format:NSPropertyListXMLFormat_v1_0 options:0 error:0];
    
    [fileHandle writeData:data];
    
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
			if ([[absoluteSubpath pathExtension] caseInsensitiveCompare:@"cue"] != NSOrderedSame ||
				[[NSUserDefaults standardUserDefaults] boolForKey:@"readCueSheetsInFolders"])
			{
				[urls addObject:[NSURL fileURLWithPath:absoluteSubpath]];
			}
		}
	}
	
	return urls;
}

- (NSArray*)insertURLs:(NSArray *)urls atIndex:(int)index sort:(BOOL)sort
{
	NSMutableSet *uniqueURLs = [NSMutableSet set];
	
	NSMutableArray *expandedURLs = [NSMutableArray array];
	NSMutableArray *containedURLs = [NSMutableArray array];
	NSMutableArray *fileURLs = [NSMutableArray array];
	NSMutableArray *validURLs = [NSMutableArray array];
    NSArray *xmlURLs = nil;
	
	if (!urls)
		return [NSArray array];
	
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
		sortedURLs = [expandedURLs sortedArrayUsingSelector:@selector(finderCompare:)];
//		sortedURLs = [expandedURLs sortedArrayUsingSelector:@selector(compareTrackNumbers:)];
	}
	else
	{
		sortedURLs = expandedURLs;
	}

	for (url in sortedURLs)
	{
		//Container vs non-container url
		if ([[self acceptableContainerTypes] containsObject:[[[url path] pathExtension] lowercaseString]]) {
			[containedURLs addObjectsFromArray:[AudioContainer urlsForContainerURL:url]];

			//Make sure the container isn't added twice.
			[uniqueURLs addObjectsFromArray:containedURLs];
		}
        else if ([[[[url path] pathExtension] lowercaseString] isEqualToString:@"xml"])
        {
            xmlURLs = [XmlContainer entriesForContainerURL:url];
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
		if ([url isFileURL] && ![[AudioPlayer fileTypes] containsObject:[[[url path] pathExtension] lowercaseString]])
			continue;

		[validURLs addObject:url];
	}
	
	//Create actual entries
    int count = [validURLs count];
    if (xmlURLs) count += [xmlURLs count];
    
	int i;
	NSMutableArray *entries = [NSMutableArray arrayWithCapacity:count];
	for (i = 0; i < [validURLs count]; i++)
	{
		NSURL *url = [validURLs objectAtIndex:i];
		
		PlaylistEntry *pe;
		if ([url isFileURL]) 
			pe = [[FilePlaylistEntry alloc] init];
		else
			pe = [[PlaylistEntry alloc] init];

		pe.URL = url;
		pe.index = index+i;
		pe.title = [[url path] lastPathComponent];
		pe.queuePosition = -1;
		[entries addObject:pe];

		[pe release];
	}
    
    for (i = 0; i < [xmlURLs count]; i++)
    {
        NSDictionary * entry = [xmlURLs objectAtIndex:i];
        
        PlaylistEntry *pe;
        if ([[entry objectForKey:@"URL"] isFileURL])
            pe = [[FilePlaylistEntry alloc] init];
        else
            pe = [[PlaylistEntry alloc] init];
        
        [pe setValuesForKeysWithDictionary:entry];
        pe.index = index+i+[validURLs count];
        pe.queuePosition = -1;
        [entries addObject:pe];
        
        [pe release];
    }
	
	NSIndexSet *is = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(index, [entries count])];
	
	[playlistController insertObjects:entries atArrangedObjectIndexes:is];
	
	//Select the first entry in the group that was just added
	[playlistController setSelectionIndex:index];
	[self performSelectorInBackground:@selector(loadInfoForEntries:) withObject:entries];
	return entries;
}

- (void)loadInfoForEntries:(NSArray *)entries
{
    for (PlaylistEntry *pe in entries)
    {
        if ([pe metadataLoaded]) continue;
        
		NSAutoreleasePool *pool =[[NSAutoreleasePool alloc] init];

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

		[pool release];
    }

	[queue waitUntilAllOperationsAreFinished];

	[playlistController performSelectorOnMainThread:@selector(updateTotalTime) withObject:nil waitUntilDone:NO];
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

		// get the playlist entry that the operation read for
		PlaylistEntry *pe = nil;
		[[object invocation] getArgument:&pe atIndex:2];
        [pe performSelectorOnMainThread:@selector(setMetadata:) withObject:[object result] waitUntilDone:NO];  
		
    }
    else
    {
        [super observeValueForKeyPath:keyPath
                             ofObject:object
                               change:change
                              context:context];
    }
}

- (void)clear:(id)sender
{
	[playlistController clear:sender];
}

- (NSArray*)addURLs:(NSArray *)urls sort:(BOOL)sort
{
	return [self insertURLs:urls atIndex:[[playlistController content] count] sort:sort];
}

- (NSArray*)addURL:(NSURL *)url
{
	return [self insertURLs:[NSArray arrayWithObject:url] atIndex:[[playlistController content] count] sort:NO];
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

- (void)willInsertURLs:(NSArray*)urls origin:(URLOrigin)origin
{
	[playlistController willInsertURLs:urls origin:origin];
}
- (void)didInsertURLs:(NSArray*)urls origin:(URLOrigin)origin
{
	[playlistController didInsertURLs:urls origin:origin];
}

@end
