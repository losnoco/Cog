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
	NSLog(@"Transforming value: %@", value);
	
    if (value == nil) return nil;
	
	ShuffleMode mode = [value integerValue];
	
	if (mode == ShuffleOff) {
		return [NSImage imageNamed:@"shuffle_off"];
	}
	else if (mode == ShuffleAlbums) {
		return [NSImage imageNamed:@"shuffle_albums"];
	}
	else if (mode == ShuffleAll) {
		return [NSImage imageNamed:@"shuffle_on"];
	}
	
	return nil;
}

@end


@implementation ShuffleModeTransformer

+ (Class)transformedValueClass { return [NSNumber class]; }
+ (BOOL)allowsReverseTransformation { return YES; }

- (id)initWithMode:(ShuffleMode)s
{
	self = [super init];
	if (self)
	{
		shuffleMode = s;
	}
	
	return self;
}

// Convert from RepeatMode to BOOL
- (id)transformedValue:(id)value {
	NSLog(@"Transforming value: %@", value);
	
    if (value == nil) return nil;
	
	ShuffleMode mode = [value integerValue];
	
	if (shuffleMode == mode) {
		return [NSNumber numberWithBool:YES];
	}
	
	
	return [NSNumber numberWithBool:NO];
}

- (id)reverseTransformedValue:(id)value {
    if (value == nil) return nil;
	
	BOOL enabled = [value boolValue];
	if (enabled) {
		return [NSNumber numberWithInt:shuffleMode];
	}
	else if(shuffleMode == ShuffleOff) {
		return [NSNumber numberWithInt:ShuffleAll];
	}
	else {
		return [NSNumber numberWithInt:ShuffleOff];
	}
}

@end