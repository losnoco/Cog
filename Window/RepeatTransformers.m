//
//  RepeatModeTransformer.m
//  Cog
//
//  Created by Vincent Spader on 2/18/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "RepeatTransformers.h"

#import "Logging.h"

@implementation RepeatModeTransformer {
    RepeatMode repeatMode;
}

+ (Class)transformedValueClass { return [NSNumber class]; }
+ (BOOL)allowsReverseTransformation { return YES; }

- (id)initWithMode:(RepeatMode)r {
    self = [super init];
    if (self) {
        repeatMode = r;
    }

    return self;
}

// Convert from RepeatMode to BOOL
- (id)transformedValue:(id)value {
    DLog(@"Transforming value: %@", value);

    if (value == nil) return nil;

    RepeatMode mode = (RepeatMode) [value integerValue];

    return @(repeatMode == mode);
}

- (id)reverseTransformedValue:(id)value {
    if (value == nil) return nil;

    BOOL enabled = [value boolValue];
    if (enabled) {
        return @(repeatMode);
    } else if (repeatMode == RepeatModeNoRepeat) {
        return @(RepeatModeRepeatAll);
    } else {
        return @(RepeatModeNoRepeat);
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

    RepeatMode mode = (RepeatMode) [value integerValue];

    if (mode == RepeatModeNoRepeat) {
        return [NSImage imageNamed:@"repeatModeOffTemplate"];
    }
    else if (mode == RepeatModeRepeatOne) {
        return [NSImage imageNamed:@"repeatModeOneTemplate"];
    }
    else if (mode == RepeatModeRepeatAlbum) {
        return [NSImage imageNamed:@"repeatModeAlbumTemplate"];
    }
    else if (mode == RepeatModeRepeatAll) {
        return [NSImage imageNamed:@"repeatModeAllTemplate"];
    }

    return nil;
}

@end
