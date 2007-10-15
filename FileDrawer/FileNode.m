//
//  FileNode.m
//  Cog
//
//  Created by Vincent Spader on 8/20/2006.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "FileNode.h"

@implementation FileNode

- (BOOL)isLeaf
{
	return YES;
}

//Disable watching for us, it doesn't matter.
- (void)startWatching
{
}
- (void)stopWatching
{
}
- (void)updatePathNotification:(NSNotification *)notification
{
}

@end
