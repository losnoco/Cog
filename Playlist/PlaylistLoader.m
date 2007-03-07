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

@implementation PlaylistLoader

//load/save playlist auto-determines type to be either pls or m3u.
- (BOOL)load:(NSString *)filename
{
	NSString *ext = [filename pathExtension];
	if ([ext isEqualToString:@"m3u"])
	{
		return [self loadM3u:filename];
	}
	else if ([ext isEqualToString:@"pls"])
	{
		return [self loadPls:filename];
	}
	else
	{
		return [self loadPls:filename] || [self loadM3u:filename];
	}
}

- (BOOL)save
{
	[self save:currentFilename asType:currentType];
}

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

- (BOOL)loadM3u:(NSString *)filename
{
}

- (NSString *)pathRelativeTo:(NSString *)filename forEntry:(PlaylistEntry *)pe
{
	NSString *basePath = [[[filename stringByStandardizingPath] stringByDeletingLastPathComponent] stringByAppendingString:@"/"];

	NSURL *entryURL = [pe url];
	if ([[entryURL scheme] isEqualToString:@"file"]) {
		//We want relative paths.
		NSMutableString *entryPath = [[[[entryURL path] stringByStandardizingPath] mutableCopy] autorelease];

		[entryPath replaceOccurrencesOfString:basePath withString:@"" options:(NSAnchoredSearch | NSLiteralSearch | NSCaseInsensitiveSearch) range:NSMakeRange(0, [entryURL length])];

		return entryPath;		
	}
	else {
		//Write [entryURL absoluteString] to file
		return [entryURL absoluteString];
	}
}

	return paths;
}

- (BOOL)saveM3u:(NSString *)filename
{
	NSFileHandle *fileHandle = [NSFileHandle fileHandleForWritingAtPath:filename];
	if (!fileHandle) {
		return NO;
	}

	PlaylistEntry *pe;
	NSEnumerator *e = [[playlistController content] objectEnumerator];

	while (pe = [e nextObject])
	{
		NSString *path = [self pathRelativeTo:filename forEntry:pe];
		[fileHandle writeData:[[path stringByAppendingString:@"\n"] dataUsingEncoding:NSUTF8StringEncoding]];
	}

	[self setCurrentFile:filename];
	[self setType:kPlaylistM3u];

	return YES;
}

- (BOOL)loadPls:(NSString *)filename
{
	NSError *error;
	NSStringEncoding enc;
	NSString *contents = [NSString stringWithContentsOfFile:filename encoding:&enc error:&error];
	if (error || !contents) {
		return NO;
	}

	NSArray *entries = [contents componentsSeparatedByString:@"\n"];

	NSString *entry;
	NSEnumerator *e = [entries objectEnumerator];

	while (entry = [e nextObject])
	{
		NSString *

	}

	[playlistController addURLs:urls];
	
	return YES;
}

- (BOOL)savePls:(NSString *)filename
{
	NSFileHandle *fileHandle = [NSFileHandle fileHandleForWritingAtPath:filename];
	if (!fileHandle) {
		return NO;
	}

	NSEnumerator *e = [[playlistController content] objectEnumerator];
	PlaylistEntry *pe;

	while (pe = [e nextObject])
	{
		NSString *path = [self pathRelativeTo:filename forEntry:pe];
		NSString *entry = [NSString stringWithFormat:@"File%i=%@\n",i,path];

		[fileHandle writeData:[entry dataUsingEncoding:NSUTF8StringEncoding]];
	}

	[self setCurrentFile:filename];
	[self setType:kPlaylistM3u];

	return YES;
}

- (NSArray *)acceptablePlaylistTypes
{
	return [NSArray arrayWithObject:@"m3u",@"pls",nil];
}

- (PlaylistType)currentType
{
	return currentType;
}

- (void)setCurrentType:(PlaylistType)type
{
	currentType = type;
}

- (NSString *)currentFile
{
	return currentFile;
}

- (void)setCurrentFile:(NSString *)file
{
	[file retain];
	[currentFile release];
	currentFile = file;
}

@end
