//
//  BadSampleCleaner.h
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/15/22.
//

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface BadSampleCleaner : NSObject
+ (void)cleanSamples:(float *)buffer amount:(NSUInteger)amount location:(NSString *)location;
@end

NS_ASSUME_NONNULL_END
