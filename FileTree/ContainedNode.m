//
//  ContainedNode.m
//  Cog
//
//  Created by Vincent Spader on 10/15/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "ContainedNode.h"


@implementation ContainedNode

- (BOOL)isLeaf
{
	return YES;
}

- (void)setURL:(NSURL *)u
{
	[super setURL:u];
	
	if ([u fragment])
	{
		[display release];
		display = [[u fragment] retain];
	}
}

@end
