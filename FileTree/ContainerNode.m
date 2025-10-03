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

#import "Logging.h"

@implementation ContainerNode

- (BOOL)isLeaf {
	return NO;
}

- (void)updatePath {
	NSArray *urls = [AudioContainer urlsForContainerURL:url];

	NSMutableArray *paths = [NSMutableArray new];
	for(NSURL *u in urls) {
		ContainedNode *node = [[ContainedNode alloc] initWithDataSource:dataSource url:u];
		DLog(@"Node: %@", u);
		[paths addObject:node];
	}

	[self setSubpaths:paths];
}

@end
