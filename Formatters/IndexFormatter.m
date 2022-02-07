//
//  IndexFormatter.m
//  Cog
//
//  Created by Zaphod Beeblebrox on 3/13/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "IndexFormatter.h"

@implementation IndexFormatter

- (NSString *)stringForObjectValue:(id)object {
	NSString *result = nil;
	int value;

	if(nil == object || NO == [object isKindOfClass:[NSNumber class]]) {
		return nil;
	}

	value = ([object intValue] + 1);

	result = [NSString stringWithFormat:@"%i", value];

	return result;
}

- (BOOL)getObjectValue:(id *)object forString:(NSString *)string errorDescription:(NSString **)error {
	if(NULL != object) {
		*object = [NSNumber numberWithInt:[string intValue]];

		return YES;
	}

	return NO;
}

- (NSAttributedString *)attributedStringForObjectValue:(id)object withDefaultAttributes:(NSDictionary *)attributes {
	NSAttributedString *result = nil;

	result = [[NSAttributedString alloc] initWithString:[self stringForObjectValue:object] attributes:attributes];
	return result;
}

@end
