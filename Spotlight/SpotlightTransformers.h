//
//  SpotlightTransformers.h
//  Cog
//
//  Created by Matthew Grinshpun on 16/02/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
@class SpotlightWindowController;

@interface PausingQueryTransformer : NSValueTransformer {
	NSArray *oldResults;
}

+ (void)setSearchController:(SpotlightWindowController *)aSearchController;

@property(copy) NSArray *oldResults;
@end

@interface AuthorToArtistTransformer : NSValueTransformer {
}
@end

@interface PathToURLTransformer : NSValueTransformer {
}
@end

@interface StringToSearchScopeTransformer : NSValueTransformer {
}
@end

@interface NumberToStringTransformer : NSValueTransformer {
}
@end