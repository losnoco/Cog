//
//  NSNumber+CogSort.m
//  Cog
//
//  Created by Matthew Grinshpun on 16/02/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "NSNumber+CogSort.h"


@implementation NSNumber (CogSort)

// Sometimes track numbers are CFNumbers
- (NSComparisonResult)compareTrackNumbers:(NSNumber *)aNumber
{
    return [self compare:aNumber];
}

@end
