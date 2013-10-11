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

#import "FileNode.h"
#import "DirectoryNode.h"
#import "SmartFolderNode.h"
#import "ContainerNode.h"

#import "Logging.h"

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
				//DLog(@"Resolved...");
				return [(NSURL *)resolvedUrl autorelease];
			}
		}
	}

	//DLog(@"Not resolved");
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

- (void)setURL:(NSURL *)u
{
	[u retain];
	
	[url release];

	url = u;

	[display release];
	display = [[NSFileManager defaultManager] displayNameAtPath:[u path]];
	[display retain];
	
	[icon release];
	icon = [[NSWorkspace sharedWorkspace] iconForFile:[url path]];
	[icon retain];
	
	[icon setSize: NSMakeSize(16.0, 16.0)];
}

- (NSURL *)URL
{
	return url;
}

- (void)updatePath
{
}

- (void)processPaths: (NSArray *)contents
{
    NSMutableArray *newSubpathsDirs = [[NSMutableArray alloc] init];
	NSMutableArray *newSubpaths = [[NSMutableArray alloc] init];
	
    for (NSString *s in contents)
	{
		if ([s characterAtIndex:0] == '.')
		{
			continue;
		}
		
		NSURL *u = [NSURL fileURLWithPath:s];
		NSString *displayName = [[NSFileManager defaultManager] displayNameAtPath:[u path]];
		
		PathNode *newNode;
		
		//DLog(@"Before: %@", u);
		u = resolveAliases(u);
		//DLog(@"After: %@", u);

        BOOL isDir;
        
		if ([[s pathExtension] caseInsensitiveCompare:@"savedSearch"] == NSOrderedSame)
		{
			DLog(@"Smart folder!");
			newNode = [[SmartFolderNode alloc] initWithDataSource:dataSource url:u];
            isDir = NO;
		}
		else
		{
			[[NSFileManager defaultManager] fileExistsAtPath:[u path] isDirectory:&isDir];

			if (!isDir && ![[AudioPlayer fileTypes] containsObject:[[[u path] pathExtension] lowercaseString]])
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

		[newNode setDisplay:displayName];
        
        if (isDir)
            [newSubpathsDirs addObject:newNode];
        else
            [newSubpaths addObject:newNode];

		[newNode release];
	}
	
    [newSubpathsDirs addObjectsFromArray:newSubpaths];
	[self setSubpaths:newSubpathsDirs];
	
    [newSubpathsDirs release];
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

- (void)setDisplay:(NSString *)s
{
	[display release];
	display = s;
	[display retain];
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
	[url release];
	[icon release];

	[subpaths release];
		
	[super dealloc];
}

@end
