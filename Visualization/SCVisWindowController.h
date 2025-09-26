//
//  SCVisWindowController.h
//  Cog
//
//  Created by Christopher Snowhill on 9/25/25.
//

#ifndef SCVisWindowController_h
#define SCVisWindowController_h

#import "PlaybackController.h"

NS_ASSUME_NONNULL_BEGIN

@interface SCVisWindowController : NSWindowController <NSWindowDelegate> {
	IBOutlet PlaybackController *playbackController;
}

@end

NS_ASSUME_NONNULL_END

#endif /* SCVisWindowController_h */
