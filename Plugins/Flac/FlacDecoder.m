//
//  FlacDecoder.m
//  zyVorbis
//
//  Created by Vincent Spader on 1/25/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "FlacDecoder.h"

#import "Logging.h"

#import "HTTPSource.h"

#import "NSDictionary+Merge.h"

extern void grabbag__cuesheet_emit(NSString **out, const FLAC__StreamMetadata *cuesheet, const char *file_reference);

@implementation FlacDecoder

FLAC__StreamDecoderReadStatus ReadCallback(const FLAC__StreamDecoder *decoder, FLAC__byte blockBuffer[], size_t *bytes, void *client_data) {
	FlacDecoder *flacDecoder = (__bridge FlacDecoder *)client_data;
	long bytesRead = [[flacDecoder source] read:blockBuffer amount:*bytes];

	if(bytesRead < 0) {
		*bytes = 0;
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT;
	} else if(bytesRead == 0) {
		*bytes = 0;
		[flacDecoder setEndOfStream:YES];
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
	} else {
		*bytes = bytesRead;
		return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
	}
}

FLAC__StreamDecoderSeekStatus SeekCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 absolute_byte_offset, void *client_data) {
	FlacDecoder *flacDecoder = (__bridge FlacDecoder *)client_data;

	if(![[flacDecoder source] seek:absolute_byte_offset whence:SEEK_SET])
		return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR;
	else {
		[flacDecoder setEndOfStream:NO];
		return FLAC__STREAM_DECODER_SEEK_STATUS_OK;
	}
}

FLAC__StreamDecoderTellStatus TellCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *absolute_byte_offset, void *client_data) {
	FlacDecoder *flacDecoder = (__bridge FlacDecoder *)client_data;

	off_t pos;
	if((pos = [[flacDecoder source] tell]) < 0)
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
	else {
		*absolute_byte_offset = (FLAC__uint64)pos;
		return FLAC__STREAM_DECODER_TELL_STATUS_OK;
	}
}

FLAC__bool EOFCallback(const FLAC__StreamDecoder *decoder, void *client_data) {
	FlacDecoder *flacDecoder = (__bridge FlacDecoder *)client_data;

	return (FLAC__bool)[flacDecoder endOfStream];
}

FLAC__StreamDecoderLengthStatus LengthCallback(const FLAC__StreamDecoder *decoder, FLAC__uint64 *stream_length, void *client_data) {
	FlacDecoder *flacDecoder = (__bridge FlacDecoder *)client_data;

	if([[flacDecoder source] seekable]) {
		long currentPos = [[flacDecoder source] tell];

		[[flacDecoder source] seek:0 whence:SEEK_END];
		*stream_length = [[flacDecoder source] tell];

		[[flacDecoder source] seek:currentPos whence:SEEK_SET];

		return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
	} else {
		*stream_length = 0;
		return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
	}
}

