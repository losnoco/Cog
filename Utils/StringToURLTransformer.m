//
//  StringToURLTransformer.m
//  Cog
//
//  Created by Vincent Spader on 2/17/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "StringToURLTransformer.h"

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
