//
//  FontSizetoLineHeightTransformer.m
//  Cog
//
//  Created by Matthew Grinshpun on 18/02/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "FontSizetoLineHeightTransformer.h"

@implementation FontSizetoLineHeightTransformer

+ (Class)transformedValueClass {
	return [NSNumber class];
}
+ (BOOL)allowsReverseTransformation {
	return NO;
}

// Convert from font size to height in playlist view
- (id)transformedValue:(id)value {
	NSFont *font = [NSFont systemFontOfSize:[(NSNumber *)value floatValue]];
	NSLayoutManager *layoutManager = [NSLayoutManager new];
	float fRowSize = [layoutManager defaultLineHeightForFont:font];
	return @(fRowSize);
}

@end
