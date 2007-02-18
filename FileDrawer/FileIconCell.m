//
//  FileIconTextCell.m
//  Cog
//
//  Created by Vincent Spader on 8/20/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "FileIconCell.h"


@implementation FileIconCell

- (void)setObjectValue:(id)o
{
	if ([o respondsToSelector:@selector(icon)]) {
		[super setObjectValue:[[o path] lastPathComponent]];
		[super setImage: [o icon]];
	}
	else {
		[super setObjectValue:o];
	}
}

@end
