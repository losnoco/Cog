//
//  ShuffleTransformers.h
//  Cog
//
//  Created by Vincent Spader on 2/27/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "PlaylistController.h"
#import <Cocoa/Cocoa.h>

@interface ShuffleImageTransformer : NSValueTransformer {
}
@end

@interface ShuffleModeTransformer : NSValueTransformer {
	ShuffleMode shuffleMode;
}

- (id)initWithMode:(ShuffleMode)s;

@end
