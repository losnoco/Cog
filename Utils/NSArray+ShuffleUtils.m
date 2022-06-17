//
//  Pair.m
//  Cog
//
//  Created by Eric Hanneken on 2/13/08.
//  Copyright 2008 Vincent Spader. All rights reserved.
//

#import "NSArray+ShuffleUtils.h"

@implementation NSArray (ShuffleUtils)

/*
 * Generates an array of random long integers in the range
 * 0 to (2**31) - 1.  The length of the array is determined
 * by the count parameter.
 */
+ (NSArray*)arrayWithRandomLongs:(NSUInteger)count {
	NSMutableArray* randomLongs = [NSMutableArray arrayWithCapacity:count];
	NSUInteger i;
	for(i = 0; i < count; ++i) {
		[randomLongs addObject:@(random())];
	}

	return randomLongs;
}

+ (NSArray*)zipArray:(NSArray*)x withArray:(NSArray*)y {
	NSUInteger xCount = [x count];
	NSUInteger yCount = [y count];
	NSUInteger minCount = (xCount < yCount) ? xCount : yCount;
	NSMutableArray* pairs = [NSMutableArray arrayWithCapacity:minCount];
	NSUInteger i;
	for(i = 0; i < minCount; ++i) {
		NSArray* p = @[[x objectAtIndex:i], [y objectAtIndex:i]];
		[pairs addObject:p];
	}

	return pairs;
}

+ (NSArray*)unzipArray:(NSArray*)pairs {
	NSMutableArray* first = [NSMutableArray arrayWithCapacity:[pairs count]];
	NSMutableArray* second = [NSMutableArray arrayWithCapacity:[pairs count]];

	for(NSArray* pair in pairs) {
		[first addObject:[pair first]];
		[second addObject:[pair second]];
	}

	return @[first, second];
}

/*
 * Compares two pairs by their first objects.
 */
- (NSComparisonResult)compareFirsts:(id)y {
	return [[self first] compare:[y first]];
}

- (id)first {
	return [self objectAtIndex:0];
}

- (id)second {
	return [self objectAtIndex:1];
}

@end
