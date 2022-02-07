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

	[self addObject:
	      [NSDictionary dictionaryWithObjectsAndKeys:
	                    NSLocalizedStringFromTableInBundle(@"ReplayGain Album Gain with peak", nil, [NSBundle bundleForClass:[self class]], @""), @"name",
	                    @"albumGainWithPeak", @"preference", nil]];

	[self addObject:
	      [NSDictionary dictionaryWithObjectsAndKeys:
	                    NSLocalizedStringFromTableInBundle(@"ReplayGain Album Gain", nil, [NSBundle bundleForClass:[self class]], @""), @"name",
	                    @"albumGain", @"preference", nil]];

	[self addObject:
	      [NSDictionary dictionaryWithObjectsAndKeys:
	                    NSLocalizedStringFromTableInBundle(@"ReplayGain Track Gain with peak", nil, [NSBundle bundleForClass:[self class]], @""), @"name",
	                    @"trackGainWithPeak", @"preference", nil]];

	[self addObject:
	      [NSDictionary dictionaryWithObjectsAndKeys:
	                    NSLocalizedStringFromTableInBundle(@"ReplayGain Track Gain", nil, [NSBundle bundleForClass:[self class]], @""), @"name",
	                    @"trackGain", @"preference", nil]];

	[self addObject:
	      [NSDictionary dictionaryWithObjectsAndKeys:
	                    NSLocalizedStringFromTableInBundle(@"Volume scale tag only", nil, [NSBundle bundleForClass:[self class]], @""), @"name",
	                    @"volumeScale", @"preference", nil]];

	[self addObject:
	      [NSDictionary dictionaryWithObjectsAndKeys:
	                    NSLocalizedStringFromTableInBundle(@"No volume scaling", nil, [NSBundle bundleForClass:[self class]], @""), @"name",
	                    @"none", @"preference", nil]];
}

@end
