//
//  AppcastArrayController.m
//  General
//
//  Created by Vincent Spader on 5/19/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "AppcastArrayController.h"


@implementation AppcastArrayController

- (void)awakeFromNib
{
	[self removeObjects:[self arrangedObjects]];

	[self addObject:
		[NSDictionary dictionaryWithObjectsAndKeys:
			NSLocalizedString(@"Stable", @""), @"name", @"http://cogx.org/appcast/stable.xml", @"url",nil]];
			
	[self addObject:
		[NSDictionary dictionaryWithObjectsAndKeys:
			NSLocalizedString(@"Unstable", @""), @"name", @"http://cogx.org/appcast/unstable.xml", @"url",nil]];
			
	[self addObject:
		[NSDictionary dictionaryWithObjectsAndKeys:
			NSLocalizedString(@"Nightly", @""), @"name", @"http://cogx.org/appcast/nightly.xml", @"url",nil]];
}


@end
