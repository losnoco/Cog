//
//  NSString+CogSort.m
//  Cog
//
//  Created by Matthew Grinshpun on 16/02/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "NSString+CogSort.h"

@implementation NSString (CogSort)

// This allows us to compare track numbers separated by slashes, ie "01/17" properly
// when sorting incoming search results
- (NSComparisonResult)compareTrackNumbers:(NSString *)aString {
	return [self compare:aString options:NSNumericSearch];
}

@end

@implementation NSURL (CogSort)

- (NSComparisonResult)compareTrackNumbers:(NSURL *)aURL {
	return [[self absoluteString] compareTrackNumbers:[aURL absoluteString]];
}

@end
