//
//  SpectrumViewSK.h
//  Cog
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import <Cocoa/Cocoa.h>

#import <SceneKit/SceneKit.h>

#import "VisualizationController.h"

NS_ASSUME_NONNULL_BEGIN

@interface SpectrumViewSK : SCNView
@property(nonatomic) BOOL isListening;
@property(nonatomic) BOOL isWorking;

+ (SpectrumViewSK *)createGuardWithFrame:(NSRect)frame;

- (void)enableCameraControl;
- (void)startPlayback;
@end

NS_ASSUME_NONNULL_END
