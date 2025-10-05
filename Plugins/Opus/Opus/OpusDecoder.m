//
//  OpusDecoder.m
//  Opus
//
//  Created by Christopher Snowhill on 10/4/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "Plugin.h"

#import "OpusDecoder.h"

#import "Logging.h"

#import "HTTPSource.h"

#import "NSDictionary+Merge.h"

#import <FLAC/metadata.h>

@implementation OpusFile

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

int sourceRead(void *_stream, unsigned char *_ptr, int _nbytes) {
	id source = (__bridge id)_stream;

	return (int)[source read:_ptr amount:_nbytes];
}

int sourceSeek(void *_stream, opus_int64 _offset, int _whence) {
	id source = (__bridge id)_stream;
	return ([source seek:_offset whence:_whence] ? 0 : -1);
}

int sourceClose(void *_stream) {
	return 0;
}

opus_int64 sourceTell(void *_stream) {
	id source = (__bridge id)_stream;

	return [source tell];
}

- (id)init {
	self = [super init];
	if(self) {
		opusRef = NULL;
	}
	return self;
}

- (BOOL)open:(id<CogSource>)s {
	source = s;

	OpusFileCallbacks callbacks = {
		.read = sourceRead,
		.seek = sourceSeek,
		.close = sourceClose,
		.tell = sourceTell
	};

	int error;
	opusRef = op_open_callbacks((__bridge void *)source, &callbacks, NULL, 0, &error);

	if(!opusRef) {
		DLog(@"FAILED TO OPEN OPUS FILE");
		return NO;
	}

	currentSection = lastSection = op_current_link(opusRef);

	bitrate = (op_bitrate(opusRef, currentSection) / 1000.0);
	channels = op_channel_count(opusRef, currentSection);

	seekable = op_seekable(opusRef);

	totalFrames = op_pcm_total(opusRef, -1);
	frame = 0;
	
	const OpusHead *head = op_head(opusRef, -1);
	const OpusTags *tags = op_tags(opusRef, -1);

	int _track_gain = 0;

	opus_tags_get_track_gain(tags, &_track_gain);

	replayGainAlbumGain = ((double)head->output_gain / 256.0) + 5.0;
	replayGainTrackGain = ((double)_track_gain / 256.0) + replayGainAlbumGain;

	op_set_gain_offset(opusRef, OP_ABSOLUTE_GAIN, 0);

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	metaDict = @{};
	icyMetaDict = @{};
	albumArt = [NSData data];

	[self updateMetadata];

	metadataUpdateInterval = 48000;
	metadataUpdateCount = 0;

	return YES;
}

static void setDictionary(NSMutableDictionary *dict, NSString *tag, NSString *value) {
	NSString *realKey = [tag stringByReplacingOccurrencesOfString:@"." withString:@"․"];
	NSMutableArray *array = [dict valueForKey:realKey];
	if(!array) {
		array = [NSMutableArray new];
		[dict setObject:array forKey:realKey];
	}
	[array addObject:value];
}

- (void)updateMetadata {
	const struct OpusTags *tags = op_tags(opusRef, -1);
	NSMutableDictionary *_metaDict = [NSMutableDictionary new];
	NSData *_albumArt = albumArt;

	if(tags) {
		for(int i = 0; i < tags->comments; ++i) {
			FLAC__StreamMetadata_VorbisComment_Entry entry = { .entry = (FLAC__byte *)tags->user_comments[i], .length = tags->comment_lengths[i] };
			char *name, *value;
			if(FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair(entry, &name, &value)) {
				@autoreleasepool {
					NSString *tagName = guess_encoding_of_string(name);
					free(name);

					tagName = [tagName lowercaseString];

					if([tagName isEqualToString:@"metadata_block_picture"]) {
						OpusPictureTag _pic = { 0 };
						if(opus_picture_tag_parse(&_pic, value) >= 0) {
							if(_pic.format == OP_PIC_FORMAT_PNG ||
							   _pic.format == OP_PIC_FORMAT_JPEG ||
							   _pic.format == OP_PIC_FORMAT_GIF ||
							   _pic.format == OP_PIC_FORMAT_UNKNOWN) {
								_albumArt = [NSData dataWithBytes:_pic.data length:_pic.data_length];
							}
						}
						opus_picture_tag_clear(&_pic);
					} else if([tagName isEqualToString:@"unsynced lyrics"] ||
							  [tagName isEqualToString:@"lyrics"]) {
						setDictionary(_metaDict, @"unsyncedlyrics", guess_encoding_of_string(value));
					} else if([tagName isEqualToString:@"comments:itunnorm"]) {
						setDictionary(_metaDict, @"soundcheck", guess_encoding_of_string(value));
					} else {
						setDictionary(_metaDict, tagName, guess_encoding_of_string(value));
					}

					free(value);
				}
			}
		}

		if(![_albumArt isEqualToData:albumArt] ||
		   ![_metaDict isEqualToDictionary:metaDict]) {
			@autoreleasepool {
				metaDict = _metaDict;
				albumArt = _albumArt;
			}

			if(![source seekable]) {
				[self willChangeValueForKey:@"metadata"];
				[self didChangeValueForKey:@"metadata"];
			}
		}
	}
}

