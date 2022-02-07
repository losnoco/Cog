//
//  Pair.h
//  Cog
//
//  Created by Eric Hanneken on 2/13/08.
//  Copyright 2008 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface NSArray (ShuffleUtils)

+ (NSArray *)arrayWithRandomLongs:(NSUInteger)count;

/*
 * zip produces a new array by pairing successive objects
 * from two input arrays until one is exhausted.  Only
 * pointers are copied; the objects are not.
 */
+ (NSArray *)zipArray:(NSArray *)x withArray:(NSArray *)y;

/*
 * Unzip produces a new pair of arrays by separating
 * an input array of pairs.  Only pointers are copied;
 * the objects are not.
 */
+ (NSArray *)unzipArray:(NSArray *)pairs;

- (NSComparisonResult)compareFirsts:(id)y;
- (id)first;
- (id)second;

@end