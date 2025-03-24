//
//  MP3Decoder.m
//  Cog
//
//  Created by Vincent Spader on 6/17/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#define MINIMP3_IMPLEMENTATION 1

#import "MP3Decoder.h"

#import "HTTPSource.h"

#import "Logging.h"

#import "id3tag.h"

#import "CVbriHeader.h"

@implementation MP3Decoder

static size_t mp3_read_callback(void *buf, size_t size, void *user_data) {
	id<CogSource> _file = (__bridge id<CogSource>)user_data;
	return [_file read:buf amount:size];
}

static int mp3_seek_callback(uint64_t position, void *user_data) {
	id<CogSource> _file = (__bridge id<CogSource>)user_data;
	return [_file seek:position whence:SEEK_SET] ? 0 : -1;
}

- (BOOL)open:(id<CogSource>)source {
	_source = source;

	seekable = [source seekable];

	size_t id3_length = 0;

	if(seekable) {
		// minimp3 already skips ID3v2, but we need to supplement it with support for
		// iTunes gapless info, which is stored in the ID3v2 tag
		uint8_t buffer[10];
		size_t buflen = [source read:&buffer[0] amount:10];

		[source seek:0 whence:SEEK_END];
		_fileSize = [source tell];
		[source seek:10 whence:SEEK_SET];

		if(10 <= buflen && 0x49 == buffer[0] && 0x44 == buffer[1] && 0x33 == buffer[2]) {
			id3_length = (((buffer[6] & 0x7F) << (3 * 7)) | ((buffer[7] & 0x7F) << (2 * 7)) |
						  ((buffer[8] & 0x7F) << (1 * 7)) | ((buffer[9] & 0x7F) << (0 * 7)));

			_foundID3v2 = YES;

			// Add 10 bytes for ID3 header
			id3_length += 10;

			void *tagBuffer = malloc(id3_length);
			if(!tagBuffer)
				return NO;

			memcpy(tagBuffer, &buffer[0], MIN(buflen, id3_length));

			long bufleft = id3_length - buflen;
			long tagRead = MIN(buflen, id3_length);

			while(bufleft > 0) {
				size_t bufRead = [source read:tagBuffer + tagRead amount:bufleft];
				if(bufRead <= 0) {
					free(tagBuffer);
					return NO;
				}
				tagRead += bufRead;
				if(bufRead < bufleft) {
					free(tagBuffer);
					return NO;
				}
				bufleft -= bufRead;
			}

			struct id3_tag *tag = id3_tag_parse(tagBuffer, id3_length);

			if(tag) {
				for(size_t i = 0; i < tag->nframes; ++i) {
					struct id3_frame *frame = tag->frames[i];
					if(!strcmp(frame->id, "COMM")) {
						union id3_field *field;
						const id3_ucs4_t *description;
						const id3_ucs4_t *value;

						field = id3_frame_field(frame, 2);
						description = id3_field_getstring(field);

						field = id3_frame_field(frame, 3);
						value = id3_field_getfullstring(field);

						if(description && value) {
							id3_utf8_t *description8 = id3_ucs4_utf8duplicate(description);
							if(!strcmp((const char *)description8, "iTunSMPB")) {
								id3_utf8_t *value8 = id3_ucs4_utf8duplicate(value);

								uint32_t zero, start_pad, end_pad;
								uint64_t last_eight_frames_offset;
								int64_t temp_duration;

								if(sscanf((const char *)value8, "%" PRIx32 " %" PRIx32 " %" PRIx32 " %" PRIx64 " %" PRIx32 " %" PRIx64, &zero, &start_pad, &end_pad, &temp_duration, &zero, &last_eight_frames_offset) == 6 &&
								   temp_duration >= 0 &&
								   start_pad <= (576 * 2 * 32) &&
								   end_pad <= (576 * 2 * 64) &&
								   (_fileSize && (last_eight_frames_offset < (_fileSize - id3_length)))) {
									if(end_pad >= 528 + 1) {
										_startPadding = start_pad + 528 + 1;
										_endPadding = end_pad - (528 + 1);
										// iTunes encodes the original length of the file here
										totalFrames = temp_duration;
										_foundiTunSMPB = YES;
									}
								}

								free(value8);
							}
							free(description8);
						}
					}
				}

				id3_tag_delete(tag);
			}

			free(tagBuffer);
		}

		_decoder_io.read = mp3_read_callback;
		_decoder_io.read_data = (__bridge void *)source;
		_decoder_io.seek = mp3_seek_callback;
		_decoder_io.seek_data = (__bridge void *)source;
		int error = mp3dec_ex_open_cb(&_decoder_ex, &_decoder_io, MP3D_SEEK_TO_SAMPLE);
		if(error)
			return NO;
		if(_foundiTunSMPB) {
			// start_delay is used for seeking, to_skip must be filled for the first packet decoded
			// and detected_samples will truncate the end padding
			_decoder_ex.start_delay = _decoder_ex.to_skip = _startPadding * _decoder_ex.info.channels;
			_decoder_ex.detected_samples = totalFrames * _decoder_ex.info.channels;
			_decoder_ex.samples = (totalFrames + _startPadding + _endPadding) * _decoder_ex.info.channels;
		}
		mp3d_sample_t *sample_ptr = NULL;
		size_t samples = mp3dec_ex_read_frame(&_decoder_ex, &sample_ptr, &_decoder_info, MINIMP3_MAX_SAMPLES_PER_FRAME);
		if(samples && sample_ptr) {
			samples_filled = samples / _decoder_info.channels;
			memcpy(&_decoder_buffer_output[0], sample_ptr, sizeof(mp3d_sample_t) * samples);
		}
		inputEOF = NO;
		if(!_foundiTunSMPB) {
			size_t samples = _decoder_ex.detected_samples;
			if(!samples) {
				samples = _decoder_ex.samples;
			}
			totalFrames = samples / _decoder_info.channels;
		}
		bitrate = (double)((_fileSize - id3_length) * 8) / ((double)totalFrames / (double)_decoder_info.hz) / 1000.0;
	} else {
		_decoder_buffer_filled = [source read:_decoder_buffer amount:INPUT_BUFFER_SIZE];
		inputEOF = _decoder_buffer_filled < INPUT_BUFFER_SIZE;
		mp3dec_init(&_decoder_ex.mp3d);
		int samples = mp3dec_decode_frame(&_decoder_ex.mp3d, _decoder_buffer, (int)_decoder_buffer_filled, &_decoder_buffer_output[0], &_decoder_info);
		if(!samples)
			return NO;
		size_t bytes_read = _decoder_info.frame_bytes;
		if(bytes_read >= _decoder_buffer_filled) {
			_decoder_buffer_filled = 0;
		} else {
			_decoder_buffer_filled -= bytes_read;
			memmove(&_decoder_buffer[0], &_decoder_buffer[bytes_read], _decoder_buffer_filled);
		}
		samples_filled = samples;
		bitrate = _decoder_info.bitrate_kbps;
	}

	[self syncFormat];

	// DLog(@"OPEN: %i", _firstFrame);

	seconds = 0.0;

	genre = @"";
	album = @"";
	artist = @"";
	title = @"";

	metadataUpdateInterval = sampleRate;
	metadataUpdateCount = 0;

	return YES;
}

