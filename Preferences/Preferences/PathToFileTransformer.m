//
//  PathToFileTransformer.m
//  General
//
//  Created by Christopher Snowhill on 10/17/13.
//
//

#import "PathToFileTransformer.h"

@implementation PathToFileTransformer

+ (Class)transformedValueClass {
	return [NSString class];
}
+ (BOOL)allowsReverseTransformation {
	return NO;
}

// Convert from string to NSURL
- (id)transformedValue:(id)value {
	if(value == nil) return nil;

	return [value lastPathComponent];
}
@end
