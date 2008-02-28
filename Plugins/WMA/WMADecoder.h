//
//  WMADecoder.h
//  WMA
//
//  Created by Andre Reffhaug on 2/26/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "Plugin.h"

#import "WMA/avcodec.h"
#import "WMA/avformat.h"

@interface WMADecoder : NSObject <CogDecoder> 
{
	id<CogSource> source;
	void *sampleBuffer;
	int numSamples;
	int samplePos;
	
	AVFormatContext *ic;
	AVCodecContext *c;
	AVCodec *codec;
	
	BOOL seekable;
	int bitsPerSample;
	int bitrate;
	int channels;
	float frequency;
	long totalFrames;
	
}

@end
