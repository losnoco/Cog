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
@class ContainerNode;

@implementation PathNode

//From http://developer.apple.com/documentation/Cocoa/Conceptual/LowLevelFileMgmt/Tasks/ResolvingAliases.html
NSURL *resolveAliases(NSURL *url)
{
	FSRef fsRef;
	if (CFURLGetFSRef((CFURLRef)url, &fsRef))
	{
		Boolean targetIsFolder, wasAliased;

		if (FSResolveAliasFile (&fsRef, true /*resolveAliasChains*/, &targetIsFolder, &wasAliased) == noErr && wasAliased)
		{
			CFURLRef resolvedUrl = CFURLCreateFromFSRef(NULL, &fsRef);
			
			if (resolvedUrl != NULL)
			{
				NSLog(@"Resolved...");
				return [(NSURL *)resolvedUrl autorelease];
			}
		}
	}

	NSLog(@"Not resolved");
	return url;
}

- (id)initWithDataSource:(FileTreeDataSource *)ds url:(NSURL *)u
{
	self = [super init];

	if (self)
	{
		dataSource = ds;
		[self setURL: u];
	}
	
	return self;
}

- (void)stopWatching
{
	if (url)
	{
		NSLog(@"Stopped watching...: %@", [url path]);
		
		//Remove all in one go
		[[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:self];
		
		[[UKKQueue sharedFileWatcher] removePath:[url path]];
	}
}

- (void)startWatching
{
	if (url)
	{
		NSLog(@"WATCHING! %@", [url path]);

		[[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(updatePathNotification:) name:UKFileWatcherRenameNotification			object:nil];
		[[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(updatePathNotification:) name:UKFileWatcherWriteNotification			object:nil];
		[[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(updatePathNotification:) name:UKFileWatcherDeleteNotification			object:nil];
		[[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(updatePathNotification:) name:UKFileWatcherAttributeChangeNotification	object:nil];
		[[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(updatePathNotification:) name:UKFileWatcherSizeIncreaseNotification		object:nil];
		[[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(updatePathNotification:) name:UKFileWatcherLinkCountChangeNotification	object:nil];
		[[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self selector:@selector(updatePathNotification:) name:UKFileWatcherAccessRevocationNotification object:nil];

		[[UKKQueue sharedFileWatcher] addPath:[url path]];
	}
}

- (void)setURL:(NSURL *)u
{
	[u retain];
	
	[self stopWatching];
	
	[url release];

	url = u;

	[self startWatching];

	[display release];
	display = [[NSFileManager defaultManager] displayNameAtPath:[url path]];
	[display retain];
	
	[icon release];
	icon = [[NSWorkspace sharedWorkspace] iconForFile:[url path]];
	[icon retain];
	
	[icon setSize: NSMakeSize(16.0, 16.0)];
}

- (NSURL *)url
{
	return url;
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
	if ([p isEqualToString:[url path]])
	{
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
		
		NSURL *u = [NSURL fileURLWithPath:s];
		
		PathNode *newNode;
		
		NSLog(@"Before: %@", u);
		u = resolveAliases(u);
		NSLog(@"After: %@", u);
		
		if ([[s pathExtension] caseInsensitiveCompare:@"savedSearch"] == NSOrderedSame)
		{
			NSLog(@"Smart folder!");
			newNode = [[SmartFolderNode alloc] initWithDataSource:dataSource url:u];
		}
		else
		{
			BOOL isDir;
			
			[[NSFileManager defaultManager] fileExistsAtPath:[u path] isDirectory:&isDir];

			if (!isDir && ![[AudioPlayer fileTypes] containsObject:[[u path] pathExtension]])
			{
				continue;
			}
			
			if (isDir)
			{
				newNode = [[DirectoryNode alloc] initWithDataSource:dataSource url:u];
			}
			else if ([[AudioPlayer containerTypes] containsObject:[[[u path] pathExtension] lowercaseString]])
			{
				newNode = [[ContainerNode alloc] initWithDataSource:dataSource url:u];
			}
			else
			{
				newNode = [[FileNode alloc] initWithDataSource:dataSource url:u];
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

- (NSString *)display
{
	return display;
}

- (NSImage *)icon
{
	return icon;
}

- (void)dealloc
{
	[self stopWatching];
	
	[url release];
	[icon release];

	[subpaths release];
		
	[super dealloc];
}

@end
