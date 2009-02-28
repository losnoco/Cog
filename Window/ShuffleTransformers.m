//
//  ShuffleTransformers.m
//  Cog
//
//  Created by Vincent Spader on 2/27/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "ShuffleTransformers.h"


@implementation ShuffleImageTransformer

+ (Class)transformedValueClass { return [NSImage class]; }
+ (BOOL)allowsReverseTransformation { return NO; }

// Convert from string to RepeatMode
- (id)transformedValue:(id)value {
    if (value == nil) return nil;
	
	BOOL shuffleEnabled = [value boolValue];
	
	if (shuffleEnabled == YES) {
		return [NSImage imageNamed:@"shuffle_on"];
	}
	return [NSImage imageNamed:@"shuffle_off"];
}

@end
