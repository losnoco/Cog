//
//  OpusDecoder.m
//  Opus
//
//  Created by Christopher Snowhill on 10/4/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "OpusDecoder.h"


@implementation OpusFile

int sourceRead(void *_stream, unsigned char *_ptr, int _nbytes)
{
	id source = (id)_stream;

	return [source read:_ptr amount:_nbytes];
}

int sourceSeek(void *_stream, opus_int64 _offset, int _whence)
{
	id source = (id)_stream;
	return ([source seek:_offset whence:_whence] ? 0 : -1);
}

int sourceClose(void *_stream)
{
	id source = (id)_stream;
	[source close];

	return 0;
}

opus_int64 sourceTell(void *_stream)
{
	id source = (id)_stream;

	return [source tell];
}

- (id)init
{
    self = [super init];
    if (self)
    {
        opusRef = NULL;
    }
    return self;
}

- (BOOL)open:(id<CogSource>)s
{
	source = [s retain];
	
	OpusFileCallbacks callbacks = {
		.read =  sourceRead,
		.seek =  sourceSeek,
		.close =  sourceClose,
		.tell =  sourceTell
	};
    
    int error;
    opusRef = op_open_callbacks(source, &callbacks, NULL, 0, &error);
	
	if (!opusRef)
	{
		NSLog(@"FAILED TO OPEN VORBIS FILE");
		return NO;
	}

    currentSection = lastSection = op_current_link( opusRef );
    
	bitrate = (op_bitrate(opusRef, currentSection )/1000.0);
	channels = op_channel_count( opusRef, currentSection );
	
	seekable = op_seekable(opusRef);
	
	totalFrames = op_pcm_total(opusRef, -1);

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];
	
	return YES;
}

- (int)readAudio:(void *)buf frames:(UInt32)frames
{
	int numread;
	int total = 0;
	
	if (currentSection != lastSection) {
		bitrate = (op_bitrate(opusRef, currentSection)/1000.0);
		channels = op_channel_count(opusRef, currentSection);
		
		[self willChangeValueForKey:@"properties"];
		[self didChangeValueForKey:@"properties"];
	}
	
    int size = frames * sizeof(int16_t);
    
    do {
		lastSection = currentSection;
        numread = op_read_stereo( opusRef, &((int16_t *)buf)[total], size - total);
        currentSection = op_current_link( opusRef );
		if (numread > 0) {
			total += numread * sizeof(int16_t);
		}
	
		if (currentSection != lastSection) {
			break;
		}
		
    } while (total != frames && numread != 0);

    return total/sizeof(int16_t);
}

- (void)close
{
    op_free(opusRef);
    opusRef = NULL;
	
	[source close];
	[source release];
}

- (long)seek:(long)frame
{
    op_pcm_seek(opusRef, frame);
	
	return frame;
}

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:channels], @"channels",
		[NSNumber numberWithInt:16], @"bitsPerSample",
		[NSNumber numberWithFloat:48000], @"sampleRate",
		[NSNumber numberWithDouble:totalFrames], @"totalFrames",
		[NSNumber numberWithInt:bitrate], @"bitrate",
		[NSNumber numberWithBool:([source seekable] && seekable)], @"seekable",
		nil];
}


+ (NSArray *)fileTypes
{
	return [NSArray arrayWithObjects:@"opus",nil];
}

+ (NSArray *)mimeTypes
{
	return [NSArray arrayWithObjects:@"audio/x-opus+ogg", nil];
}

@end
