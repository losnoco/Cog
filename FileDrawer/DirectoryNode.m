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

#import "FileTreeController.h"
#import "FileTreeWatcher.h"

@implementation DirectoryNode

- (BOOL)isLeaf
{
	return NO;
}

- (NSArray *)subpaths
{
	if (subpaths == nil)
	{
		subpaths = [[NSMutableArray alloc] init];
		NSArray *contents = [[[NSFileManager defaultManager] directoryContentsAtPath:path] sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)];
		NSLog(@"Contents: %@", contents);
		[self processPaths: contents];
		
//		[[controller watcher] addPath:[self path]];
	}
	
	return subpaths;
}

@end
