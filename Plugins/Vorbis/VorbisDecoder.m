//
//  VorbisFile.m
//  zyVorbis
//
//  Created by Vincent Spader on 1/22/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "VorbisDecoder.h"

#import "Logging.h"

@implementation VorbisDecoder

static const int MAXCHANNELS = 8;
static const int chmap[MAXCHANNELS][MAXCHANNELS] = {
	{
	0,
	}, // mono
	{
	0,
	1,
	}, // l, r
	{
	0,
	2,
	1,
	}, // l, c, r -> l, r, c
	{
	0,
	1,
	2,
	3,
	}, // l, r, bl, br
	{
	0,
	2,
	1,
	3,
	4,
	}, // l, c, r, bl, br -> l, r, c, bl, br
	{ 0, 2, 1, 5, 3, 4 }, // l, c, r, bl, br, lfe -> l, r, c, lfe, bl, br
	{ 0, 2, 1, 6, 5, 3, 4 }, // l, c, r, sl, sr, bc, lfe -> l, r, c, lfe, bc, sl, sr
	{ 0, 2, 1, 7, 5, 6, 3, 4 } // l, c, r, sl, sr, bl, br, lfe -> l, r, c, lfe, bl, br, sl, sr
};

size_t sourceRead(void *buf, size_t size, size_t nmemb, void *datasource) {
	id source = (__bridge id)datasource;

	return [source read:buf amount:(size * nmemb)];
}

int sourceSeek(void *datasource, ogg_int64_t offset, int whence) {
	id source = (__bridge id)datasource;
	return ([source seek:offset whence:whence] ? 0 : -1);
}

int sourceClose(void *datasource) {
	return 0;
}

long sourceTell(void *datasource) {
	id source = (__bridge id)datasource;

	return [source tell];
}

- (BOOL)open:(id<CogSource>)s {
	source = s;

	ov_callbacks callbacks = {
		.read_func = sourceRead,
		.seek_func = sourceSeek,
		.close_func = sourceClose,
		.tell_func = sourceTell
	};

	if(ov_open_callbacks((__bridge void *)(source), &vorbisRef, NULL, 0, callbacks) != 0) {
		DLog(@"FAILED TO OPEN VORBIS FILE");
		return NO;
	}

	vorbis_info *vi;

	vi = ov_info(&vorbisRef, -1);

	bitrate = (vi->bitrate_nominal / 1000.0);
	channels = vi->channels;
	frequency = vi->rate;

	seekable = ov_seekable(&vorbisRef);

	totalFrames = ov_pcm_total(&vorbisRef, -1);

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (int)readAudio:(void *)buf frames:(UInt32)frames {
	int numread;
	int total = 0;

	if(currentSection != lastSection) {
		vorbis_info *vi;
		vi = ov_info(&vorbisRef, -1);

		bitrate = (vi->bitrate_nominal / 1000.0);
		channels = vi->channels;
		frequency = vi->rate;

		[self willChangeValueForKey:@"properties"];
		[self didChangeValueForKey:@"properties"];
	}

	do {
		lastSection = currentSection;
		float **pcm;
		numread = (int)ov_read_float(&vorbisRef, &pcm, frames - total, &currentSection);
		if(numread > 0) {
			if(channels <= MAXCHANNELS) {
				for(int i = 0; i < channels; i++) {
					for(int j = 0; j < numread; j++) {
						((float *)buf)[(total + j) * channels + i] = pcm[chmap[channels - 1][i]][j];
					}
				}
			} else {
				for(int i = 0; i < channels; i++) {
					for(int j = 0; j < numread; j++) {
						((float *)buf)[(total + j) * channels + i] = pcm[i][j];
					}
				}
			}
			total += numread;
		}

		if(currentSection != lastSection) {
			break;
		}

	} while(total != frames && numread != 0);

	return total;
}

- (void)close {
	ov_clear(&vorbisRef);
}

- (void)dealloc {
	[self close];
}

- (long)seek:(long)frame {
	ov_pcm_seek(&vorbisRef, frame);

	return frame;
}

- (NSDictionary *)properties {
	return @{@"channels": [NSNumber numberWithInt:channels],
			 @"bitsPerSample": [NSNumber numberWithInt:32],
			 @"floatingPoint": [NSNumber numberWithBool:YES],
			 @"sampleRate": [NSNumber numberWithFloat:frequency],
			 @"totalFrames": [NSNumber numberWithDouble:totalFrames],
			 @"bitrate": [NSNumber numberWithInt:bitrate],
			 @"seekable": [NSNumber numberWithBool:([source seekable] && seekable)],
			 @"codec": @"Ogg Vorbis",
			 @"endian": @"host",
			 @"encoding": @"lossy"};
}

- (NSDictionary *)metadata {
	return @{};
}

+ (NSArray *)fileTypes {
	return @[@"ogg"];
}

+ (NSArray *)mimeTypes {
	return @[@"application/ogg", @"application/x-ogg", @"audio/x-vorbis+ogg"];
}

+ (float)priority {
	return 1.0;
}

+ (NSArray *)fileTypeAssociations {
	return @[
		@[@"Ogg Vorbis File", @"ogg.icns", @"ogg"]
	];
}

@end