- (BOOL)syncFormat {
	float _sampleRate = _decoder_info.hz;
	int _channels = _decoder_info.channels;
	int _layer = _decoder_info.layer;

	BOOL changed = (_sampleRate != sampleRate ||
	                _channels != channels ||
	                _layer != layer);

	if(changed) {
		sampleRate = _sampleRate;
		channels = _channels;
		layer = _layer;

		[self willChangeValueForKey:@"properties"];
		[self didChangeValueForKey:@"properties"];
	}

	return changed;
}

- (AudioChunk *)readAudio {
	id audioChunkClass = NSClassFromString(@"AudioChunk");
	AudioChunk *chunk = nil;

	for(;;) {
		long framesToCopy = samples_filled;

		if(framesToCopy) {
			chunk = [[audioChunkClass alloc] initWithProperties:[self properties]];
			[chunk setStreamTimestamp:seconds];
			[chunk assignSamples:_decoder_buffer_output frameCount:framesToCopy];
			seconds += [chunk duration];
			samples_filled = 0;
			_framesDecoded += framesToCopy;
			break;
		}

		if(seekable) {
			mp3d_sample_t *sample_ptr = NULL;
			size_t samples = mp3dec_ex_read_frame(&_decoder_ex, &sample_ptr, &_decoder_info, MINIMP3_MAX_SAMPLES_PER_FRAME);
			if(samples && sample_ptr) {
				samples_filled = samples / _decoder_info.channels;
				memcpy(&_decoder_buffer_output[0], sample_ptr, sizeof(mp3d_sample_t) * samples);
			} else {
				inputEOF = YES;
			}
		} else {
			size_t bytesRemain = INPUT_BUFFER_SIZE - _decoder_buffer_filled;
			ssize_t bytesRead = [_source read:&_decoder_buffer[_decoder_buffer_filled] amount:bytesRemain];
			if(bytesRead < 0 || bytesRead < bytesRemain) {
				inputEOF = YES;
			}
			if(bytesRead > 0) {
				_decoder_buffer_filled += bytesRead;
			}
			int samples = mp3dec_decode_frame(&_decoder_ex.mp3d, &_decoder_buffer[0], (int)_decoder_buffer_filled, &_decoder_buffer_output[0], &_decoder_info);
			if(samples > 0) {
				samples_filled = samples;
			} else {
				inputEOF = YES;
			}
		}

		[self syncFormat];

		if(inputEOF)
			break;
	}

	metadataUpdateCount += chunk ? [chunk frameCount] : 0;
	if(metadataUpdateCount >= metadataUpdateInterval) {
		metadataUpdateCount -= metadataUpdateInterval;
		[self updateMetadata];
	}

	// DLog(@"Read: %i/%i", bytesRead, size);
	return chunk;
}

