//
//  MP3Decoder.m
//  Cog
//
//  Created by Vincent Spader on 6/17/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#define MINIMP3_IMPLEMENTATION 1

// minimp3 decodes a frame in large stack buffers by design; the frame size
// warning is for Cog's own code.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wframe-larger-than"
#import "MP3Decoder.h"
#pragma clang diagnostic pop

#import "Logging.h"

#import "NSDictionary+Optional.h"

#import "id3tag.h"

#import "CVbriHeader.h"

#import "TagLibID3v2Reader.h"

@implementation MP3Decoder

static size_t mp3_read_callback(void *buf, size_t size, void *user_data) {
	id<CogSource> _file = (__bridge id<CogSource>)user_data;
	return [_file read:buf amount:size];
}

static int mp3_seek_callback(uint64_t position, void *user_data) {
	id<CogSource> _file = (__bridge id<CogSource>)user_data;
	return [_file seek:position whence:SEEK_SET] ? 0 : -1;
}

// A chapter fragment is a plain decimal index, the whole of it: "2", not
// "2x" or "-1" (intValue would take the leading number of either). The
// digit cap keeps an absurd fragment from overflowing the parse; any real
// chapter count is far below it.
static BOOL chapterIndexFromFragment(NSString *fragment, NSUInteger *index) {
	NSUInteger length = [fragment length];
	if(!length || length > 9) return NO;
	for(NSUInteger i = 0; i < length; ++i) {
		unichar c = [fragment characterAtIndex:i];
		if(c < '0' || c > '9') return NO;
	}
	*index = (NSUInteger)[fragment integerValue];
	return YES;
}

