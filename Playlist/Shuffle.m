//
//  Shuffle.m
//  Cog
//
//  Created by Vincent Spader on 1/14/06.
//  Revised by Eric Hanneken on 2/14/08.
//  Copyright 2008 Vincent Spader. All rights reserved.
//

#import "Shuffle.h"
#import "NSArray+ShuffleUtils.h"

@implementation Shuffle

+ (void)initialize {
	static BOOL initialized = NO;
	if(!initialized) {
		// Call srandom() exactly once.
		srandom((unsigned)time(NULL));
		initialized = YES;
	}
}

+ (NSMutableArray*)shuffleList:(NSArray*)l {
	NSArray* randomLongs = [NSArray arrayWithRandomLongs:[l count]];
	// randomLongs is an array of random integers, equal in length to l.

	NSArray* pairs = [NSArray zipArray:randomLongs withArray:l];
	// randomLongs and l are paired.

	NSArray* shuffledPairs = [pairs sortedArrayUsingSelector:@selector(compareFirsts:)];
	// The numbers from randomLongs are sorted in ascending order; the tracks from l
	// are in random order.

	// Peel the tracks off and return them.
	return [[NSArray unzipArray:shuffledPairs] objectAtIndex:1];
}

@end
