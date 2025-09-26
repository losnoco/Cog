//
//  SCView.h
//  Cog
//
//  Created by Christopher Snowhill on 9/25/25.
//

#ifndef SCView_h
#define SCView_h

NS_ASSUME_NONNULL_BEGIN

@interface SCView : MTKView<MTKViewDelegate>
@property(nonatomic) BOOL isListening;

- (void)startPlayback;
@end

NS_ASSUME_NONNULL_END

#endif /* SCView_h */
