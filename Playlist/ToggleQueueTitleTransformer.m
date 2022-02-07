//
//  ToggleQueueTitleTransformer.m
//  Cog
//
//  Created by Vincent Spader on 3/8/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "ToggleQueueTitleTransformer.h"

#import "Logging.h"

@implementation ToggleQueueTitleTransformer

+ (Class)transformedValueClass {
	return [NSNumber class];
}
+ (BOOL)allowsReverseTransformation {
	return NO;
}

// Convert from NSNumber to NSString
- (id)transformedValue:(id)value {
	DLog(@"VALUE: %@", value);
	if(value == nil) return nil;
	BOOL queued = [value boolValue];

	if(queued) {
		return @"Remove from Queue";
	} else {
		return @"Add to Queue";
	}
}

@end
