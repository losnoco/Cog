//
//  FFMPEGDecoder.h
//  FFMPEG
//
//  Created by Andre Reffhaug on 2/26/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "Plugin.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

extern int ffmpeg_read(void *opaque, uint8_t *buf, int buf_size);
extern int ffmpeg_write(void *opaque, uint8_t *buf, int buf_size);
int64_t ffmpeg_seek(void *opaque, int64_t offset, int whence);

@interface FFMPEGDecoder : NSObject <CogDecoder> {
	id<CogSource> source;
	BOOL seekable;
	int channels;
	uint32_t channelConfig;
	int bitsPerSample;
	BOOL floatingPoint;
	BOOL lossy;
	float frequency;
	long totalFrames;
	long framesRead;
	int bitrate;
	int subsong;
	int64_t startTime;
	int64_t endTime;

	@private
	unsigned char *buffer;
	AVIOContext *ioCtx;
	int streamIndex;
	AVFormatContext *formatCtx;
	AVCodecContext *codecCtx;
	AVFrame *lastDecodedFrame;
	AVPacket *lastReadPacket;

	BOOL metadataUpdated;

	int prebufferedAudio;
	uint8_t *prebufferedAudioData;

	BOOL rawDSD;
	BOOL rawDSDReverseBits;
	BOOL rawDSDPlanar;

	int bytesConsumedFromDecodedFrame;
	BOOL readNextPacket;
	int64_t seekFrame;
	int64_t skipSamples;
	BOOL endOfStream;
	BOOL endOfAudio;

	int metadataIndex;
	NSString *artist;
	NSString *albumartist;
	NSString *album;
	NSString *title;
	NSString *genre;
	NSNumber *year;
	NSNumber *track;
	NSNumber *disc;
	float replayGainAlbumGain;
	float replayGainAlbumPeak;
	float replayGainTrackGain;
	float replayGainTrackPeak;

	int attachedPicIndex;
	NSData *albumArt;

	NSDictionary *id3Metadata;
}

@end
