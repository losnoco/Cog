//
//  SpotlightTransformers.m
//  Cog
//
//  Created by Matthew Grinshpun on 11/02/08.
//  Copyright 2008 Matthew Leon Grinshpun. All rights reserved.
//

#import "SpotlightTransformers.h"
#import "SpotlightWindowController.h"

@implementation StringToURLTransformer
+ (Class)transformedValueClass { return [NSURL class]; }
+ (BOOL)allowsReverseTransformation { return YES; }

// Convert from string to NSURL
- (id)transformedValue:(id)value {
    if (value == nil) return nil;

    return [NSURL URLWithString:value];
}

// Convert from NSURL to string
- (id)reverseTransformedValue:(id)value {
    if (value == nil) return nil;
    
    return [value absoluteString];
}
@end

@implementation PausingQueryTransformer
+ (Class)transformedValueClass { return [NSArray class]; }
+ (BOOL)allowsReverseTransformation { return NO; }

// Convert from string to NSURL
- (id)transformedValue:(id)value {
    if([(NSArray *)value count] > 0) {
        self.oldResults = (NSArray *)value;
    }
    return self.oldResults;
}

@synthesize oldResults;

@end