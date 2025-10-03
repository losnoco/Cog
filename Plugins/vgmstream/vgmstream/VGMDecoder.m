//
//  VGMDecoder.m
//  vgmstream
//
//  Created by Christopher Snowhill on 02/25/14.
//  Copyright 2014 __NoWork, Inc__. All rights reserved.
//

#import "VGMDecoder.h"
#import "VGMInterface.h"

#import <libvgmstream/libvgmstream.h>

#import "PlaylistController.h"

#include <stdlib.h>

#define MAX_BUFFER_SAMPLES ((int)2048)

static NSString *get_description_tag(const char *description, const char *tag, char delimiter) {
	// extract a "tag" from the description string
	if(!delimiter) delimiter = '\n';
	const char *pos = strstr(description, tag);
	const char *eos = NULL;
	if(pos != NULL) {
		pos += strlen(tag);
		eos = strchr(pos, delimiter);
		if(eos == NULL) eos = pos + strlen(pos);
		char temp[eos - pos + 1];
		memcpy(temp, pos, eos - pos);
		temp[eos - pos] = '\0';
		return [NSString stringWithUTF8String:temp];
	}
	return nil;
}

@implementation VGMInfoCache

+ (id)sharedCache {
	static VGMInfoCache *sharedMyCache = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		sharedMyCache = [self new];
	});
	return sharedMyCache;
}

- (id)init {
	if(self = [super init]) {
		storage = [NSMutableDictionary new];
	}
	return self;
}