FLAC__StreamDecoderWriteStatus WriteCallback(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 *const sampleblockBuffer[], void *client_data) {
	FlacDecoder *flacDecoder = (__bridge FlacDecoder *)client_data;

	if(flacDecoder->abortFlag)
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

	uint32_t channels = frame->header.channels;
	uint32_t bitsPerSample = frame->header.bits_per_sample;
	uint32_t frequency = frame->header.sample_rate;

	if(channels != flacDecoder->channels ||
	   bitsPerSample != flacDecoder->bitsPerSample ||
	   frequency != flacDecoder->frequency) {
		if(channels != flacDecoder->channels) {
			flacDecoder->channelConfig = 0;
		}
		flacDecoder->channels = channels;
		flacDecoder->bitsPerSample = bitsPerSample;
		flacDecoder->frequency = frequency;
		[flacDecoder willChangeValueForKey:@"properties"];
		[flacDecoder didChangeValueForKey:@"properties"];
	}

	void *blockBuffer = [flacDecoder blockBuffer];

	int8_t *alias8;
	int16_t *alias16;
	int32_t *alias32;
	int sample, channel;
	int32_t audioSample;

	switch(frame->header.bits_per_sample) {
		case 8:
			// Interleave the audio (no need for byte swapping)
			alias8 = blockBuffer;
			for(sample = 0; sample < frame->header.blocksize; ++sample) {
				for(channel = 0; channel < frame->header.channels; ++channel) {
					*alias8++ = (int8_t)sampleblockBuffer[channel][sample];
				}
			}

			break;

		case 16:
			// Interleave the audio, converting to big endian byte order
			alias16 = blockBuffer;
			for(sample = 0; sample < frame->header.blocksize; ++sample) {
				for(channel = 0; channel < frame->header.channels; ++channel) {
					*alias16++ = (int16_t)OSSwapHostToBigInt16((int16_t)sampleblockBuffer[channel][sample]);
				}
			}

			break;

		case 24:
			// Interleave the audio (no need for byte swapping)
			alias8 = blockBuffer;
			for(sample = 0; sample < frame->header.blocksize; ++sample) {
				for(channel = 0; channel < frame->header.channels; ++channel) {
					audioSample = sampleblockBuffer[channel][sample];
					*alias8++ = (int8_t)(audioSample >> 16);
					*alias8++ = (int8_t)(audioSample >> 8);
					*alias8++ = (int8_t)audioSample;
				}
			}

			break;

		case 32:
			// Interleave the audio, converting to big endian byte order
			alias32 = blockBuffer;
			for(sample = 0; sample < frame->header.blocksize; ++sample) {
				for(channel = 0; channel < frame->header.channels; ++channel) {
					*alias32++ = OSSwapHostToBigInt32(sampleblockBuffer[channel][sample]);
				}
			}
		default:
			// Time for some nearest byte padding up to 32
			alias8 = blockBuffer;
			int sampleSize = frame->header.bits_per_sample;
			int sampleBit;
			for(sample = 0; sample < frame->header.blocksize; ++sample) {
				for(channel = 0; channel < frame->header.channels; ++channel) {
					int32_t sampleExtended = sampleblockBuffer[channel][sample];
					for(sampleBit = sampleSize - 8; sampleBit >= -8; sampleBit -= 8) {
						if(sampleBit >= 0)
							*alias8++ = (uint8_t)((sampleExtended >> sampleBit) & 0xFF);
						else
							*alias8++ = (uint8_t)((sampleExtended << -sampleBit) & 0xFF);
					}
				}
			}
			break;
	}

	[flacDecoder setBlockBufferFrames:frame->header.blocksize];

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static void setDictionary(NSMutableDictionary *dict, NSString *tag, NSString *value) {
	NSString *realKey = [tag stringByReplacingOccurrencesOfString:@"." withString:@"․"];
	NSMutableArray *array = [dict valueForKey:realKey];
	if(!array) {
		array = [[NSMutableArray alloc] init];
		[dict setObject:array forKey:realKey];
	}
	[array addObject:value];
}

// This callback is only called for STREAMINFO blocks
void MetadataCallback(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data) {
	// Some flacs observed in the wild have multiple STREAMINFO metadata blocks,
	// of which only first one has sane values, so only use values from the first STREAMINFO
	// to determine stream format (this seems to be consistent with flac spec: http://flac.sourceforge.net/format.html)
	FlacDecoder *flacDecoder = (__bridge FlacDecoder *)client_data;

	if(!flacDecoder->hasStreamInfo && metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
		flacDecoder->channels = metadata->data.stream_info.channels;
		flacDecoder->channelConfig = 0;
		flacDecoder->frequency = metadata->data.stream_info.sample_rate;
		flacDecoder->bitsPerSample = metadata->data.stream_info.bits_per_sample;

		flacDecoder->totalFrames = metadata->data.stream_info.total_samples;

		flacDecoder->hasStreamInfo = YES;
	}

	if(metadata->type == FLAC__METADATA_TYPE_CUESHEET && !flacDecoder->cuesheetFound) {
		flacDecoder->cuesheetFound = YES;

		@autoreleasepool {
			NSString *_cuesheet;
			grabbag__cuesheet_emit(&_cuesheet, metadata, [[NSString stringWithFormat:@"\"%@\"", [[[flacDecoder->source url] path] lastPathComponent]] UTF8String]);

			if(![_cuesheet isEqual:flacDecoder->cuesheet]) {
				flacDecoder->cuesheet = _cuesheet;
				if(![flacDecoder->source seekable]) {
					[flacDecoder willChangeValueForKey:@"metadata"];
					[flacDecoder didChangeValueForKey:@"metadata"];
				}
			}
		}
	}

	if(metadata->type == FLAC__METADATA_TYPE_PICTURE) {
		NSData *_albumArt = [NSData dataWithBytes:metadata->data.picture.data length:metadata->data.picture.data_length];
		if(![_albumArt isEqual:flacDecoder->albumArt]) {
			flacDecoder->albumArt = _albumArt;
			if(![flacDecoder->source seekable]) {
				[flacDecoder willChangeValueForKey:@"metadata"];
				[flacDecoder didChangeValueForKey:@"metadata"];
			}
		}
	}

	if(metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
		NSMutableDictionary *_metaDict = [[NSMutableDictionary alloc] init];
		NSString *_cuesheet = flacDecoder->cuesheet;
		const FLAC__StreamMetadata_VorbisComment *vorbis_comment = &metadata->data.vorbis_comment;
		for(int i = 0; i < vorbis_comment->num_comments; ++i) {
			char *_name;
			char *_value;
			if(FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair(vorbis_comment->comments[i], &_name, &_value)) {
				@autoreleasepool {
					NSString *name = guess_encoding_of_string(_name);
					NSString *value = guess_encoding_of_string(_value);
					free(_name);
					free(_value);
					name = [name lowercaseString];
					name = [flacDecoder->dataStore coalesceString:name];
					value = [flacDecoder->dataStore coalesceString:value];
					if([name isEqualToString:@"cuesheet"]) {
						_cuesheet = value;
						flacDecoder->cuesheetFound = YES;
					} else if([name isEqualToString:@"waveformatextensible_channel_mask"]) {
						if([value hasPrefix:@"0x"]) {
							char *end;
							const char *_value = [value UTF8String] + 2;
							flacDecoder->channelConfig = (uint32_t)strtoul(_value, &end, 16);
						}
					} else if([name isEqualToString:@"unsynced lyrics"] ||
							  [name isEqualToString:@"lyrics"]) {
						setDictionary(_metaDict, @"unsyncedlyrics", value);
					} else {
						setDictionary(_metaDict, name, value);
					}
				}
			}
		}

		if(![_metaDict isEqualToDictionary:flacDecoder->metaDict] ||
		   ![_cuesheet isEqualToString:flacDecoder->cuesheet]) {
			flacDecoder->metaDict = _metaDict;
			flacDecoder->cuesheet = _cuesheet;

			if(![flacDecoder->source seekable]) {
				[flacDecoder willChangeValueForKey:@"metadata"];
				[flacDecoder didChangeValueForKey:@"metadata"];
			}
		}
	}
}

void ErrorCallback(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data) {
	FlacDecoder *flacDecoder = (__bridge FlacDecoder *)client_data;
	if(status != FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC)
		flacDecoder->abortFlag = YES;
}

- (BOOL)open:(id<CogSource>)s {
	[self setSource:s];
	[self setSize:0];

	if([s seekable]) {
		[s seek:0 whence:SEEK_END];
		[self setSize:[s tell]];
		[s seek:0 whence:SEEK_SET];
	}

	// Must peek at stream! HTTP reader supports seeking within its buffer
	BOOL isOggFlac = NO;
	uint8_t buffer[4];
	[s read:buffer amount:4];
	[s seek:0 whence:SEEK_SET];
	if(memcmp(buffer, "OggS", 4) == 0) {
		isOggFlac = YES;
	}

	metaDict = [NSDictionary dictionary];
	icyMetaDict = [NSDictionary dictionary];
	albumArt = [NSData data];
	cuesheetFound = NO;
	cuesheet = @"";

	id dataStoreClass = NSClassFromString(@"RedundantPlaylistDataStore");
	dataStore = [[dataStoreClass alloc] init];

	decoder = FLAC__stream_decoder_new();
	if(decoder == NULL)
		return NO;

	if(![source seekable]) {
		FLAC__stream_decoder_set_md5_checking(decoder, false);
	}

	FLAC__stream_decoder_set_metadata_ignore_all(decoder);
	FLAC__stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_STREAMINFO);
	FLAC__stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_VORBIS_COMMENT);
	FLAC__stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_PICTURE);
	FLAC__stream_decoder_set_metadata_respond(decoder, FLAC__METADATA_TYPE_CUESHEET);

	abortFlag = NO;

	FLAC__StreamDecoderInitStatus ret;

	if(isOggFlac) {
		FLAC__stream_decoder_set_decode_chained_stream(decoder, true);

		ret = FLAC__stream_decoder_init_ogg_stream(decoder,
		                                           ReadCallback,
		                                           ([source seekable] ? SeekCallback : NULL),
		                                           ([source seekable] ? TellCallback : NULL),
		                                           ([source seekable] ? LengthCallback : NULL),
		                                           ([source seekable] ? EOFCallback : NULL),
		                                           WriteCallback,
		                                           MetadataCallback,
		                                           ErrorCallback,
		                                           (__bridge void *)(self));
	} else {
		ret = FLAC__stream_decoder_init_stream(decoder,
		                                       ReadCallback,
		                                       ([source seekable] ? SeekCallback : NULL),
		                                       ([source seekable] ? TellCallback : NULL),
		                                       ([source seekable] ? LengthCallback : NULL),
		                                       ([source seekable] ? EOFCallback : NULL),
		                                       WriteCallback,
		                                       MetadataCallback,
		                                       ErrorCallback,
		                                       (__bridge void *)(self));
	}

	if(ret != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
		return NO;
	}

	FLAC__stream_decoder_process_until_end_of_metadata(decoder);

	if(hasStreamInfo) {
		[self willChangeValueForKey:@"properties"];
		[self didChangeValueForKey:@"properties"];
	}

	blockBuffer = malloc(SAMPLE_blockBuffer_SIZE);

	frame = 0;
	seconds = 0.0;

	return YES;
}