- (void)close {
	if(seekable) {
		mp3dec_ex_close(&_decoder_ex);
	}
	if(_source) {
		_source = nil;
	}
}

- (long)seek:(long)frame {
	if(frame == _framesDecoded) {
		return frame;
	}

	if(frame > totalFrames)
		frame = totalFrames;

	if(seekable) {
		int error = mp3dec_ex_seek(&_decoder_ex, frame);
		if(error < 0) {
			return -1;
		}
		samples_filled = 0;
	} else {
		return -1;
	}

	seconds = (double)frame / (double)_decoder_info.hz;
	_framesDecoded = frame;

	return frame;
}

- (void)updateMetadata {
	NSString *_artist = artist;
	NSString *_album = album;
	NSString *_title = title;
	NSString *_genre = genre;

	Class sourceClass = [_source class];
	if([sourceClass isEqual:NSClassFromString(@"HTTPSource")]) {
		HTTPSource *httpSource = (HTTPSource *)_source;
		if([httpSource hasMetadata]) {
			NSDictionary *metadata = [httpSource metadata];
			_genre = [metadata valueForKey:@"genre"];
			_album = [metadata valueForKey:@"album"];
			_artist = [metadata valueForKey:@"artist"];
			_title = [metadata valueForKey:@"title"];
		}
	}

	if(![_artist isEqual:artist] ||
	   ![_album isEqual:album] ||
	   ![_title isEqual:title] ||
	   ![_genre isEqual:genre]) {
		artist = _artist;
		album = _album;
		title = _title;
		genre = _genre;
		if(![_source seekable]) {
			[self willChangeValueForKey:@"metadata"];
			[self didChangeValueForKey:@"metadata"];
		}
	}
}

- (NSDictionary *)properties {
	if(layer < 1 || layer > 3) return nil;
	const NSString *layers[3] = { @"MP1", @"MP2", @"MP3" };
	return @{ @"channels": @(channels),
		      @"bitsPerSample": @(32),
		      @"sampleRate": @(sampleRate),
		      @"floatingPoint": @(YES),
		      @"bitrate": @(bitrate),
		      @"totalFrames": @(totalFrames),
		      @"seekable": @(seekable),
		      @"codec": layers[layer - 1],
		      @"endian": @"host",
		      @"encoding": @"lossy" };
}

- (NSDictionary *)metadata {
	return @{ @"artist": artist, @"album": album, @"title": title, @"genre": genre };
}

+ (NSArray *)fileTypes {
	return @[@"mp3", @"m2a", @"mpa"];
}

+ (NSArray *)mimeTypes {
	return @[@"audio/mpeg", @"audio/x-mp3"];
}

+ (NSArray *)fileTypeAssociations {
	return @[@[@"MPEG Audio File", @"mp3.icns", @"mp3", @"m2a", @"mpa"]];
}

+ (float)priority {
	return 2.0;
}

@end
