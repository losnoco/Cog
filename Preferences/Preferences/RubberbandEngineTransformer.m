//
//  EngineTransformer.m
//  Preferences
//
//  Created by Christopher Snowhill on 2/11/25.
//

#import <Foundation/Foundation.h>

#import "RubberbandEngineTransformer.h"

@implementation RubberbandEngineR3Transformer
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
		if([stringValue isEqualToString:@"disabled"] ||
		   [stringValue isEqualToString:@"finer"]) {
			return @(NO);
		}
	}

	return @(YES);
}

@end

@implementation RubberbandEngineEnabledTransformer
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
		if([stringValue isEqualToString:@"disabled"]) {
			return @(NO);
		}
	}

	return @(YES);
}

@end

@implementation RubberbandEngineHiddenTransformer
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
		if([stringValue isEqualToString:@"disabled"]) {
			return @(NO);
		}
	}

	return @(YES);
}

@end