- (AudioChunk *)readAudio {
	id audioChunkClass = NSClassFromString(@"AudioChunk");
	AudioChunk *chunk = nil;

	while (blockBufferFrames <= 0) {
		FLAC__StreamDecoderState state = FLAC__stream_decoder_get_state(decoder);
		if(state == FLAC__STREAM_DECODER_END_OF_STREAM) {
			return nil;
		} else if(state == FLAC__STREAM_DECODER_END_OF_LINK) {
			if(!FLAC__stream_decoder_finish_link(decoder)) {
				return nil;
			}
		}

		if(!FLAC__stream_decoder_process_single(decoder)) {
			return nil;
		}
	}

	if(blockBufferFrames > 0) {
		chunk = [[audioChunkClass alloc] initWithProperties:[self properties]];

		frame += blockBufferFrames;
		[chunk setStreamTimestamp:seconds];

		[chunk assignSamples:blockBuffer frameCount:blockBufferFrames];

		seconds += [chunk duration];

		blockBufferFrames = 0;
	}

	if(![source seekable]) {
		Class sourceClass = [source class];
		if([sourceClass isEqual:NSClassFromString(@"HTTPSource")]) {
			HTTPSource *httpSource = (HTTPSource *)source;
			NSMutableDictionary *_icyMetaDict = [[NSMutableDictionary alloc] init];
			if([httpSource hasMetadata]) {
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
				if(![_icyMetaDict isEqualToDictionary:icyMetaDict]) {
					icyMetaDict = _icyMetaDict;
					[self willChangeValueForKey:@"metadata"];
					[self didChangeValueForKey:@"metadata"];
				}
			}
		}
	}

	return chunk;
}

