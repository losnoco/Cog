//
//  SpectrumView.h
//  Cog
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import <Cocoa/Cocoa.h>

#import <SceneKit/SceneKit.h>

#import "VisualizationController.h"

NS_ASSUME_NONNULL_BEGIN

@interface SpectrumView : SCNView
@property(nonatomic) BOOL isListening;
@end

NS_ASSUME_NONNULL_END
