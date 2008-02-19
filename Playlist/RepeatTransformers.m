//
//  RepeatModeTransformer.m
//  Cog
//
//  Created by Vincent Spader on 2/18/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "RepeatTransformers.h"
#import "PlaylistController.h"

@implementation RepeatModeTransformer

+ (Class)transformedValueClass { return [NSNumber class]; }
+ (BOOL)allowsReverseTransformation { return NO; }

- (id)initWithMode:(RepeatMode) r
{
	self = [super init];
	if (self)
	{
		repeatMode = r;
	}
	
	return self;
}

// Convert from RepeatMode to BOOL
- (id)transformedValue:(id)value {
	NSLog(@"Transforming value: %@", value);
	
    if (value == nil) return nil;
	
	RepeatMode mode = [value integerValue];
	
	if (repeatMode == mode) {
		return [NSNumber numberWithBool:YES];
	}
	

	return [NSNumber numberWithBool:NO];
}

@end

@implementation RepeatModeImageTransformer

+ (Class)transformedValueClass { return [NSImage class]; }
+ (BOOL)allowsReverseTransformation { return NO; }

// Convert from string to RepeatMode
- (id)transformedValue:(id)value {
	NSLog(@"Transforming value: %@", value);
	
    if (value == nil) return nil;

	RepeatMode mode = [value integerValue];
	
	if (mode == RepeatNone) {
		return [NSImage imageNamed:@"repeat_none"];
	}
	else if (mode == RepeatOne) {
		return [NSImage imageNamed:@"repeat_one"];
	}
	else if (mode == RepeatAll) {
		return [NSImage imageNamed:@"repeat_all"];
	}

	return nil;
}

@end