- (void)updateIcyMetadata {
	if([source seekable]) return;

	NSMutableDictionary *_icyMetaDict = [NSMutableDictionary new];

	Class sourceClass = [source class];
	if([sourceClass isEqual:NSClassFromString(@"HTTPSource")]) {
		HTTPSource *httpSource = (HTTPSource *)source;
		if([httpSource hasMetadata]) {
			@autoreleasepool {
				NSDictionary *metadata = [httpSource metadata];
				NSString *_genre = [metadata valueForKey:@"genre"];
				NSString *_album = [metadata valueForKey:@"album"];
				NSString *_artist = [metadata valueForKey:@"artist"];
				NSString *_title = [metadata valueForKey:@"title"];

				if(_genre && [_genre length]) {
					setDictionary(_icyMetaDict, @"genre", _genre);
				}
				if(_album && [_album length]) {
					setDictionary(_icyMetaDict, @"album", _album);
				}
				if(_artist && [_artist length]) {
					setDictionary(_icyMetaDict, @"artist", _artist);
				}
				if(_title && [_title length]) {
					setDictionary(_icyMetaDict, @"title", _title);
				}
			}
		}
	}

	if(![_icyMetaDict isEqualToDictionary:icyMetaDict]) {
		@autoreleasepool {
			icyMetaDict = _icyMetaDict;
		}
		[self willChangeValueForKey:@"metadata"];
		[self didChangeValueForKey:@"metadata"];
	}
}

- (AudioChunk *)readAudio {
	int numread;
	int total = 0;

	if(currentSection != lastSection) {
		bitrate = (op_bitrate(opusRef, currentSection) / 1000.0);
		channels = op_channel_count(opusRef, currentSection);

		[self willChangeValueForKey:@"properties"];
		[self didChangeValueForKey:@"properties"];

		[self updateMetadata];
	}

	int frames = 1024;
	int size = frames * channels;
	float buffer[size];
	void *buf = (void *)buffer;

	do {
		float *out = ((float *)buf) + total;
		float tempbuf[512 * channels];
		lastSection = currentSection;
		int toread = size - total;
		if(toread > 512) toread = 512;
		numread = op_read_float(opusRef, (channels < MAXCHANNELS) ? tempbuf : out, toread, NULL);
		if(numread > 0 && channels <= MAXCHANNELS) {
			for(int i = 0; i < numread; ++i) {
				for(int j = 0; j < channels; ++j) {
					out[i * channels + j] = tempbuf[i * channels + chmap[channels - 1][j]];
				}
			}
		}
		currentSection = op_current_link(opusRef);
		if(numread > 0) {
			total += numread * channels;
		}

		if(currentSection != lastSection) {
			break;
		}

	} while(total != size && numread != 0);

	metadataUpdateCount += total / channels;
	if(metadataUpdateCount >= metadataUpdateInterval) {
		metadataUpdateCount -= metadataUpdateInterval;
		[self updateIcyMetadata];
	}

	double streamTimestamp = (double)(frame) / 48000.0;
	frame += total / channels;

	id audioChunkClass = NSClassFromString(@"AudioChunk");
	AudioChunk *chunk = [[audioChunkClass alloc] initWithProperties:[self properties]];
	[chunk setStreamTimestamp:streamTimestamp];
	[chunk assignSamples:buffer frameCount:total / channels];

	return chunk;
}

- (void)close {
	op_free(opusRef);
	opusRef = NULL;
}

- (void)dealloc {
	[self close];
}

- (long)seek:(long)frame {
	op_pcm_seek(opusRef, frame);

	self->frame = frame;

	return frame;
}

- (NSDictionary *)properties {
	return @{ @"channels": @(channels),
		      @"bitsPerSample": @(32),
		      @"floatingPoint": @(YES),
		      @"sampleRate": @(48000),
		      @"totalFrames": @(totalFrames),
		      @"bitrate": @(bitrate),
		      @"seekable": @([source seekable] && seekable),
		      @"replaygain_album_gain": @(replayGainAlbumGain),
		      @"replaygain_track_gain": @(replayGainTrackGain),
		      @"codec": @"Opus",
		      @"endian": @"host",
		      @"encoding": @"lossy" };
}

- (NSDictionary *)metadata {
	NSDictionary *dict1 = @{ @"albumArt": albumArt };
	NSDictionary *dict2 = [dict1 dictionaryByMergingWith:metaDict];
	NSDictionary *dict3 = [dict2 dictionaryByMergingWith:icyMetaDict];
	return dict3;
}

+ (NSArray *)fileTypes {
	return @[@"opus", @"ogg"];
}

+ (NSArray *)mimeTypes {
	return @[@"audio/x-opus+ogg", @"application/ogg"];
}

// We want this to take priority over both Core Audio and FFmpeg
+ (float)priority {
	return 2.0;
}

+ (NSArray *)fileTypeAssociations {
	return @[
		@[@"Opus Audio File", @"ogg.icns", @"opus"],
		@[@"Ogg Audio File", @"ogg.icns", @"ogg"]
	];
}

@end
