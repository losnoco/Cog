//
//  ConverterNode.h
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/2/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <CoreAudio/AudioHardware.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AudioUnit/AudioUnit.h>

#import <audio/audio_resampler.h>

#import "Node.h"
#import "RefillNode.h"

@interface ConverterNode : Node {
    NSDictionary * rgInfo;
    
    void *resampler_data;
    const retro_resampler_t *resampler;

    void *inputBuffer;
    size_t inputBufferSize;
    size_t inpSize, inpOffset;
    
    BOOL stopping;
    BOOL convertEntered;
    BOOL paused;
    BOOL outputFormatChanged;
    
    BOOL skipResampler;
    
    ssize_t latencyStarted;
    size_t latencyEaten;
    BOOL latencyPostfill;
    
    double sampleRatio;
    
    float volumeScale;
    
    void *floatBuffer;
    size_t floatBufferSize;
    size_t floatSize, floatOffset;
    
    size_t floatConvertedSize;
    float floatConvertedLast[CHUNK_SIZE];
    
    void *extrapolateBuffer;
    size_t extrapolateBufferSize;
    
    void **dsd2pcm;
    size_t dsd2pcmCount;
    int dsd2pcmLatency;
	
	AudioStreamBasicDescription inputFormat;
    AudioStreamBasicDescription floatFormat;
    AudioStreamBasicDescription dmFloatFormat; // downmixed/upmixed float format
	AudioStreamBasicDescription outputFormat;
    
    AudioStreamBasicDescription previousOutputFormat;
    AudioStreamBasicDescription rememberedInputFormat;
    RefillNode *refillNode;
    id __weak originalPreviousNode;
}

- (id)initWithController:(id)c previous:(id)p;

- (BOOL)setupWithInputFormat:(AudioStreamBasicDescription)inputFormat outputFormat:(AudioStreamBasicDescription)outputFormat;
- (void)cleanUp;

- (void)process;
- (int)convert:(void *)dest amount:(int)amount;

- (void)setRGInfo:(NSDictionary *)rgi;

- (void)setOutputFormat:(AudioStreamBasicDescription)format;

- (void)inputFormatDidChange:(AudioStreamBasicDescription)format;

- (void)refreshVolumeScaling;

@end
