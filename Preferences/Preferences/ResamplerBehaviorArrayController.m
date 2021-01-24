//
//  ResamplerBehaviorArrayController.m
//  General
//
//  Created by Christopher Snowhill on 03/26/14.
//
//

#import "ResamplerBehaviorArrayController.h"

@implementation ResamplerBehaviorArrayController
- (void)awakeFromNib
{
	[self removeObjects:[self arrangedObjects]];
    
	[self addObject:
     [NSDictionary dictionaryWithObjectsAndKeys:
      NSLocalizedStringFromTableInBundle(@"Zero Order Hold", nil, [NSBundle bundleForClass:[self class]], @"") , @"name",
      @"zoh", @"preference",nil]];
    
	[self addObject:
     [NSDictionary dictionaryWithObjectsAndKeys:
      NSLocalizedStringFromTableInBundle(@"Blep Synthesis", nil, [NSBundle bundleForClass:[self class]], @"") , @"name",
      @"blep", @"preference",nil]];
    
	[self addObject:
     [NSDictionary dictionaryWithObjectsAndKeys:
      NSLocalizedStringFromTableInBundle(@"Linear Interpolation", nil, [NSBundle bundleForClass:[self class]], @"") , @"name",
      @"linear", @"preference",nil]];
    
    [self addObject:
     [NSDictionary dictionaryWithObjectsAndKeys:
      NSLocalizedStringFromTableInBundle(@"Blam Synthesis", nil, [NSBundle bundleForClass:[self class]], @"") , @"name",
      @"blam", @"preference",nil]];
    
	[self addObject:
     [NSDictionary dictionaryWithObjectsAndKeys:
      NSLocalizedStringFromTableInBundle(@"Cubic Interpolation", nil, [NSBundle bundleForClass:[self class]], @"") , @"name",
      @"cubic", @"preference",nil]];
    
	[self addObject:
     [NSDictionary dictionaryWithObjectsAndKeys:
      NSLocalizedStringFromTableInBundle(@"Sinc Interpolation", nil, [NSBundle bundleForClass:[self class]], @"") , @"name",
      @"sinc", @"preference",nil]];
}

@end
