//
//  ColorToValueTransformer.m
//  Preferences
//
//  Created by Christopher Snowhill on 5/22/22.
//

#import "ColorToValueTransformer.h"

@implementation ColorToValueTransformer

+ (Class)transformedValueClass {
	return [NSColor class];
}
+ (BOOL)allowsReverseTransformation {
	return YES;
}

// Convert from string to NSURL
- (id)reverseTransformedValue:(id)value {
	if(value == nil) return nil;

	NSError *error;
	NSData *data = [NSKeyedArchiver archivedDataWithRootObject:value
	                                     requiringSecureCoding:YES
	                                                     error:&error];

	return data;
}

- (id)transformedValue:(id)value {
	if(value == nil) return [NSColor colorWithRed:0 green:0 blue:0 alpha:1.0];

	NSError *error;
	NSColor *color = nil;

	if(@available(macOS 11.0, *)) {
		color = (NSColor *)[NSKeyedUnarchiver unarchivedArrayOfObjectsOfClass:[NSColor class]
		                                                             fromData:value
		                                                                error:&error];
	} else {
		NSSet *allowed = [NSSet setWithArray:@[[NSColor class]]];
		color = (NSColor *)[NSKeyedUnarchiver unarchivedObjectOfClasses:allowed
		                                                       fromData:value
		                                                          error:&error];
	}

	return color;
}

@end
