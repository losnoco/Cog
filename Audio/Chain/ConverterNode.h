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

#import <soxr.h>

#import "Node.h"
#import "RefillNode.h"

#import "HeadphoneFilter.h"

@interface ConverterNode : Node {
    NSDictionary * rgInfo;
    
    soxr_t soxr;

    void *inputBuffer;
    size_t inputBufferSize;
    size_t inpSize, inpOffset;
    
    BOOL stopping;
    BOOL convertEntered;
    BOOL paused;
    BOOL outputFormatChanged;
    
    BOOL skipResampler;
    
    unsigned int PRIME_LEN_;
    unsigned int N_samples_to_add_;
    unsigned int N_samples_to_drop_;
    
    unsigned int is_preextrapolated_;
    unsigned int is_postextrapolated_;

    int latencyEaten;
    int latencyEatenPost;
    
    double sampleRatio;
    
    float volumeScale;
    
    void *floatBuffer;
    size_t floatBufferSize;
    size_t floatSize, floatOffset;
    
    void *extrapolateBuffer;
    size_t extrapolateBufferSize;
    
    void **dsd2pcm;
    size_t dsd2pcmCount;
    int dsd2pcmLatency;
    int dsdLatencyEaten;
    
    BOOL rememberedLossless;
	
	AudioStreamBasicDescription inputFormat;
    AudioStreamBasicDescription floatFormat;
    AudioStreamBasicDescription dmFloatFormat; // downmixed/upmixed float format
	AudioStreamBasicDescription outputFormat;
    
    AudioStreamBasicDescription previousOutputFormat;
    AudioStreamBasicDescription rememberedInputFormat;
    RefillNode *refillNode;
    id __weak originalPreviousNode;
    
    void *hdcd_decoder;
    
    HeadphoneFilter *hFilter;
}

@property AudioStreamBasicDescription inputFormat;

- (id)initWithController:(id)c previous:(id)p;

- (BOOL)setupWithInputFormat:(AudioStreamBasicDescription)inputFormat outputFormat:(AudioStreamBasicDescription)outputFormat isLossless:(BOOL)lossless;
- (void)cleanUp;

- (void)process;
- (int)convert:(void *)dest amount:(int)amount;

- (void)setRGInfo:(NSDictionary *)rgi;

- (void)setOutputFormat:(AudioStreamBasicDescription)format;

- (void)inputFormatDidChange:(AudioStreamBasicDescription)format;

- (void)refreshVolumeScaling;

@end
