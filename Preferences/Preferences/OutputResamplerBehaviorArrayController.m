//
//  OutputResamplerBehaviorArrayController.m
//  General
//
//  Created by Christopher Snowhill on 01/11/22.
//
//

#import "OutputResamplerBehaviorArrayController.h"

@implementation OutputResamplerBehaviorArrayController
- (void)awakeFromNib
{
	[self removeObjects:[self arrangedObjects]];
    
	[self addObject:
     [NSDictionary dictionaryWithObjectsAndKeys:
      NSLocalizedStringFromTableInBundle(@"Lowest", nil, [NSBundle bundleForClass:[self class]], @"") , @"name",
      @"lowest", @"preference",nil]];
    
	[self addObject:
     [NSDictionary dictionaryWithObjectsAndKeys:
      NSLocalizedStringFromTableInBundle(@"Lower", nil, [NSBundle bundleForClass:[self class]], @"") , @"name",
      @"lower", @"preference",nil]];
    
	[self addObject:
     [NSDictionary dictionaryWithObjectsAndKeys:
      NSLocalizedStringFromTableInBundle(@"Normal", nil, [NSBundle bundleForClass:[self class]], @"") , @"name",
      @"normal", @"preference",nil]];
    
    [self addObject:
     [NSDictionary dictionaryWithObjectsAndKeys:
      NSLocalizedStringFromTableInBundle(@"Higher", nil, [NSBundle bundleForClass:[self class]], @"") , @"name",
      @"higher", @"preference",nil]];
    
	[self addObject:
     [NSDictionary dictionaryWithObjectsAndKeys:
      NSLocalizedStringFromTableInBundle(@"Highest", nil, [NSBundle bundleForClass:[self class]], @"") , @"name",
      @"highest", @"preference",nil]];
}

@end