- (void)close {
	if(decoder) {
		FLAC__stream_decoder_finish(decoder);
		FLAC__stream_decoder_delete(decoder);
	}
	if(blockBuffer) {
		free(blockBuffer);
	}

	decoder = NULL;
	blockBuffer = NULL;
}

- (void)dealloc {
	[self close];
}

- (long)seek:(long)sample {
	if(!FLAC__stream_decoder_seek_absolute(decoder, sample))
		return -1;

	frame = sample;
	seconds = (double)(sample) / frequency;

	return sample;
}

// bs methods
- (char *)blockBuffer {
	return blockBuffer;
}
- (int)blockBufferFrames {
	return blockBufferFrames;
}
- (void)setBlockBufferFrames:(int)frames {
	blockBufferFrames = frames;
}

- (FLAC__StreamDecoder *)decoder {
	return decoder;
}

- (void)setSource:(id<CogSource>)s {
	source = s;
}
- (id<CogSource>)source {
	return source;
}

- (void)setEndOfStream:(BOOL)eos {
	endOfStream = eos;
}

- (BOOL)endOfStream {
	return endOfStream;
}

- (void)setSize:(long)size {
	fileSize = size;
}

- (NSDictionary *)properties {
	return @{ @"channels": @(channels),
		      @"channelConfig": @(channelConfig),
		      @"bitsPerSample": @(bitsPerSample),
		      @"sampleRate": @(frequency),
		      @"totalFrames": @(totalFrames),
		      @"seekable": @([source seekable]),
		      @"bitrate": @(fileSize ? (fileSize * 8 / ((totalFrames + (frequency / 2)) / frequency)) / 1000 : 0),
		      @"codec": @"FLAC",
		      @"endian": @"big",
		      @"encoding": @"lossless" };
}

- (NSDictionary *)metadata {
	NSDictionary *dict1 = @{ @"albumArt": albumArt, @"cuesheet": cuesheet };
	NSDictionary *dict2 = [dict1 dictionaryByMergingWith:metaDict];
	NSDictionary *dict3 = [dict2 dictionaryByMergingWith:icyMetaDict];
	return dict3;
}

+ (NSArray *)fileTypes {
	return @[@"flac"];
}

+ (NSArray *)mimeTypes {
	return @[@"audio/x-flac", @"application/ogg", @"audio/ogg"];
}

+ (float)priority {
	return 2.0;
}

+ (NSArray *)fileTypeAssociations {
	return @[
		@[@"FLAC Audio File", @"flac.icns", @"flac"]
	];
}

@end
