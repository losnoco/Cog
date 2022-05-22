//
//  VisualizationController.h
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface VisualizationController : NSObject {
	double sampleRate;
	float visAudio[4096];
}

+ (VisualizationController *)sharedController;

- (void)postSampleRate:(double)sampleRate;
- (void)postVisPCM:(const float *)inPCM amount:(int)amount;
- (double)readSampleRate;
- (void)copyVisPCM:(float *)outPCM visCQT:(float *)outCQT;

@end

NS_ASSUME_NONNULL_END
