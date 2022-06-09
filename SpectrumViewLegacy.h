//
//  SpectrumViewLegacy.h
//  Cog
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import <Cocoa/Cocoa.h>

#import "VisualizationController.h"

NS_ASSUME_NONNULL_BEGIN

@interface SpectrumViewLegacy : NSView
@property(nonatomic) BOOL isListening;

- (void)startPlayback;
@end

NS_ASSUME_NONNULL_END
