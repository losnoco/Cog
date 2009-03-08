//
//  MissingAlbumArtTransformer.m
//  Cog
//
//  Created by Vincent Spader on 3/8/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "MissingAlbumArtTransformer.h"


@implementation MissingAlbumArtTransformer

+ (Class)transformedValueClass { return [NSImage class]; }
+ (BOOL)allowsReverseTransformation { return NO; }

// Convert from NSImage to NSImage
- (id)transformedValue:(id)value {
    if (value == nil) {
		return [NSImage imageNamed:@"missingArt"];
	}
	
	return value;
}

@end
