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

#import "Node.h"

@interface ConverterNode : Node {
    NSDictionary * rgInfo;

	AudioConverterRef converter;
    AudioConverterRef converterFloat;
	void *callbackBuffer;
    size_t callbackBufferSize;
    
    BOOL stopping;
    BOOL convertEntered;
    BOOL ACInputEntered;
    BOOL ACFloatEntered;
    
    float sampleRatio;
    
    float volumeScale;
    
    void *floatBuffer;
    size_t floatBufferSize;
    int floatSize, floatOffset;
	
	AudioStreamBasicDescription inputFormat;
    AudioStreamBasicDescription floatFormat;
    AudioStreamBasicDescription dmFloatFormat; // downmixed float format
	AudioStreamBasicDescription outputFormat;
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
