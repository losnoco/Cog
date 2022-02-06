//
//  AudioChunk.h
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/5/22.
//

#ifndef AudioChunk_h
#define AudioChunk_h

#import <Foundation/Foundation.h>
#import <CoreAudio/CoreAudio.h>

NS_ASSUME_NONNULL_BEGIN

@interface AudioChunk : NSObject {
    AudioStreamBasicDescription format;
    NSMutableData * chunkData;
    BOOL formatAssigned;
    BOOL lossless;
}

@property AudioStreamBasicDescription format;
@property BOOL lossless;

- (id) init;

- (void) assignSamples:(const void *)data frameCount:(size_t)count;

- (NSData *) removeSamples:(size_t)frameCount;

- (BOOL) isEmpty;

- (size_t) frameCount;

- (double) duration;

@end

NS_ASSUME_NONNULL_END

#endif /* AudioChunk_h */
