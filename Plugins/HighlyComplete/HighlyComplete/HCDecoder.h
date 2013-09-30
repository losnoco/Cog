//
//  HCDecoder.h
//  HighlyComplete
//
//  Created by Christopher Snowhill on 9/30/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "Plugin.h"

@interface HCDecoder : NSObject<CogDecoder,CogMetadataReader> {
	id<CogSource> currentSource;
    uint8_t *emulatorCore;
    void *emulatorExtra;
    
    NSDictionary *metadataList;
    
    int tagLengthMs;
    int tagFadeMs;
    
    int volumeScale;
	
	int type;
    
    int sampleRate;
	
	long totalFrames;
	long framesRead;
}

@end
