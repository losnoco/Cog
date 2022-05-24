//
//  VGMDecoder.m
//  vgmstream
//
//  Created by Christopher Snowhill on 02/25/14.
//  Copyright 2014 __NoWork, Inc__. All rights reserved.
//

#import "VGMDecoder.h"
#import "VGMInterface.h"

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
		sharedMyCache = [[self alloc] init];
	});
	return sharedMyCache;
}

- (id)init {
	if(self = [super init]) {
		storage = [[NSMutableDictionary alloc] init];
	}
	return self;
}

- (void)stuffURL:(NSURL *)url stream:(VGMSTREAM *)stream {
	vgmstream_cfg_t vcfg = { 0 };

	vcfg.allow_play_forever = 1;
	vcfg.play_forever = 0;
	vcfg.loop_count = 2;
	vcfg.fade_time = 10;
	vcfg.fade_delay = 0;
	vcfg.ignore_loop = 0;

	vgmstream_apply_config(stream, &vcfg);

	int output_channels = stream->channels;

	vgmstream_mixing_autodownmix(stream, 6);
	vgmstream_mixing_enable(stream, MAX_BUFFER_SAMPLES, NULL, &output_channels);

	int track_num = [[url fragment] intValue];

	int sampleRate = stream->sample_rate;
	int channels = output_channels;
	long totalFrames = vgmstream_get_samples(stream);

	int bitrate = get_vgmstream_average_bitrate(stream);

	char description[1024];
	describe_vgmstream(stream, description, 1024);

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
	NSNumber *year = [NSNumber numberWithInt:0];
	NSNumber *track = [NSNumber numberWithInt:0];
	NSNumber *disc = [NSNumber numberWithInt:0];
	NSString *title = @"";

	NSString *codec;

	NSNumber *rgTrackGain = [NSNumber numberWithInt:0];
	NSNumber *rgTrackPeak = [NSNumber numberWithInt:0];
	NSNumber *rgAlbumGain = [NSNumber numberWithInt:0];
	NSNumber *rgAlbumPeak = [NSNumber numberWithInt:0];

	codec = get_description_tag(description, "encoding: ", 0);

	STREAMFILE *tagFile = open_cog_streamfile_from_url(tagurl);
	if(tagFile) {
		VGMSTREAM_TAGS *tags;
		const char *tag_key, *tag_val;

		tags = vgmstream_tags_init(&tag_key, &tag_val);
		vgmstream_tags_reset(tags, [filename UTF8String]);
		while(vgmstream_tags_next_tag(tags, tagFile)) {
			NSString *value = guess_encoding_of_string(tag_val);
			if(!strncasecmp(tag_key, "REPLAYGAIN_", strlen("REPLAYGAIN_"))) {
				if(!strncasecmp(tag_key + strlen("REPLAYGAIN_"), "TRACK_", strlen("TRACK_"))) {
					if(!strcasecmp(tag_key + strlen("REPLAYGAIN_TRACK_"), "GAIN")) {
						rgTrackGain = [NSNumber numberWithFloat:[value floatValue]];
					} else if(!strcasecmp(tag_key + strlen("REPLAYGAIN_TRACK_"), "PEAK")) {
						rgTrackPeak = [NSNumber numberWithFloat:[value floatValue]];
					}
				} else if(!strncasecmp(tag_key + strlen("REPLAYGAIN_"), "ALBUM_", strlen("ALBUM_"))) {
					if(!strcasecmp(tag_key + strlen("REPLAYGAIN_ALBUM_"), "GAIN")) {
						rgAlbumGain = [NSNumber numberWithFloat:[value floatValue]];
					} else if(!strcasecmp(tag_key + strlen("REPLAYGAIN_ALBUM_"), "PEAK")) {
						rgAlbumPeak = [NSNumber numberWithFloat:[value floatValue]];
					}
				}
			} else if(!strcasecmp(tag_key, "ALBUM")) {
				album = value;
			} else if(!strcasecmp(tag_key, "ARTIST")) {
				artist = value;
			} else if(!strcasecmp(tag_key, "DATE")) {
				year = [NSNumber numberWithInt:[value intValue]];
			} else if(!strcasecmp(tag_key, "TRACK") ||
			          !strcasecmp(tag_key, "TRACKNUMBER")) {
				track = [NSNumber numberWithInt:[value intValue]];
			} else if(!strcasecmp(tag_key, "DISC") ||
			          !strcasecmp(tag_key, "DISCNUMBER")) {
				disc = [NSNumber numberWithInt:[value intValue]];
			} else if(!strcasecmp(tag_key, "TITLE")) {
				title = value;
			}
		}
		vgmstream_tags_close(tags);
		close_streamfile(tagFile);
	}

	NSDictionary *properties = @{@"bitrate": [NSNumber numberWithInt:bitrate / 1000],
								 @"sampleRate": [NSNumber numberWithInt:sampleRate],
								 @"totalFrames": [NSNumber numberWithDouble:totalFrames],
								 @"bitsPerSample": [NSNumber numberWithInt:16],
								 @"floatingPoint": [NSNumber numberWithBool:NO],
								 @"channels": [NSNumber numberWithInt:channels],
								 @"seekable": [NSNumber numberWithBool:YES],
								 @"replayGainAlbumGain": rgAlbumGain,
								 @"replayGainAlbumPeak": rgAlbumPeak,
								 @"replayGainTrackGain": rgTrackGain,
								 @"replayGainTrackPeak": rgTrackPeak,
								 @"codec": codec,
								 @"endian": @"host",
								 @"encoding": @"lossy/lossless"};

	if([title isEqualToString:@""]) {
		if(stream->num_streams > 1) {
			title = [NSString stringWithFormat:@"%@ - %@", [[urlTrimmed URLByDeletingPathExtension] lastPathComponent], guess_encoding_of_string(stream->stream_name)];
		} else {
			title = [[urlTrimmed URLByDeletingPathExtension] lastPathComponent];
		}
	}

	if([track isEqualToNumber:[NSNumber numberWithInt:0]])
		track = [NSNumber numberWithInt:track_num];

	NSMutableDictionary *mutableMetadata = [@{ @"title": title,
		                                       @"track": track,
		                                       @"disc": disc } mutableCopy];

	if(![album isEqualToString:@""])
		[mutableMetadata setValue:album forKey:@"album"];
	if(![artist isEqualToString:@""])
		[mutableMetadata setValue:artist forKey:@"artist"];
	if(![year isEqualToNumber:[NSNumber numberWithInt:0]])
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

	NSString *path = [[s url] absoluteString];
	NSRange fragmentRange = [path rangeOfString:@"#" options:NSBackwardsSearch];
	if(fragmentRange.location != NSNotFound) {
		path = [path substringToIndex:fragmentRange.location];
	}

	NSLog(@"Opening %@ subsong %d", path, track_num);

	stream = init_vgmstream_from_cogfile([[path stringByRemovingPercentEncoding] UTF8String], track_num);
	if(!stream)
		return NO;

	int output_channels = stream->channels;

	vgmstream_mixing_autodownmix(stream, 6);
	vgmstream_mixing_enable(stream, MAX_BUFFER_SAMPLES, NULL, &output_channels);

	canPlayForever = stream->loop_flag;
	if(canPlayForever) {
		playForever = IsRepeatOneSet();
	} else {
		playForever = NO;
	}

	vgmstream_cfg_t vcfg = { 0 };

	vcfg.allow_play_forever = 1;
	vcfg.play_forever = playForever;
	vcfg.loop_count = 2;
	vcfg.fade_time = 10;
	vcfg.fade_delay = 0;
	vcfg.ignore_loop = 0;

	vgmstream_apply_config(stream, &vcfg);

	sampleRate = stream->sample_rate;
	channels = output_channels;
	totalFrames = vgmstream_get_samples(stream);

	framesRead = 0;

	bitrate = get_vgmstream_average_bitrate(stream);

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (NSDictionary *)properties {
	return @{@"bitrate": [NSNumber numberWithInt:bitrate / 1000],
			 @"sampleRate": [NSNumber numberWithInt:sampleRate],
			 @"totalFrames": [NSNumber numberWithDouble:totalFrames],
			 @"bitsPerSample": [NSNumber numberWithInt:16],
			 @"floatingPoint": [NSNumber numberWithBool:NO],
			 @"channels": [NSNumber numberWithInt:channels],
			 @"seekable": [NSNumber numberWithBool:YES],
			 @"endian": @"host",
			 @"encoding": @"lossy/lossless"};
}

- (NSDictionary *)metadata {
	return @{};
}

- (int)readAudio:(void *)buf frames:(UInt32)frames {
	UInt32 framesMax = frames;
	UInt32 framesDone = 0;

	if(canPlayForever) {
		BOOL repeatone = IsRepeatOneSet();

		if(repeatone != playForever) {
			playForever = repeatone;
			vgmstream_set_play_forever(stream, repeatone);
		}
	}

	if(framesRead + frames > totalFrames && !playForever)
		frames = totalFrames - framesRead;
	if(frames > framesMax)
		frames = 0; // integer overflow?

	while(frames) {
		sample sample_buffer[MAX_BUFFER_SAMPLES * VGMSTREAM_MAX_CHANNELS];

		UInt32 frames_to_do = frames;
		if(frames_to_do > MAX_BUFFER_SAMPLES)
			frames_to_do = MAX_BUFFER_SAMPLES;

		memset(sample_buffer, 0, frames_to_do * channels * sizeof(sample_buffer[0]));

		render_vgmstream(sample_buffer, frames_to_do, stream);

		framesRead += frames_to_do;
		framesDone += frames_to_do;

		sample *sbuf = (sample *)buf;

		memcpy(sbuf, sample_buffer, frames_to_do * channels * sizeof(sbuf[0]));

		sbuf += frames_to_do * channels;

		buf = (void *)sbuf;

		frames -= frames_to_do;
	}

	return framesDone;
}

- (long)seek:(long)frame {
	if(canPlayForever) {
		BOOL repeatone = IsRepeatOneSet();

		if(repeatone != playForever) {
			playForever = repeatone;
			vgmstream_set_play_forever(stream, repeatone);
		}
	}

	if(frame > totalFrames)
		frame = totalFrames;

	seek_vgmstream(stream, frame);

	framesRead = frame;

	return frame;
}

- (void)close {
	close_vgmstream(stream);
	stream = NULL;
}

- (void)dealloc {
	[self close];
}

+ (NSArray *)fileTypes {
	NSMutableArray *array = [[NSMutableArray alloc] init];

	size_t count;
	const char **formats = vgmstream_get_formats(&count);

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
	NSMutableArray *ret = [[NSMutableArray alloc] init];
	[ret addObject:@"VGMStream Files"];
	[ret addObject:@"vg.icns"];
	[ret addObjectsFromArray:[self fileTypes]];

	return @[[NSArray arrayWithArray:ret]];
}

@end
