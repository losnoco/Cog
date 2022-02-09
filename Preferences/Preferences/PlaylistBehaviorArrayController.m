//
//  PlaylistBehaviorArrayController.m
//  General
//
//  Created by Vasily Fedoseyev on 5/18/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "PlaylistBehaviorArrayController.h"

@implementation PlaylistBehaviorArrayController
- (void)awakeFromNib {
	[self removeObjects:[self arrangedObjects]];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"Clear playlist and play", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"clearAndPlay"}];
	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"Enqueue", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"enqueue"}];
	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"Enqueue and play", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"enqueueAndPlay"}];
}

@end
