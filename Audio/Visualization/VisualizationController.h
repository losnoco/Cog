//
//  VisualizationController.h
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface VisualizationController : NSObject {
	float visAudio[512];
}

+ (VisualizationController *)sharedController;

- (void)postVisPCM:(const float *)inPCM;
- (void)copyVisPCM:(float *)outPCM visFFT:(float *)outFFT;

@end

NS_ASSUME_NONNULL_END
