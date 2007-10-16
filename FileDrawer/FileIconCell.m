//
//  FileIconTextCell.m
//  Cog
//
//  Created by Vincent Spader on 8/20/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "FileIconCell.h"
#import "PathNode.h"

@implementation FileIconCell

- (void)setObjectValue:(PathNode *)o
{
	if ([o respondsToSelector:@selector(icon)] && [o respondsToSelector:@selector(display)]) {
		[super setObjectValue:[o display]];
		[super setImage: [o icon]];
	}
	else {
		[super setObjectValue:(id)o];
	}
}

@end
