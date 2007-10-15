//
//  Node.m
//  Cog
//
//  Created by Vincent Spader on 8/20/2006.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "PathNode.h"

#import "CogAudio/AudioPlayer.h"

#import "FileTreeDataSource.h"

#import "UKKQueue.h"

@class FileNode;
@class DirectoryNode;
@class SmartFolderNode;

@implementation PathNode

- (id)initWithDataSource:(FileTreeDataSource *)ds path:(NSString *)p
{
	self = [super init];

	if (self)
	{
		dataSource = ds;
		[self setPath: p];
	}
	
	return self;
}

- (void)stopWatching
{
	if (path)
	{
		NSLog(@"Stopped watching...: %@", path);
		
		//Remove all in one go
		[[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:self];
		
		[[UKKQueue sharedFileWatcher] removePath:path];
	}
}

- (void)startWatching
{
	if (path)
	{
		NSLog(@"WATCHING! %@ %i", path, path);

		[[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(updatePathNotification:) name:UKFileWatcherRenameNotification			object:nil];
		[[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(updatePathNotification:) name:UKFileWatcherWriteNotification			object:nil];
		[[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(updatePathNotification:) name:UKFileWatcherDeleteNotification			object:nil];
		[[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(updatePathNotification:) name:UKFileWatcherAttributeChangeNotification	object:nil];
		[[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(updatePathNotification:) name:UKFileWatcherSizeIncreaseNotification		object:nil];
		[[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(updatePathNotification:) name:UKFileWatcherLinkCountChangeNotification	object:nil];
		[[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(updatePathNotification:) name:UKFileWatcherAccessRevocationNotification object:nil];

		[[UKKQueue sharedFileWatcher] addPath:path];
	}
}

- (void)setPath:(NSString *)p
{
	[p retain];
	
	[self stopWatching];
	
	[path release];

	path = p;

	[self startWatching];

	[displayPath release];
	displayPath = [[NSFileManager defaultManager] displayNameAtPath:path];
	[displayPath retain];
	
	[icon release];
	icon = [[NSWorkspace sharedWorkspace] iconForFile:path];
	[icon retain];
	
	[icon setSize: NSMakeSize(16.0, 16.0)];
}

- (NSString *)path
{
	return path;
}

- (void)updatePath
{
}

- (void)updatePathNotification:(NSNotification *)notification
{
	[self performSelectorOnMainThread:@selector(updatePathNotificationMainThread:) withObject:notification waitUntilDone:YES];
}

- (void)updatePathNotificationMainThread:(NSNotification *)notification
{
	NSString *p = [[notification userInfo] objectForKey:@"path"];
	if (p == path)
	{
		NSLog(@"Update path notification: %@", [NSThread currentThread]);
		
		[self updatePath];

		[dataSource reloadPathNode:self];
	}
}

- (void)processPaths: (NSArray *)contents
{
	NSMutableArray *newSubpaths = [[NSMutableArray alloc] init];
	
	NSEnumerator *e = [contents objectEnumerator];
	NSString *s;
	while ((s = [e nextObject]))
	{
		if ([s characterAtIndex:0] == '.')
		{
			continue;
		}
		
		PathNode *newNode;
		
		if ([[s pathExtension] caseInsensitiveCompare:@"savedSearch"] == NSOrderedSame)
		{
			NSLog(@"Smart folder!");
			newNode = [[SmartFolderNode alloc] initWithDataSource:dataSource path:s];
		}
		else
		{
			BOOL isDir;
			
			[[NSFileManager defaultManager] fileExistsAtPath:s isDirectory:&isDir];
			
			if (!isDir && ![[AudioPlayer fileTypes] containsObject:[s pathExtension]])
			{
				continue;
			}
			
			if (isDir)
			{
				newNode = [[DirectoryNode alloc] initWithDataSource:dataSource path: s];
			}
			else
			{
				newNode = [[FileNode alloc] initWithDataSource:dataSource path: s];
			}
		}
					
		[newSubpaths addObject:newNode];

		[newNode release];
	}
	
	[self setSubpaths:newSubpaths];
	
	[newSubpaths release];
}

- (NSArray *)subpaths
{
	if (subpaths == nil)
	{
		[self updatePath];
	}
	
	return subpaths;
}

- (void)setSubpaths:(NSArray *)s
{
	[s retain];
	[subpaths release];
	subpaths = s;
}


- (BOOL)isLeaf
{
	return YES;
}

- (NSString *)displayPath
{
	return displayPath;
}

- (NSImage *)icon
{
	return icon;
}

- (void)dealloc
{
	[self stopWatching];
	
	[path release];
	[icon release];

	[subpaths release];
		
	[super dealloc];
}

@end
