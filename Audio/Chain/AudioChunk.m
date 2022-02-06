//
//  AudioChunk.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/5/22.
//

#import "AudioChunk.h"

@implementation AudioChunk

- (id) init {
    self = [super init];
    
    if (self) {
        chunkData = [[NSMutableData alloc] init];
        formatAssigned = NO;
        lossless = NO;
    }
    
    return self;
}

@synthesize lossless;

- (AudioStreamBasicDescription) format {
    return format;
}

- (void) setFormat:(AudioStreamBasicDescription)informat {
    formatAssigned = YES;
    format = informat;
}

- (void) assignSamples:(const void *)data frameCount:(size_t)count {
    if (formatAssigned) {
        const size_t bytesPerPacket = format.mBytesPerPacket;
        [chunkData appendBytes:data length:bytesPerPacket * count];
    }
}

- (NSData *) removeSamples:(size_t)frameCount {
    if (formatAssigned) {
        const size_t bytesPerPacket = format.mBytesPerPacket;
        const size_t byteCount = bytesPerPacket * frameCount;
        NSData * ret = [chunkData subdataWithRange:NSMakeRange(0, byteCount)];
        [chunkData replaceBytesInRange:NSMakeRange(0, byteCount) withBytes:NULL length:0];
        return ret;
    }
    return [NSData data];
}

- (BOOL) isEmpty {
    return [chunkData length] == 0;
}

- (size_t) frameCount {
    if (formatAssigned) {
        const size_t bytesPerPacket = format.mBytesPerPacket;
        return [chunkData length] / bytesPerPacket;
    }
    return 0;
}

- (double) duration {
    if (formatAssigned) {
        const size_t bytesPerPacket = format.mBytesPerPacket;
        const double sampleRate = format.mSampleRate;
        return (double)([chunkData length] / bytesPerPacket) / sampleRate;
    }
    return 0.0;
}

@end
