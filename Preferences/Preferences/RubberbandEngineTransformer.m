//
//  EngineTransformer.m
//  Preferences
//
//  Created by Christopher Snowhill on 2/11/25.
//

#import <Foundation/Foundation.h>

#import "RubberbandEngineTransformer.h"

@implementation RubberbandEngineTransformer
+ (Class)transformedValueClass {
	return [NSNumber class];
}
+ (BOOL)allowsReverseTransformation {
	return NO;
}

- (id)transformedValue:(id)value {
	if(value == nil) return @(YES);
	
	if([value isKindOfClass:[NSString class]]) {
		NSString *stringValue = value;
		if([stringValue isEqualToString:@"finer"]) {
			return @(NO);
		}
	}
	
	return @(YES);
}

@end




















