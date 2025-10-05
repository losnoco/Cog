//
//  VolumeBehaviorArrayController.m
//  General
//
//  Created by Christopher Snowhill on 10/1/13.
//
//

#import "VolumeBehaviorArrayController.h"

@implementation VolumeBehaviorArrayController
- (void)awakeFromNib {
	[self removeObjects:[self arrangedObjects]];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"ReplayGain Album Gain with peak", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"albumGainWithPeak"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"ReplayGain Album Gain", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"albumGain"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"ReplayGain Track Gain with peak", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"trackGainWithPeak"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"ReplayGain Track Gain", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"trackGain"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"SoundCheck", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"soundcheck"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"Volume scale tag only", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"volumeScale"}];

	[self addObject:@{@"name": NSLocalizedStringFromTableInBundle(@"No volume scaling", nil, [NSBundle bundleForClass:[self class]], @""), @"preference": @"none"}];
}

@end
