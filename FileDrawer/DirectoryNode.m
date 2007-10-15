//
//  DirectoryNode.m
//  Cog
//
//  Created by Vincent Spader on 8/20/2006.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "DirectoryNode.h"

#import "FileNode.h"
#import "SmartFolderNode.h"

@implementation DirectoryNode

- (BOOL)isLeaf
{
	return NO;
}

- (void)updatePath
{
	NSArray *contents = [[[NSFileManager defaultManager] directoryContentsAtPath:path] sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)];
	NSMutableArray *fullPaths = [[NSMutableArray alloc] init];
	NSString *s;
	NSEnumerator *e = [contents objectEnumerator];
	while (s = [e nextObject])
	{
		[fullPaths addObject:[path stringByAppendingPathComponent: s]];
	}

	[self processPaths: fullPaths];

	[fullPaths release];
}

@end
