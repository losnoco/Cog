//
//  MaybeSecureValueDataTransformer.m
//  Cog
//
//  Created by Christopher Snowhill on 9/10/25.
//
//  This stupid wrapper exists because NSSecureUnarchiveFromDataTransformer requires
//  macOS 10.14, but the necessary elements to recreate it already exist on 10.13,
//  which is still our current minimum target. Beside which, this allows us to limit
//  the exact types of objects that are allowed to be decoded from the dictionary to
//  just those that are currently used for metadata storage.

#import "MaybeSecureValueDataTransformer.h"

@implementation MaybeSecureValueDataTransformer

+ (Class)transformedValueClass {
	return [NSData class];
}
+ (BOOL)allowsReverseTransformation {
	return YES;
}

// Convert a dictionary of metadata values into data
- (id)transformedValue:(id)value {
	if(value == nil) return nil;

	if(![value isKindOfClass:[NSDictionary class]]) return nil;

	NSError *error = nil;
	NSData *ret = [NSKeyedArchiver archivedDataWithRootObject:value requiringSecureCoding:YES error:&error];
	if(error) {
		NSLog(@"Error archiving secure data: %@", error);
		return nil;
	}
	return ret;
}

// Unarchive a dictionary of metadata values from data
- (id)reverseTransformedValue:(id)value {
	if(value == nil) return nil;

	if(![value isKindOfClass:[NSData class]]) return nil;

	NSError *error = nil;
	NSKeyedUnarchiver *unarchiver = [[NSKeyedUnarchiver alloc] initForReadingFromData:value error:&error];
	if(error) {
		NSLog(@"Error initializing unarchiver for data: %@", error);
		return nil;
	}
	unarchiver.requiresSecureCoding = YES;
	NSSet *classes = [NSSet setWithArray:@[[NSDictionary class], [NSArray class], [NSString class], [NSNumber class], [NSDate class]]];
	id ret = [unarchiver decodeObjectOfClasses:classes forKey:@"root"];
	if(!ret) {
		NSLog(@"Error decoding dictionary from data");
	}
	return ret;
}
@end
