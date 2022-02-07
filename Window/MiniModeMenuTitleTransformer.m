//
// Created by UFO on 12/5/12.
//
#import "MiniModeMenuTitleTransformer.h"

@implementation MiniModeMenuTitleTransformer

+ (Class)transformedValueClass {
	return [NSString class];
}
+ (BOOL)allowsReverseTransformation {
	return NO;
}

- (id)transformedValue:(id)value {
	if([(NSNumber *)value boolValue]) {
		return NSLocalizedString(@"SwitchFromMiniPlayer", @"");
	} else {
		return NSLocalizedString(@"SwitchToMiniPlayer", @"");
	}
}

@end
