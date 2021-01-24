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

    //[self addObject:
    // [NSDictionary dictionaryWithObjectsAndKeys:
    //  NSLocalizedStringFromTableInBundle(@"Stable", nil, [NSBundle bundleForClass:[self class]], @"") , @"name", @"http://mamburu.net/cog/stable.xml", @"url",nil]];

    //      [self addObject:
    //              [NSDictionary dictionaryWithObjectsAndKeys:
    //                      NSLocalizedStringFromTableInBundle(@"Unstable", nil, [NSBundle bundleForClass:[self class]], @"") , @"name", @"http://cogx.org/appcast/unstable.xml", @"url",nil]];
    
    //[self addObject:
    // [NSDictionary dictionaryWithObjectsAndKeys:
    //  NSLocalizedStringFromTableInBundle(@"Nightly", nil, [NSBundle bundleForClass:[self class]], @"") , @"name", @"http://mamburu.net/cog/nightly.xml", @"url",nil]];

    [self addObject:
     [NSDictionary dictionaryWithObjectsAndKeys:
      NSLocalizedStringFromTableInBundle(@"losno.co wheneverly", nil, [NSBundle bundleForClass:[self class]], @"") , @"name", @"https://f.losno.co/cog/mercury.xml", @"url",nil]];
}

@end
