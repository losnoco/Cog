//
//  TimeIntervalToStringTransformer.m
//  Preferences
//
//  Created by Christopher Snowhill on 7/1/22.
//

#import "TimeIntervalToStringTransformer.h"

#import <math.h>

@implementation TimeIntervalToStringTransformer
+ (Class)transformedValueClass {
	return [NSString class];
}
+ (BOOL)allowsReverseTransformation {
	return YES;
}

// Convert from string to NSURL
- (id)reverseTransformedValue:(id)value {
	if(value == nil) return nil;

	if([value isKindOfClass:[NSString class]]) {
		NSString *theString = (NSString *)value;

		NSArray *components = [theString componentsSeparatedByString:@":"];

		double interval = 0.0;

		for(size_t i = 0, j = [components count]; i < j; ++i) {
			interval += ([components[j - i - 1] doubleValue]) * pow(60.0, i);
		}

		return @(interval);
	}

	return nil;
}

- (id)transformedValue:(id)value {
	if(value == nil) return @"";

	if([value isKindOfClass:[NSNumber class]]) {
		NSDateComponentsFormatter *formatter = [NSDateComponentsFormatter new];
		[formatter setAllowedUnits:NSCalendarUnitHour | NSCalendarUnitMinute | NSCalendarUnitSecond];
		double secondsValue = [value doubleValue];
		double fractionValue = fmod(secondsValue, 1.0);
		secondsValue = (double)(int)secondsValue;
		NSString *wholePart = [formatter stringFromTimeInterval:(NSTimeInterval)secondsValue];
		NSUInteger fractionMillis = (int)(fractionValue * 1000.0);
		if(fractionMillis >= 1) {
			NSString *fractionPart = [NSString stringWithFormat:@".%03lu", fractionMillis];
			fractionPart = [fractionPart stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithRange:NSMakeRange('0', 1)]];
			return [wholePart stringByAppendingString:fractionPart];
		}
		return wholePart;
	}

	return @"";
}

@end
