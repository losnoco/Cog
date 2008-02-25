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

@synthesize playImage;
@synthesize queueImage;
@synthesize errorImage;
@synthesize stopAfterCurrentImage;

+ (Class)transformedValueClass { return [NSImage class]; }
+ (BOOL)allowsReverseTransformation { return NO; }

- (id)init
{
	self = [super init];
	if (self)
	{
		self.playImage = [NSImage imageNamed:@"play"];
		self.queueImage = [NSImage imageNamed:@"NSAddTemplate"];
		self.errorImage = [NSImage imageNamed:@"NSStopProgressTemplate"];
		self.stopAfterCurrentImage = [NSImage imageNamed:@"stop_current"];
	}
	
	return self;
}

// Convert from string to RepeatMode
- (id)transformedValue:(id)value {
    if (value == nil) return nil;

	PlaylistEntryStatus status = [value integerValue];
	if (status == kCogEntryPlaying) {
		return self.playImage;
	}
	else if (status == kCogEntryQueued) {
		return self.queueImage;
	}
	else if (status == kCogEntryError) {
		return self.errorImage;
	}
	else if (status == kCogEntryStoppingAfterCurrent) {
		return self.stopAfterCurrentImage;
	}

	return nil;
}


@end
