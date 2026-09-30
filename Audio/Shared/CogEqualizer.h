//
//  CogEqualizer.h
//  CogAudio
//
//  Created by Christopher Snowhill on 2/11/25.
//

#ifndef CogEqualizer_h
#define CogEqualizer_h

#import <Foundation/Foundation.h>

/// The equalizer the EQ window drives: the engine's equalizer stage.
@protocol CogEqualizer <NSObject>
- (void)setBandGain:(float)gainDB forIndex:(int)i;
- (void)setAllBands:(float *_Nonnull)gainsDB;
- (void)setPreamp:(float)preampDB;
@end

#endif /* CogEqualizer_h */
