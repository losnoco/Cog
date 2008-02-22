//
//  StatusImageTransformer.m
//  Cog
//
//  Created by Vincent Spader on 2/21/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "StatusImageTransformer.h"
#import "PlaylistEntry.h"


@implementation StatusImageTransformer

+ (Class)transformedValueClass { return [NSImage class]; }
+ (BOOL)allowsReverseTransformation { return NO; }

// Convert from string to RepeatMode
- (id)transformedValue:(id)value {
	NSLog(@"Transforming value: %@", value);
	
    if (value == nil) return nil;

	PlaylistEntryStatus status = [value integerValue];
	NSLog(@"STATUS IS %i", status);
	if (status == kCogEntryPlaying) {
		return [NSImage imageNamed:@"play"];
	}
	else if (status == kCogEntryQueued) {
		return [NSImage imageNamed:@"NSAddTemplate"];
	}
	else if (status == kCogEntryError) {
		return [NSImage imageNamed:@"NSStopProgressTemplate"];
	}

	return nil;
}


@end