- (BOOL)open:(id<CogSource>)source {
	// "file.mp3#2" is a chapter FFMPEGContainer listed. Its span comes from
	// the ID3v2 chapters, read below; a fragment that is not a decimal
	// index, or has no chapters to index, is refused, passing it to FFmpeg's
	// decoder, next in priority. (A cue sheet's tracks open their file
	// without a fragment.)
	NSString *fragment = [[source url] fragment];
	NSUInteger fragmentIndex = 0;
	BOOL wantsChapter = NO;
	if([fragment length]) {
		if(![source seekable]) return NO;
		if(!chapterIndexFromFragment(fragment, &fragmentIndex)) return NO;
		wantsChapter = YES;
	}

	_isChapter = NO;
	_chapters = nil;
	_chapterIndex = 0;
	_chapterStartFrame = 0;
	_fileTotalFrames = 0;
	_fileTitle = @"";

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
					} else if(!strcmp(frame->id, "TIT2")) {
						// The file's own title, which a chapter row carries
						// as its album (FFmpeg's rule). Read here, not via
						// metadataForTag:, which retitles it to album itself.
						// A text frame's string is a string list, not a full
						// string, so take its first entry.
						union id3_field *field = id3_frame_field(frame, 1);
						if(id3_field_getnstrings(field) > 0) {
							const id3_ucs4_t *value = id3_field_getstrings(field, 0);
							if(value) {
								id3_utf8_t *value8 = id3_ucs4_utf8duplicate(value);
								_fileTitle = [NSString stringWithUTF8String:(const char *)value8] ?: @"";
								free(value8);
							}
						}
					}
				}

				id3_tag_delete(tag);
			}

			if(wantsChapter) {
				Class tagReader = NSClassFromString(@"TagLibID3v2Reader");
				if(![tagReader respondsToSelector:@selector(chaptersForTag:)]) {
					free(tagBuffer);
					return NO;
				}
				NSArray *chapters = [tagReader chaptersForTag:[NSData dataWithBytes:tagBuffer length:id3_length]];
				// FFMPEGContainer numbers the rows as FFmpeg lists them, by
				// start, so the fragment indexes the same order.
				_chapters = [chapters sortedArrayWithOptions:NSSortStable usingComparator:^NSComparisonResult(NSDictionary *a, NSDictionary *b) {
					return [a[CogChapterStart] compare:b[CogChapterStart]];
				}];
			if(fragmentIndex >= [_chapters count]) {
				free(tagBuffer);
				return NO;
			}
			_chapterIndex = fragmentIndex;
			}

			free(tagBuffer);
		}

		if(wantsChapter && ![_chapters count]) return NO;

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
		if(!_foundiTunSMPB) {
			size_t samples = _decoder_ex.detected_samples;
			if(!samples) {
				samples = _decoder_ex.samples;
			}
			totalFrames = samples / _decoder_ex.info.channels;
		}
		bitrate = (double)((_fileSize - id3_length) * 8) / ((double)totalFrames / (double)_decoder_ex.info.hz) / 1000.0;
		if(wantsChapter) {
			// The chapter's span, positioned at its start; the first frame
			// decoded below is the chapter's own.
			long startFrame = 0, endFrame = 0;
			if(![self chapterSpanAtIndex:_chapterIndex start:&startFrame end:&endFrame fileFrames:totalFrames] ||
			   mp3dec_ex_seek(&_decoder_ex, (uint64_t)startFrame * _decoder_ex.info.channels) < 0) {
				mp3dec_ex_close(&_decoder_ex);
				return NO;
			}
			_fileTotalFrames = totalFrames;
			_chapterStartFrame = startFrame;
			totalFrames = endFrame - startFrame;
			_isChapter = YES;
		}
		mp3d_sample_t *sample_ptr = NULL;
		size_t samples = 0;
		int retry = 10;
		do {
			samples = mp3dec_ex_read_frame(&_decoder_ex, &sample_ptr, &_decoder_info, MINIMP3_MAX_SAMPLES_PER_FRAME);
		} while(!samples && --retry > 0);
		if(samples && sample_ptr) {
			samples_filled = samples / _decoder_info.channels;
			memcpy(&_decoder_buffer_output[0], sample_ptr, sizeof(mp3d_sample_t) * samples);
		} else {
			inputEOF = YES;
			mp3dec_ex_close(&_decoder_ex);
			return NO;
		}
		inputEOF = NO;
	} else {
		_decoder_buffer_filled = [source read:_decoder_buffer amount:MINIMP3_BUF_SIZE];
		inputEOF = _decoder_buffer_filled < MINIMP3_BUF_SIZE;
		mp3dec_init(&_decoder_ex.mp3d);
		int error = mp3dec_detect_buf(_decoder_buffer, _decoder_buffer_filled);
		if(error)
			return NO;

		for(;;) {
			error = mp3dec_decode_frame(&_decoder_ex.mp3d, _decoder_buffer, (int)_decoder_buffer_filled, &_decoder_buffer_output[0], &_decoder_info);
			if(_decoder_info.frame_bytes > _decoder_buffer_filled) {
				break;
			}
			_decoder_buffer_filled -= _decoder_info.frame_bytes;
			memmove(&_decoder_buffer[0], &_decoder_buffer[_decoder_info.frame_bytes], _decoder_buffer_filled);
			if(error) {
				break;
			}
		}
		if(!error) {
			return NO;
		}
		samples_filled = error;
		bitrate = _decoder_info.bitrate_kbps;
	}

	[self syncFormat];

	// DLog(@"OPEN: %i", _firstFrame);

	seconds = 0.0;

	genre = @"";
	album = @"";
	artist = @"";
	title = @"";
	if(_isChapter) {
		title = _chapters[_chapterIndex][CogChapterTitle] ?: @"";
		album = _fileTitle;
	}

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

		if(_isChapter) {
			// The chapter ends where the next one starts; the rest of a
			// frame that straddles the boundary belongs to the next track.
			long remaining = totalFrames - _framesDecoded;
			if(remaining <= 0) {
				inputEOF = YES;
				break;
			}
			if(framesToCopy > remaining) {
				framesToCopy = remaining;
			}
		}

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
			size_t bytesRemain = MINIMP3_BUF_SIZE - _decoder_buffer_filled;
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
			}
			if(_decoder_info.frame_bytes > _decoder_buffer_filled) {
				inputEOF = YES;
			} else {
				_decoder_buffer_filled -= _decoder_info.frame_bytes;
				memmove(&_decoder_buffer[0], &_decoder_buffer[_decoder_info.frame_bytes], _decoder_buffer_filled);
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

- (void)interrupt {
	if([_source respondsToSelector:@selector(interrupt)]) {
		[_source interrupt];
	}
}

- (long)seek:(long)frame {
	if(frame == _framesDecoded) {
		return frame;
	}

	if(frame > totalFrames)
		frame = totalFrames;

	if(seekable) {
		long absolute = _isChapter ? _chapterStartFrame + frame : frame;
		int error = mp3dec_ex_seek(&_decoder_ex, (uint64_t)absolute * _decoder_ex.info.channels);
		if(error < 0) {
			return -1;
		}
		samples_filled = 0;
		inputEOF = NO;
	} else {
		return -1;
	}

	seconds = (double)frame / (double)_decoder_info.hz;
	_framesDecoded = frame;

	return frame;
}

// The chapter's span in content frames at the file's own rate, its end
// clamped to the file's length. The tag keeps the times in milliseconds.
- (BOOL)chapterSpanAtIndex:(NSUInteger)index start:(long *)start end:(long *)end fileFrames:(long)fileFrames {
	if(index >= [_chapters count]) return NO;
	NSDictionary *chapter = _chapters[index];
	unsigned long long hz = (unsigned long long)_decoder_ex.info.hz;
	if(!hz) return NO;
	unsigned long long startMs = [chapter[CogChapterStart] unsignedLongLongValue];
	unsigned long long endMs = [chapter[CogChapterEnd] unsignedLongLongValue];
	long startFrame = (long)((startMs * hz + 500) / 1000);
	long endFrame = (long)((endMs * hz + 500) / 1000);
	if(endFrame > fileFrames) endFrame = fileFrames;
	if(startFrame >= endFrame) return NO;
	*start = startFrame;
	*end = endFrame;
	return YES;
}

// The next chapter of the same file: the feeder carries on without
// reopening. The seek is sample-accurate, and lands where the last chapter
// ended when the two are contiguous.
- (BOOL)setTrack:(NSURL *)track {
	if(!_isChapter) return NO;
	if(![[track path] isEqualToString:[[_source url] path]]) return NO;
	NSString *fragment = [track fragment];
	NSUInteger index = 0;
	if(!chapterIndexFromFragment(fragment, &index)) return NO;
	if(index >= [_chapters count]) return NO;
	long startFrame = 0, endFrame = 0;
	if(![self chapterSpanAtIndex:index start:&startFrame end:&endFrame fileFrames:_fileTotalFrames]) return NO;
	if(mp3dec_ex_seek(&_decoder_ex, (uint64_t)startFrame * _decoder_ex.info.channels) < 0) return NO;

	samples_filled = 0;
	_chapterIndex = index;
	_chapterStartFrame = startFrame;
	totalFrames = endFrame - startFrame;
	_framesDecoded = 0;
	seconds = 0.0;
	inputEOF = NO;
	title = _chapters[_chapterIndex][CogChapterTitle] ?: @"";

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];
	[self willChangeValueForKey:@"metadata"];
	[self didChangeValueForKey:@"metadata"];

	return YES;
}

- (void)updateMetadata {
	// A chapter's title and album are fixed at open; only a stream's
	// metadata changes mid-play, and a chapter is never a stream.
	if(_isChapter) return;
	NSString *_artist = artist;
	NSString *_album = album;
	NSString *_title = title;
	NSString *_genre = genre;

	if([_source conformsToProtocol:@protocol(CogHTTPSource)]) {
		id<CogHTTPSource> httpSource = (id<CogHTTPSource>)_source;
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
		      @"floatingPoint": @YES,
		      @"bitrate": @(bitrate),
		      @"totalFrames": @(totalFrames),
		      @"seekable": @(seekable),
		      @"codec": layers[layer - 1],
		      @"endian": @"host",
		      @"encoding": @"lossy" };
}

- (NSDictionary *)metadata {
	const NSString* keys[] = {
		@"artist",
		@"album",
		@"title",
		@"genre"
	};
	const id values[] = {
		artist,
		album,
		title,
		genre
	};
	return [NSDictionary initWithOptionalObjects:values forKeys:keys count:sizeof(keys) / sizeof(keys[0])];
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
