//
//  ToggleQueueTitleTransformer.m
//  Cog
//
//  Created by Vincent Spader on 3/8/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "ToggleQueueTitleTransformer.h"


@implementation ToggleQueueTitleTransformer


+ (Class)transformedValueClass { return [NSArray class]; }
+ (BOOL)allowsReverseTransformation { return NO; }

// Convert from NSNumber to NSString
- (id)transformedValue:(id)value {
    if (value == nil) return nil;
	if ([value count] == 0) return nil;
	
	BOOL trueFound = NO;
	BOOL falseFound = NO;
	
	for (NSNumber *q in value) {
		BOOL queued = [q boolValue];
		
		if (queued)
		{
			trueFound = YES;
		}
		else if (!queued)
		{
			falseFound = YES;
		}
	}
	
	
	if (trueFound && !falseFound)
		return @"Remove from Queue";
	else if (falseFound && !trueFound)
		return @"Add to Queue";
	else
		return @"Toggle Queued";

}


@end
