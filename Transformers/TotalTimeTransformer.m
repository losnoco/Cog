//
//  TotalTimeTransformer.m
//  Cog
//
//  Created by Christopher Snowhill on 6/15/22.
//

#import "TotalTimeTransformer.h"

@implementation TotalTimeTransformer

+ (Class)transformedValueClass {
	return [NSString class];
}
+ (BOOL)allowsReverseTransformation {
	return NO;
}

- (id)transformedValue:(id)value {
	if(value == nil) return @"";

	if([value isKindOfClass:[NSString class]]) {
		NSString *string = (NSString *)value;
		if([string length] > 0) {
			return [@"Total Duration: " stringByAppendingString:string];
		}
	}

	return @"";
}

@end