- (void)stuffURL:(NSURL *)url stream:(libvgmstream_t *)stream {
	int output_channels = stream->format->channels;

	int track_num = [[url fragment] intValue];

	int sampleRate = stream->format->sample_rate;
	int channels = output_channels;
	long totalFrames = stream->format->play_samples;

	int bitrate = stream->format->stream_bitrate;

	NSString *path = [url absoluteString];
	NSRange fragmentRange = [path rangeOfString:@"#" options:NSBackwardsSearch];
	if(fragmentRange.location != NSNotFound) {
		path = [path substringToIndex:fragmentRange.location];
	}

	NSURL *urlTrimmed = [NSURL fileURLWithPath:[path stringByRemovingPercentEncoding]];

	NSURL *folder = [urlTrimmed URLByDeletingLastPathComponent];
	NSURL *tagurl = [folder URLByAppendingPathComponent:@"!tags.m3u" isDirectory:NO];

	NSString *filename = [urlTrimmed lastPathComponent];

	NSString *album = @"";
	NSString *artist = @"";
	NSNumber *year = @(0);
	NSNumber *track = @(0);
	NSNumber *disc = @(0);
	NSString *title = @"";

	NSString *codec;

	NSNumber *rgTrackGain = @(0);
	NSNumber *rgTrackPeak = @(0);
	NSNumber *rgAlbumGain = @(0);
	NSNumber *rgAlbumPeak = @(0);

	codec = [NSString stringWithUTF8String:stream->format->codec_name];

	libstreamfile_t* sf_tags = open_vfs([[tagurl absoluteString] UTF8String]);
	if(sf_tags) {
		libvgmstream_tags_t* tags = NULL;

		tags = libvgmstream_tags_init(sf_tags);
		libvgmstream_tags_find(tags, [filename UTF8String]);
		while(libvgmstream_tags_next_tag(tags)) {
			const char* tag_key = tags->key;
			const char* tag_val = tags->val;
			NSString *value = guess_encoding_of_string(tag_val);
			if(!strncasecmp(tag_key, "REPLAYGAIN_", strlen("REPLAYGAIN_"))) {
				if(!strncasecmp(tag_key + strlen("REPLAYGAIN_"), "TRACK_", strlen("TRACK_"))) {
					if(!strcasecmp(tag_key + strlen("REPLAYGAIN_TRACK_"), "GAIN")) {
						rgTrackGain = @([value floatValue]);
					} else if(!strcasecmp(tag_key + strlen("REPLAYGAIN_TRACK_"), "PEAK")) {
						rgTrackPeak = @([value floatValue]);
					}
				} else if(!strncasecmp(tag_key + strlen("REPLAYGAIN_"), "ALBUM_", strlen("ALBUM_"))) {
					if(!strcasecmp(tag_key + strlen("REPLAYGAIN_ALBUM_"), "GAIN")) {
						rgAlbumGain = @([value floatValue]);
					} else if(!strcasecmp(tag_key + strlen("REPLAYGAIN_ALBUM_"), "PEAK")) {
						rgAlbumPeak = @([value floatValue]);
					}
				}
			} else if(!strcasecmp(tag_key, "ALBUM")) {
				album = value;
			} else if(!strcasecmp(tag_key, "ARTIST")) {
				artist = value;
			} else if(!strcasecmp(tag_key, "DATE")) {
				year = @([value intValue]);
			} else if(!strcasecmp(tag_key, "TRACK") ||
			          !strcasecmp(tag_key, "TRACKNUMBER")) {
				track = @([value intValue]);
			} else if(!strcasecmp(tag_key, "DISC") ||
			          !strcasecmp(tag_key, "DISCNUMBER")) {
				disc = @([value intValue]);
			} else if(!strcasecmp(tag_key, "TITLE")) {
				title = value;
			}
		}
		libvgmstream_tags_free(tags);
		libstreamfile_close(sf_tags);
	}

	BOOL formatFloat;
	int bps;
	switch(stream->format->sample_format) {
		case LIBVGMSTREAM_SFMT_PCM16: bps = 16; formatFloat = NO; break;
		case LIBVGMSTREAM_SFMT_PCM24: bps = 24; formatFloat = NO; break;
		case LIBVGMSTREAM_SFMT_PCM32: bps = 32; formatFloat = NO; break;
		case LIBVGMSTREAM_SFMT_FLOAT: bps = 32; formatFloat = YES; break;
		default:
			return;
	}

	NSDictionary *properties = @{ @"bitrate": @(bitrate / 1000),
		                          @"sampleRate": @(sampleRate),
		                          @"totalFrames": @(totalFrames),
								  @"bitsPerSample": @(bps),
		                          @"floatingPoint": @(formatFloat),
		                          @"channels": @(channels),
		                          @"seekable": @(YES),
		                          @"replaygain_album_gain": rgAlbumGain,
		                          @"replaygain_album_peak": rgAlbumPeak,
		                          @"replaygain_track_gain": rgTrackGain,
		                          @"replaygain_track_peak": rgTrackPeak,
		                          @"codec": codec,
		                          @"endian": @"host",
		                          @"encoding": @"lossy/lossless" };

	if([title isEqualToString:@""]) {
		if(stream->format->subsong_count > 1) {
			title = [NSString stringWithFormat:@"%@ - %@", [[urlTrimmed URLByDeletingPathExtension] lastPathComponent], guess_encoding_of_string(stream->format->stream_name)];
		} else {
			title = [[urlTrimmed URLByDeletingPathExtension] lastPathComponent];
		}
	}

	if([track isEqualToNumber:@(0)])
		track = @(track_num);

	NSMutableDictionary *mutableMetadata = [@{ @"title": title,
		                                       @"track": track,
		                                       @"disc": disc } mutableCopy];

	if(![album isEqualToString:@""])
		[mutableMetadata setValue:album forKey:@"album"];
	if(![artist isEqualToString:@""])
		[mutableMetadata setValue:artist forKey:@"artist"];
	if(![year isEqualToNumber:@(0)])
		[mutableMetadata setValue:year forKey:@"year"];

	NSDictionary *metadata = [NSDictionary dictionaryWithDictionary:mutableMetadata];

	NSDictionary *package = @{@"properties": properties,
							  @"metadata": metadata};

	@synchronized(self) {
		[storage setValue:package forKey:[url absoluteString]];
	}
}

- (NSDictionary *)getPropertiesForURL:(NSURL *)url {
	NSDictionary *properties = nil;

	@synchronized(self) {
		NSDictionary *package = [storage objectForKey:[url absoluteString]];
		if(package) {
			properties = [package objectForKey:@"properties"];
		}
	}

	return properties;
}

- (NSDictionary *)getMetadataForURL:(NSURL *)url {
	NSDictionary *metadata = nil;

	@synchronized(self) {
		NSDictionary *package = [storage objectForKey:[url absoluteString]];
		if(package) {
			metadata = [package objectForKey:@"metadata"];
		}
	}

	return metadata;
}

@end

@implementation VGMDecoder

+ (void)initialize {
	register_log_callback();
}

