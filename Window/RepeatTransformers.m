//
//  RepeatModeTransformer.m
//  Cog
//
//  Created by Vincent Spader on 2/18/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "RepeatTransformers.h"
#import "PlaylistController.h"

#import "Logging.h"

@implementation RepeatModeTransformer

+ (Class)transformedValueClass { return [NSNumber class]; }
+ (BOOL)allowsReverseTransformation { return YES; }

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
	DLog(@"Transforming value: %@", value);
	
    if (value == nil) return nil;
	
	RepeatMode mode = [value integerValue];
	
	if (repeatMode == mode) {
		return [NSNumber numberWithBool:YES];
	}
	

	return [NSNumber numberWithBool:NO];
}

- (id)reverseTransformedValue:(id)value {
    if (value == nil) return nil;
	
	BOOL enabled = [value boolValue];
	if (enabled) {
		return [NSNumber numberWithInt:repeatMode];
	}
	else if(repeatMode == RepeatNone) {
		return [NSNumber numberWithInt:RepeatAll];
	}
	else {
		return [NSNumber numberWithInt:RepeatNone];
	}
}

@end

@implementation RepeatModeImageTransformer

+ (Class)transformedValueClass { return [NSImage class]; }
+ (BOOL)allowsReverseTransformation { return NO; }

// Convert from string to RepeatMode
- (id)transformedValue:(id)value {
	DLog(@"Transforming value: %@", value);
	
    if (value == nil) return nil;

	RepeatMode mode = [value integerValue];
	
	if (mode == RepeatNone) {
		return [NSImage imageNamed:@"repeatModeOffTemplate"];
	}
	else if (mode == RepeatOne) {
		return [NSImage imageNamed:@"repeatModeOneTemplate"];
	}
	else if (mode == RepeatAlbum) {
		return [NSImage imageNamed:@"repeatModeAlbumTemplate"];
	}
	else if (mode == RepeatAll) {
		return [NSImage imageNamed:@"repeatModeAllTemplate"];
	}

	return nil;
}

@end
