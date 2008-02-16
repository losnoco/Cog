//
//  NSArray+CogSort.m
//  Cog
//
//  Created by Matthew Grinshpun on 16/02/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "NSArray+CogSort.h"


@implementation NSArray (CogSort)

// Hack to sort search results by artist
- (NSComparisonResult)compareFirstString:(NSArray *)anArray
{
    return [(NSString *)[self objectAtIndex:0] 
        caseInsensitiveCompare:(NSString *)[anArray objectAtIndex:0]];
}


@end
