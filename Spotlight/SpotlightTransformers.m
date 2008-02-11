//
//  SpotlightTransformers.m
//  Cog
//
//  Created by Matthew Grinshpun on 10/02/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SpotlightTransformers.h"

// kMDItemAuthor is an array of values. 
// For music, the first value is the artist
@implementation SpotlightArtistTransformer

+ (Class)transformedValueClass {
    return [NSString class];
}

- (id)transformedValue:(id)value {
    NSString * artistString = @"";
    if (value != nil) {
        artistString = [NSString stringWithString:
            (NSString *)[value objectAtIndex:0]];
    }
    return artistString;
}
@end