- (BOOL)open:(id<CogSource>)s {
	int track_num = [[[s url] fragment] intValue];

	loopCount = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthDefaultLoopCount"] intValue];
	if(loopCount < 1) {
		loopCount = 1;
	} else if(loopCount > 10) {
		loopCount = 10;
	}

	fadeTime = [[[[NSUserDefaultsController sharedUserDefaultsController] defaults] valueForKey:@"synthDefaultFadeSeconds"] doubleValue];
	if(fadeTime < 0.0) {
		fadeTime = 0.0;
	}

	NSString *path = [[s url] absoluteString];
	NSRange fragmentRange = [path rangeOfString:@"#" options:NSBackwardsSearch];
	if(fragmentRange.location != NSNotFound) {
		path = [path substringToIndex:fragmentRange.location];
	}

	playForever = IsRepeatOneSet();

	libvgmstream_config_t vcfg = { 0 };

	vcfg.allow_play_forever = 1;
	vcfg.play_forever = playForever;
	vcfg.loop_count = loopCount;
	vcfg.fade_time = fadeTime;
	vcfg.fade_delay = 0;
	vcfg.ignore_loop = 0;
	vcfg.auto_downmix_channels = 6;

	NSLog(@"Opening %@ subsong %d", path, track_num);

	libstreamfile_t* sf = open_vfs([[path stringByRemovingPercentEncoding] UTF8String]);
	if(!sf)
		return NO;
	stream = libvgmstream_create(sf, track_num, &vcfg);
	libstreamfile_close(sf);
	if(!stream)
		return NO;

	int output_channels = stream->format->channels;

	sampleRate = stream->format->sample_rate;
	channels = output_channels;
	totalFrames = stream->format->play_samples;

	framesRead = 0;

	bitrate = stream->format->stream_bitrate;

	switch(stream->format->sample_format) {
		case LIBVGMSTREAM_SFMT_PCM16: bps = 16; formatFloat = NO; break;
		case LIBVGMSTREAM_SFMT_PCM24: bps = 24; formatFloat = NO; break;
		case LIBVGMSTREAM_SFMT_PCM32: bps = 32; formatFloat = NO; break;
		case LIBVGMSTREAM_SFMT_FLOAT: bps = 32; formatFloat = YES; break;
		default:
			libvgmstream_free(stream);
			stream = NULL;
			return NO;
	}

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (NSDictionary *)properties {
	return @{ @"bitrate": @(bitrate / 1000),
		      @"sampleRate": @(sampleRate),
		      @"totalFrames": @(totalFrames),
			  @"bitsPerSample": @(bps),
		      @"floatingPoint": @(formatFloat),
		      @"channels": @(channels),
		      @"seekable": @(YES),
		      @"endian": @"host",
		      @"encoding": @"lossy/lossless" };
}

- (NSDictionary *)metadata {
	return @{};
}

- (AudioChunk *)readAudio {
	double streamTimestamp = (double)(libvgmstream_get_play_position(stream)) / sampleRate;

	id audioChunkClass = NSClassFromString(@"AudioChunk");
	AudioChunk *chunk = [[audioChunkClass alloc] initWithProperties:[self properties]];
	size_t framesDone = 0;

	for(;;) {
		if(stream->decoder->done) break;

		int err = libvgmstream_render(stream);
		if(err < 0)
			break;

		const size_t bytes_done = stream->decoder->buf_bytes;
		if(!bytes_done) {
			continue;
		}

		const size_t bytes_per_sample = stream->format->channels * (bps / 8);
		framesDone = bytes_done / bytes_per_sample;

		framesRead += framesDone;
		break;
	}

	if(framesDone) {
		[chunk setStreamTimestamp:streamTimestamp];
		[chunk assignSamples:stream->decoder->buf frameCount:framesDone];
	}

	return chunk;
}

- (long)seek:(long)frame {
	if(frame > totalFrames)
		frame = totalFrames;

	libvgmstream_seek(stream, frame);

	framesRead = frame;

	return frame;
}

- (void)close {
	libvgmstream_free(stream);
	stream = NULL;
}

- (void)dealloc {
	[self close];
}

+ (NSArray *)fileTypes {
	NSMutableArray *array = [NSMutableArray new];

	int count;
	const char **formats = libvgmstream_get_extensions(&count);

	for(size_t i = 0; i < count; ++i) {
		[array addObject:[NSString stringWithUTF8String:formats[i]]];
	}

	return [NSArray arrayWithArray:array];
}

+ (NSArray *)mimeTypes {
	return nil;
}

+ (float)priority {
	return 0.0;
}

+ (NSArray *)fileTypeAssociations {
	NSMutableArray *ret = [NSMutableArray new];
	[ret addObject:@"VGMStream Files"];
	[ret addObject:@"vg.icns"];
	[ret addObjectsFromArray:[self fileTypes]];

	return @[[NSArray arrayWithArray:ret]];
}

@end
