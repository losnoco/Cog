//
//  ContainerNode.m
//  Cog
//
//  Created by Vincent Spader on 10/15/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "ContainerNode.h"
#import "CogAudio/AudioContainer.h"

#import "ContainedNode.h"

@implementation ContainerNode

- (BOOL)isLeaf
{
	return NO;
}

- (void)updatePath
{
	NSArray *urls = [AudioContainer urlsForContainerURL:url];
	
	NSURL *u;
	NSEnumerator *e = [urls objectEnumerator];
	NSMutableArray *paths = [[NSMutableArray alloc] init];
	while (u = [e nextObject])
	{
		ContainedNode *node = [[ContainedNode alloc] initWithDataSource:dataSource url:u];
		NSLog(@"Node: %@", u);
		[paths addObject:node];
	}	
	
	[self setSubpaths:paths];
	
	[paths release];
}

@end
