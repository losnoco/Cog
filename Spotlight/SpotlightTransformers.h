//
//  SpotlightTransformers.h
//  Cog
//
//  Created by Matthew Grinshpun on 16/02/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
@class SpotlightWindowController;

@interface StringToURLTransformer: NSValueTransformer {}
@end

@interface PausingQueryTransformer: NSValueTransformer {
    NSArray *oldResults;
}

@property(copy) NSArray *oldResults;
@end