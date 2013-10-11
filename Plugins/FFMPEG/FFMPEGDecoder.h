//
//  FFMPEGDecoder.h
//  FFMPEG
//
//  Created by Andre Reffhaug on 2/26/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "Plugin.h"

#define __FRAMEWORK__
#import <FFMPEG/avcodec.h>
#import <FFMPEG/avformat.h>
#undef __FRAMEWORK__

@interface FFMPEGDecoder : NSObject <CogDecoder>
{
    BOOL seekable;
    int channels;
    int bitsPerSample;
    BOOL floatingPoint;
    float frequency;
    long totalFrames;
    long framesRead;
    int bitrate;
    
@private
    int streamIndex;
    AVFormatContext *formatCtx;
    AVCodecContext *codecCtx;
    AVFrame *lastDecodedFrame;
    AVPacket *lastReadPacket;
    int bytesConsumedFromDecodedFrame;
    int bytesReadFromPacket;
    BOOL readNextPacket;
}

@end
