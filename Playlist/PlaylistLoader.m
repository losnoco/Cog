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

#import "NSFileHandle+CreateFile.h"

#import "CogAudio/AudioPlayer.h"
#import "CogAudio/AudioContainer.h"

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

	if ([entryURL isFileURL] && [[entryURL fragment] isEqualToString:@""]) {
		//We want relative paths.
		NSMutableString *entryPath = [[[[entryURL path] stringByStandardizingPath] mutableCopy] autorelease];

		[entryPath replaceOccurrencesOfString:basePath withString:@"" options:(NSAnchoredSearch | NSLiteralSearch | NSCaseInsensitiveSearch) range:NSMakeRange(0, [entryPath length])];

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
		return nil;
	}
	[fileHandle truncateFileAtOffset:0];
	
	PlaylistEntry *pe;
	NSEnumerator *e = [[playlistController content] objectEnumerator];

	while (pe = [e nextObject])
	{
		NSString *path = [self relativePathFrom:filename toURL:[pe url]];
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

	NSEnumerator *e = [[playlistController content] objectEnumerator];
	PlaylistEntry *pe;
	int i = 1;
	while (pe = [e nextObject])
	{
		NSString *path = [self relativePathFrom:filename toURL:[pe url]];
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
		
	NSString *subpath;
	NSArray *subpaths = [manager subpathsAtPath:path];
	NSEnumerator *e = [subpaths objectEnumerator];

	while(subpath = [e nextObject])
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
	NSMutableArray *allURLs = [NSMutableArray array];
	NSMutableArray *validURLs = [NSMutableArray array];
	
	if (!urls)
		return;
	
	if (index < 0)
		index = 0;
	
	NSEnumerator *urlEnumerator = [urls objectEnumerator];
	NSURL *url;
	while (url = [urlEnumerator nextObject])
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

	urlEnumerator = [sortedURLs objectEnumerator];
	while (url = [urlEnumerator nextObject])
	{
		//File url
		if ([[self acceptableContainerTypes] containsObject:[[[url path] pathExtension] lowercaseString]]) {
			if ([url isFileURL] ) {
				[allURLs addObjectsFromArray:[AudioContainer urlsForContainerURL:url]];
			}
		}
		else
		{
			[allURLs addObject:url];
		}
	}

	urlEnumerator = [allURLs objectEnumerator];
	while (url = [urlEnumerator nextObject])
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

	//Create actual entries
	int i;
	NSMutableArray *entries = [NSMutableArray array];
	for (i = 0; i < [validURLs count]; i++)
	{
		PlaylistEntry *pe = [[PlaylistEntry alloc] init];
		NSURL *url = [validURLs objectAtIndex:i];

		[pe	setURL:url];
		[pe setIndex:[NSNumber numberWithInt:(index+i)]];
		[pe setTitle:[[url path] lastPathComponent]];
		
		[entries addObject:pe];

		[pe release];
	}
	
	NSIndexSet *is = [NSIndexSet indexSetWithIndexesInRange:NSMakeRange(index, [entries count])];

	[playlistController insertObjects:entries atArrangedObjectIndexes:is];
	
	//Select the first entry in the group that was just added
	[playlistController setSelectionIndex:index];
	
	//Other thread for reading things...
	[NSThread detachNewThreadSelector:@selector(readEntriesInfoThread:) toTarget:self withObject:entries];
	
	return;
}

- (void)readEntriesInfoThread:(NSArray *)entries
{
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	NSEnumerator *e = [entries objectEnumerator];
	PlaylistEntry *pe;
	while (pe = [e nextObject])
	{
		[pe readPropertiesThread];

		[pe readMetadataThread];

		//Hack so the display gets updated
		if (pe == [playlistController currentEntry])
			[playlistController performSelectorOnMainThread:@selector(setCurrentEntry:) withObject:[playlistController currentEntry] waitUntilDone:YES];
	}


	[playlistController performSelectorOnMainThread:@selector(updateTotalTime) withObject:nil waitUntilDone:NO];
	
	[pool release];
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
