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
	NSArray *contents = [[[NSFileManager defaultManager] directoryContentsAtPath:[url path]] sortedArrayUsingSelector:@selector(finderCompare:)];
	NSMutableArray *fullPaths = [[NSMutableArray alloc] init];

	for (NSString *s in contents)
	{
		if (![s hasPrefix:@"."])
			[fullPaths addObject:[[url path] stringByAppendingPathComponent: s]];
	}

	[self processPaths: fullPaths];

	[fullPaths release];
}

@end
