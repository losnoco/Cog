//
//  VorbisFile.m
//  zyVorbis
//
//  Created by Vincent Spader on 1/22/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "VorbisDecoder.h"

#import "Logging.h"

#import "HTTPSource.h"

#import "picture.h"

#import "NSDictionary+Merge.h"

#import <FLAC/metadata.h>

@implementation VorbisDecoder

//static const int MAXCHANNELS = 8;
enum { MAXCHANNELS = 8 };
static const int chmap[MAXCHANNELS][MAXCHANNELS] = {
	{ 0, }, // mono
	{ 0, 1, }, // l, r
	{ 0, 2, 1, }, // l, c, r -> l, r, c
	{ 0, 1, 2, 3, }, // l, r, bl, br
	{ 0, 2, 1, 3, 4, }, // l, c, r, bl, br -> l, r, c, bl, br
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
	frame = 0;

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	metaDict = @{};
	icyMetaDict = @{};
	albumArt = [NSData data];

	[self updateMetadata];

	metadataUpdateInterval = frequency;
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
	vorbis_comment *tags = ov_comment(&vorbisRef, -1);
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
						flac_picture_t *picture = flac_picture_parse_from_base64(value);
						if(picture) {
							if(picture->binary && picture->binary_length) {
								_albumArt = [NSData dataWithBytes:picture->binary length:picture->binary_length];
							}
							flac_picture_free(picture);
						}
					} else if([tagName isEqualToString:@"unsynced lyrics"] ||
							  [tagName isEqualToString:@"lyrics"]) {
						setDictionary(_metaDict, @"unsyncedlyrics", guess_encoding_of_string(value));
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
	int frames = 1024;

	double streamTimestamp = (double)(frame) / frequency;

	if(currentSection != lastSection) {
		vorbis_info *vi;
		vi = ov_info(&vorbisRef, -1);

		bitrate = (vi->bitrate_nominal / 1000.0);
		channels = vi->channels;
		frequency = vi->rate;

		metadataUpdateInterval = frequency;
		metadataUpdateCount = 0;

		[self willChangeValueForKey:@"properties"];
		[self didChangeValueForKey:@"properties"];

		[self updateMetadata];
	}

	id audioChunkClass = NSClassFromString(@"AudioChunk");
	AudioChunk *chunk = [[audioChunkClass alloc] initWithProperties:[self properties]];

	float buffer[frames * channels];
	void *buf = (void *)buffer;

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

	metadataUpdateCount += total;
	if(metadataUpdateCount >= metadataUpdateInterval) {
		metadataUpdateCount -= metadataUpdateInterval;
		[self updateIcyMetadata];
	}

	[chunk setStreamTimestamp:streamTimestamp];
	[chunk assignSamples:buffer frameCount:total];

	return chunk;
}

- (void)close {
	ov_clear(&vorbisRef);
}

- (void)dealloc {
	[self close];
}

- (long)seek:(long)frame {
	ov_pcm_seek(&vorbisRef, frame);

	self->frame = frame;

	return frame;
}

- (NSDictionary *)properties {
	return @{ @"channels": @(channels),
		      @"bitsPerSample": @(32),
		      @"floatingPoint": @(YES),
		      @"sampleRate": @(frequency),
		      @"totalFrames": @(totalFrames),
		      @"bitrate": @(bitrate),
		      @"seekable": @([source seekable] && seekable),
		      @"codec": @"Ogg Vorbis",
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
	return @[@"ogg"];
}

+ (NSArray *)mimeTypes {
	return @[@"application/ogg", @"application/x-ogg", @"audio/ogg", @"audio/x-vorbis+ogg"];
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
