//
//  ResamplerBehaviorArrayController.m
//  General
//
//  Created by Christopher Snowhill on 03/26/14.
//
//

#import "ResamplerBehaviorArrayController.h"

@implementation ResamplerBehaviorArrayController
- (void)awakeFromNib {
	[self removeObjects:[self arrangedObjects]];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"Zero Order Hold", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"zoh"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"Blep Synthesis", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"blep"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"Linear Interpolation", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"linear"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"Blam Synthesis", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"blam"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"Cubic Interpolation", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"cubic"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"Sinc Interpolation", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"sinc"}];
}

@end
