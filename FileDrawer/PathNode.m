//
//  Node.m
//  Cog
//
//  Created by Vincent Spader on 8/20/2006.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "PathNode.h"

@implementation PathNode

- (id)initWithPath:(NSString *)p
{
	self = [super init];

	if (self)
	{
		path = [p retain];
		[self setPathIcon:[[PathIcon alloc] initWithPath:path]];
	}
	
	return self;
}

- (void)dealloc
{
	[path release];
	[super dealloc];
}

- (NSString *)path
{
	return path;
}

- (id)pathIcon
{
	return pathIcon;
}

- (void)setPathIcon:(id)pi
{
	[pi retain];
	[pathIcon release];
	pathIcon = pi;
}


@end
