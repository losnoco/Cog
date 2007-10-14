//
//  Node.m
//  Cog
//
//  Created by Vincent Spader on 8/20/2006.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "PathNode.h"

#import "CogAudio/AudioPlayer.h"

@class FileNode;
@class DirectoryNode;
@class SmartFolderNode;

@implementation PathNode

- (id)initWithPath:(NSString *)p
{
	self = [super init];

	if (self)
	{
		[self setPath: p];
	}
	
	return self;
}

- (void)dealloc
{
	[path release];
	[icon release];

	if (subpaths) {
		[subpaths release];
		subpaths = nil;
	}
		
	[super dealloc];
}

- (void)setPath:(NSString *)p
{
	[p retain];
	[path release];
	
	path = p;

	[icon release];
	icon = [[NSWorkspace sharedWorkspace] iconForFile:path];
	[icon retain];
	
	[icon setSize: NSMakeSize(16.0, 16.0)];
}

- (NSString *)path
{
	return path;
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
		NSString *newSubpath = [path stringByAppendingPathComponent: s];
		
		if ([[s pathExtension] caseInsensitiveCompare:@"savedSearch"] == NSOrderedSame)
		{
			newNode = [[SmartFolderNode alloc] initWithPath:newSubpath];
		}
		else
		{
			BOOL isDir;
			
			[[NSFileManager defaultManager] fileExistsAtPath:newSubpath isDirectory:&isDir];
			
			if (!isDir && ![[AudioPlayer fileTypes] containsObject:[s pathExtension]])
			{
				continue;
			}
			
			if (isDir)
				newNode = [[DirectoryNode alloc] initWithPath: newSubpath];
			else
				newNode = [[FileNode alloc] initWithPath: newSubpath];
		}
					
		[newSubpaths addObject:newNode];

		[newNode release];
	}
	
	[self setSubpaths:[[newSubpaths copy] autorelease]];
	
	[newSubpaths release];
}

- (NSArray *)subpaths
{
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


- (NSImage *)icon
{
	return icon;
}


@end
