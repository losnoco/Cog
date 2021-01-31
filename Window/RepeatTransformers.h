//
//  RepeatModeTransformer.h
//  Cog
//
//  Created by Vincent Spader on 2/18/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "PlaylistController.h"

@interface RepeatModeTransformer : NSValueTransformer

- (id)initWithMode:(RepeatMode) r;

@end

@interface RepeatModeImageTransformer : NSValueTransformer {}
@end

