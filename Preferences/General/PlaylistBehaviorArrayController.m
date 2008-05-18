//
//  PlaylistBehaviorArrayController.m
//  General
//
//  Created by Vasily Fedoseyev on 5/18/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "PlaylistBehaviorArrayController.h"


@implementation PlaylistBehaviorArrayController
- (void)awakeFromNib
{
	[self removeObjects:[self arrangedObjects]];

	[self addObject:
		[NSDictionary dictionaryWithObjectsAndKeys:
			NSLocalizedStringFromTableInBundle(@"Clear playlist and play", nil, [NSBundle bundleForClass:[self class]], @"") , @"name", 
			@"clearAndPlay", @"slug",nil]];
	[self addObject:
		[NSDictionary dictionaryWithObjectsAndKeys:
			NSLocalizedStringFromTableInBundle(@"Enqueue", nil, [NSBundle bundleForClass:[self class]], @"") , @"name", 
			@"enqueue", @"slug",nil]];
	[self addObject:
		[NSDictionary dictionaryWithObjectsAndKeys:
			NSLocalizedStringFromTableInBundle(@"Enqueue and play", nil, [NSBundle bundleForClass:[self class]], @"") , @"name", 
			@"enqueueAndPlay", @"slug",nil]];
			
}

@end
