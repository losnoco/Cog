//
//  SpectrumViewCG.h
//  Cog
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import <Cocoa/Cocoa.h>

#import <CogAudio/VisualizationController.h>

NS_ASSUME_NONNULL_BEGIN

@interface SpectrumViewCG : NSView
@property(nonatomic) BOOL isListening;

- (id)initWithFrame:(NSRect)frame;

- (void)startPlayback;
- (void)enableFullView;
@end

NS_ASSUME_NONNULL_END
