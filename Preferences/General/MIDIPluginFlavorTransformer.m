//
//  PathToFileTransformer.m
//  General
//
//  Created by Christopher Snowhill on 11/29/16.
//
//

#import "MIDIPluginFlavorTransformer.h"

@implementation MIDIPluginFlavorTransformer

+ (Class)transformedValueClass { return [NSNumber class]; }
+ (BOOL)allowsReverseTransformation { return NO; }

// Convert from string to NSURL
- (id)transformedValue:(id)value {
    if (value == nil) return nil;
    
    return [NSNumber numberWithBool:[value isEqualToString:@"Sc55rolD"]];
}
@end
