//
//  SpectrumWindowController.h
//  Cog
//
//  Created by Christopher Snowhill on 5/22/22.
//

#import <Cocoa/Cocoa.h>

#import "PlaybackController.h"

NS_ASSUME_NONNULL_BEGIN

@interface SpectrumWindowController : NSWindowController <NSWindowDelegate> {
	IBOutlet PlaybackController *playbackController;
}

@end

NS_ASSUME_NONNULL_END
