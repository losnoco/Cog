//
//  MADFile.m
//  Cog
//
//  Created by Vincent Spader on 6/17/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "MADDecoder.h"

#import "HTTPSource.h"

#import "Logging.h"

#import "id3tag.h"

#import <Accelerate/Accelerate.h>

#import "CVbriHeader.h"

@implementation MADDecoder

#define LAME_HEADER_SIZE ((8 * 5) + 4 + 4 + 8 + 32 + 16 + 16 + 4 + 4 + 8 + 12 + 12 + 8 + 8 + 2 + 3 + 11 + 32 + 32 + 32)

// From vbrheadersdk:
// ========================================
// A Xing header may be present in the ancillary
// data field of the first frame of an mp3 bitstream
// The Xing header (optionally) contains
//      frames      total number of audio frames in the bitstream
//      bytes       total number of bytes in the bitstream
//      toc         table of contents

// toc (table of contents) gives seek points
// for random access
// the ith entry determines the seek point for
// i-percent duration
// seek point in bytes = (toc[i]/256.0) * total_bitstream_bytes
// e.g. half duration seek point = (toc[50]/256.0) * total_bitstream_bytes

#define FRAMES_FLAG 0x0001
#define BYTES_FLAG 0x0002
#define TOC_FLAG 0x0004
#define VBR_SCALE_FLAG 0x0008

// Scan file quickly
- (void)bufferRefill:(struct mad_stream *)stream {
	long bytesToRead, bytesRemaining;

	if(NULL == stream->buffer || MAD_ERROR_BUFLEN == stream->error) {
		if(stream->next_frame) {
			bytesRemaining = stream->bufend - stream->next_frame;

			memmove(_inputBuffer, stream->next_frame, bytesRemaining);

			bytesToRead = INPUT_BUFFER_SIZE - bytesRemaining;
		} else {
			bytesToRead = INPUT_BUFFER_SIZE;
			bytesRemaining = 0;
		}

		// Read raw bytes from the MP3 file
		long bytesRead = [_source read:_inputBuffer + bytesRemaining amount:bytesToRead];

		if(bytesRead == 0) {
			memset(_inputBuffer + bytesRemaining + bytesRead, 0, MAD_BUFFER_GUARD);
			bytesRead += MAD_BUFFER_GUARD;
			inputEOF = YES;
		}

		mad_stream_buffer(stream, _inputBuffer, bytesRead + bytesRemaining);
		stream->error = MAD_ERROR_NONE;
	}
}

- (BOOL)scanFile {
	struct mad_stream stream;
	struct mad_frame frame;

	long framesDecoded = 0;
	int samplesPerMPEGFrame = 0;

	int id3_length = 0;

	mad_stream_init(&stream);
	mad_frame_init(&frame);

	[_source seek:0 whence:SEEK_END];
	_fileSize = [_source tell];
	[_source seek:0 whence:SEEK_SET];

	for(;;) {
		[self bufferRefill:&stream];

		if(mad_frame_decode(&frame, &stream) == -1) {
			if(MAD_RECOVERABLE(stream.error)) {
				// Prevent ID3 tags from reporting recoverable frame errors
				const uint8_t *buffer = stream.this_frame;
				unsigned long buflen = stream.bufend - stream.this_frame;

				if(10 <= buflen && 0x49 == buffer[0] && 0x44 == buffer[1] && 0x33 == buffer[2]) {
					id3_length = (((buffer[6] & 0x7F) << (3 * 7)) | ((buffer[7] & 0x7F) << (2 * 7)) |
					              ((buffer[8] & 0x7F) << (1 * 7)) | ((buffer[9] & 0x7F) << (0 * 7)));

					_foundID3v2 = YES;

					// Add 10 bytes for ID3 header
					id3_length += 10;

					void *tagBuffer = malloc(id3_length);
					if(!tagBuffer) return NO;

					memcpy(tagBuffer, &buffer[0], MIN(buflen, id3_length));

					long bufleft = id3_length - buflen;
					long tagRead = MIN(buflen, id3_length);

					while(bufleft > 0) {
						stream.error = MAD_ERROR_BUFLEN;
						stream.next_frame = NULL;
						[self bufferRefill:&stream];
						buffer = stream.this_frame;
						buflen = stream.bufend - stream.this_frame;
						memcpy(tagBuffer + tagRead, buffer, MIN(buflen, bufleft));
						tagRead += MIN(buflen, bufleft);
						bufleft -= buflen;
					}

					if(bufleft < 0) {
						mad_stream_skip(&stream, buflen + bufleft);
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
												totalFrames = temp_duration + _startPadding + _endPadding;
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
				} else if(stream.error == MAD_ERROR_BADDATAPTR) {
					goto framedecoded;
				}

				continue;
			} else if(stream.error == MAD_ERROR_BUFLEN && inputEOF) {
				break;
			} else if(stream.error == MAD_ERROR_BUFLEN) {
				continue;
			} else {
				// DLog(@"Unrecoverable error: %s", mad_stream_errorstr(&stream));
				break;
			}
		}
	framedecoded:
		framesDecoded++;

		if(framesDecoded == 1) {
			sampleRate = frame.header.samplerate;
			channels = MAD_NCHANNELS(&frame.header);

			if(MAD_FLAG_LSF_EXT & frame.header.flags || MAD_FLAG_MPEG_2_5_EXT & frame.header.flags) {
				switch(frame.header.layer) {
					case MAD_LAYER_I:
						samplesPerMPEGFrame = 384;
						layer = 1;
						break;
					case MAD_LAYER_II:
						samplesPerMPEGFrame = 1152;
						layer = 2;
						break;
					case MAD_LAYER_III:
						samplesPerMPEGFrame = 576;
						layer = 3;
						break;
				}
			} else {
				switch(frame.header.layer) {
					case MAD_LAYER_I:
						samplesPerMPEGFrame = 384;
						layer = 1;
						break;
					case MAD_LAYER_II:
						samplesPerMPEGFrame = 1152;
						layer = 2;
						break;
					case MAD_LAYER_III:
						samplesPerMPEGFrame = 1152;
						layer = 3;
						break;
				}
			}
			
			if(layer != 3) continue;

			const size_t ancillaryBitsRemaining = (stream.next_frame - stream.this_frame) * 8;
			
			static const int64_t xing_offtbl[2][2] = {{32, 17}, {17,9}};
			
			const int64_t xing_offset = xing_offtbl[!!(MAD_FLAG_LSF_EXT & frame.header.flags || MAD_FLAG_MPEG_2_5_EXT & frame.header.flags)][channels == 1] + 4; // Plus MPEG header
			
			size_t ancBitsRemainingXing = ancillaryBitsRemaining - xing_offset * 8;

			if(ancBitsRemainingXing >= 32) {
				const uint8_t *ptr = stream.this_frame + xing_offset;
				struct mad_bitptr bitptr;
				
				mad_bit_init(&bitptr, ptr);
				uint32_t magic = (uint32_t)mad_bit_read(&bitptr, 32);
				ancBitsRemainingXing -= 32;

				if('Xing' == magic || 'Info' == magic) {
					unsigned i;
					uint32_t flags = 0, frames = 0, bytes = 0, vbrScale = 0;

					if(32 > ancBitsRemainingXing)
						continue;

					flags = (uint32_t)mad_bit_read(&bitptr, 32);
					ancBitsRemainingXing -= 32;

					// 4 byte value containing total frames
					if(FRAMES_FLAG & flags) {
						if(32 > ancBitsRemainingXing)
							continue;

						frames = (uint32_t)mad_bit_read(&bitptr, 32);
						ancBitsRemainingXing -= 32;

						// Determine number of samples, discounting encoder delay and padding
						// Our concept of a frame is the same as CoreAudio's- one sample across all channels
						totalFrames = frames * samplesPerMPEGFrame;
						// DLog(@"TOTAL READ FROM XING");
					}

					// 4 byte value containing total bytes
					if(BYTES_FLAG & flags) {
						if(32 > ancBitsRemainingXing)
							continue;

						bytes = (uint32_t)mad_bit_read(&bitptr, 32);
						ancBitsRemainingXing -= 32;
					}

					// 100 bytes containing TOC information
					if(TOC_FLAG & flags) {
						if(8 * 100 > ancBitsRemainingXing)
							continue;

						for(i = 0; i < 100; ++i)
							/*_xingTOC[i] = */ mad_bit_read(&bitptr, 8);

						ancBitsRemainingXing -= (8 * 100);
					}

					// 4 byte value indicating encoded vbr scale
					if(VBR_SCALE_FLAG & flags) {
						if(32 > ancBitsRemainingXing)
							continue;

						vbrScale = (uint32_t)mad_bit_read(&bitptr, 32);
						ancBitsRemainingXing -= 32;
					}

					framesDecoded = frames;

					_foundXingHeader = YES;

					// Loook for the LAME header next
					// http://gabriel.mp3-tech.org/mp3infotag.html
					if(32 > ancBitsRemainingXing)
						continue;
					magic = (uint32_t)mad_bit_read(&bitptr, 32);

					ancBitsRemainingXing -= 32;

					if('LAME' == magic || 'Lavf' == magic || 'Lavc' == magic) {
						if(LAME_HEADER_SIZE > ancBitsRemainingXing)
							continue;

						/*unsigned char versionString [5 + 1];
						 memset(versionString, 0, 6);*/

						for(i = 0; i < 5; ++i)
							/*versionString[i] =*/mad_bit_read(&bitptr, 8);

						/*uint8_t infoTagRevision =*/mad_bit_read(&bitptr, 4);
						/*uint8_t vbrMethod =*/mad_bit_read(&bitptr, 4);

						/*uint8_t lowpassFilterValue =*/mad_bit_read(&bitptr, 8);

						/*float peakSignalAmplitude =*/mad_bit_read(&bitptr, 32);
						/*uint16_t radioReplayGain =*/mad_bit_read(&bitptr, 16);
						/*uint16_t audiophileReplayGain =*/mad_bit_read(&bitptr, 16);

						/*uint8_t encodingFlags =*/mad_bit_read(&bitptr, 4);
						/*uint8_t athType =*/mad_bit_read(&bitptr, 4);

						/*uint8_t lameBitrate =*/mad_bit_read(&bitptr, 8);

						_startPadding = mad_bit_read(&bitptr, 12);
						_endPadding = mad_bit_read(&bitptr, 12);

						_startPadding += 528 + 1; // MDCT/filterbank delay
						_endPadding -= 528 + 1;

						/*uint8_t misc =*/mad_bit_read(&bitptr, 8);

						/*uint8_t mp3Gain =*/mad_bit_read(&bitptr, 8);
						/*DLog(@"Gain: %i", mp3Gain);*/

						/*uint8_t unused =*/mad_bit_read(&bitptr, 2);
						/*uint8_t surroundInfo =*/mad_bit_read(&bitptr, 3);
						/*uint16_t presetInfo =*/mad_bit_read(&bitptr, 11);

						/*uint32_t musicGain =*/mad_bit_read(&bitptr, 32);

						/*uint32_t musicCRC =*/mad_bit_read(&bitptr, 32);

						/*uint32_t tagCRC =*/mad_bit_read(&bitptr, 32);

						ancBitsRemainingXing -= LAME_HEADER_SIZE;

						_foundLAMEHeader = YES;
						break;
					}
				}
			}
			
			const size_t vbri_offset = 4 + 32;
			
			size_t ancBitsRemainingVBRI = ancillaryBitsRemaining - vbri_offset * 8;
			
			if(ancBitsRemainingVBRI >= 32) {
				const uint8_t *ptr = stream.this_frame + vbri_offset;
				struct mad_bitptr bitptr;
				mad_bit_init(&bitptr, ptr);
				
				uint32_t magic = (uint32_t)mad_bit_read(&bitptr, 32);
				ancBitsRemainingVBRI -= 32;
			
				if('VBRI' == magic) {
					struct VbriHeader *vbri_header = 0;
					if(readVbriHeader(&vbri_header, mad_bit_nextbyte(&bitptr), ancBitsRemainingVBRI / 8) == 0) {
						uint32_t frames = VbriTotalFrames(vbri_header);
						totalFrames = frames * samplesPerMPEGFrame;
						_startPadding = 0;
						_endPadding = 0;

						_foundVBRIHeader = YES;
					}

					if(vbri_header) {
						freeVbriHeader(vbri_header);
					}

					break;
				}
			}
		} else if(_foundXingHeader || _foundiTunSMPB || _foundVBRIHeader) {
			break;
		} else if(framesDecoded > 1) {
			break;
		}
	}

	if(!_foundiTunSMPB && !_foundXingHeader && !_foundVBRIHeader) {
		// Now do CBR estimation instead of full file scanning
		size_t frameCount = (_fileSize - id3_length) / (stream.next_frame - stream.this_frame);
		mad_timer_t duration = frame.header.duration;
		mad_timer_multiply(&duration, frameCount);
		totalFrames = mad_timer_count(duration, sampleRate);
	}

	bitrate = ((double)((_fileSize - id3_length) * 8) / 1000.0) * (sampleRate / (double)totalFrames);

	mad_frame_finish(&frame);
	mad_stream_finish(&stream);

	[_source seek:0 whence:SEEK_SET];
	inputEOF = NO;

	DLog(@"Mad properties: %@", [self properties]);

	return YES;
}

- (BOOL)open:(id<CogSource>)source {
	_source = source;

	/* First the structures used by libmad must be initialized. */
	mad_stream_init(&_stream);
	mad_frame_init(&_frame);
	mad_synth_init(&_synth);

	_firstFrame = YES;
	_outputFrames = 0;
	_startPadding = 0;
	_endPadding = 0;
	// DLog(@"OPEN: %i", _firstFrame);

	inputEOF = NO;

	genre = @"";
	album = @"";
	artist = @"";
	title = @"";

	if(![_source seekable]) {
		// Decode the first frame to get the channels, samplerate, etc.
		int r;
		do {
			r = [self decodeMPEGFrame];
			DLog(@"Decoding first frame: %i", r);
		} while(r == 0);

		return (r == -1 ? NO : YES);
	}

	framesToSkip = 0;

	BOOL ret = [self scanFile];

	if(_foundLAMEHeader || _foundiTunSMPB) {
		framesToSkip = _startPadding;
	}

	return ret;
}

- (BOOL)writeOutput {
	unsigned long startingSample = 0;
	unsigned long sampleCount = _synth.pcm.length;

	// DLog(@"Position: %li/%li", _framesDecoded, totalFrames);
	// DLog(@"<%i, %i>", _startPadding, _endPadding);
	if(framesToSkip > 0) {
		startingSample = framesToSkip;
	}

	// DLog(@"Counts: %i, %i", startingSample, sampleCount);
	if(_foundLAMEHeader || _foundiTunSMPB) {
		// Past the end of the file.
		if(totalFrames - _endPadding <= _framesDecoded) {
			// DLog(@"End of file. Not writing.");
			return YES;
		}

		// Clip this for the following calculation, so this doesn't underflow
		// when seeking and skipping a lot of samples
		unsigned long startingSampleClipped = MIN(startingSample, sampleCount);

		// We are at the end of the file and need to read the last few frames
		if(_framesDecoded + (sampleCount - startingSampleClipped) > totalFrames - _endPadding) {
			// DLog(@"End of file. %li",  totalFrames - _endPadding - _framesDecoded);
			sampleCount = totalFrames - _endPadding - _framesDecoded + startingSample;
		}
	} else {
		// Past the end of the file.
		if(totalFrames <= _framesDecoded) {
			return YES;
		}
	}

	// We haven't even gotten to the start yet
	if(startingSample >= sampleCount) {
		// DLog(@"Skipping entire sample");
		_framesDecoded += sampleCount;
		framesToSkip -= sampleCount;
		return NO;
	}

	framesToSkip = 0;

	// DLog(@"Revised: %i, %i", startingSample, sampleCount);

	_framesDecoded += sampleCount;

	if(_outputFrames > 0) {
		DLog(@"LOSING FRAMES!");
	}
	_outputFrames = (sampleCount - startingSample);

	if(_currentOutputFrames < _outputFrames) {
		_outputBuffer = (float *)realloc(_outputBuffer, _outputFrames * channels * sizeof(float));
		_currentOutputFrames = _outputFrames;
	}

	int ch;

	// samples [0 ... n]
	for(ch = 0; ch < channels; ch++) {
		vDSP_vflt32(&_synth.pcm.samples[ch][startingSample], 1, &_outputBuffer[ch], channels, _outputFrames);
	}
	float scale = (float)MAD_F_ONE;
	vDSP_vsdiv(&_outputBuffer[0], 1, &scale, &_outputBuffer[0], 1, _outputFrames * channels);

	// Output to a file
	// FILE *f = fopen("data.raw", "a");
	// fwrite(_outputBuffer, channels * 2, _outputFrames, f);
	// fclose(f);

	return NO;
}

- (int)decodeMPEGFrame {
	if(_stream.buffer == NULL || _stream.error == MAD_ERROR_BUFLEN) {
		int inputToRead;
		int inputRemaining;

		if(_stream.next_frame != NULL) {
			inputRemaining = (int)(_stream.bufend - _stream.next_frame);

			memmove(_inputBuffer, _stream.next_frame, inputRemaining);

			inputToRead = INPUT_BUFFER_SIZE - inputRemaining;
		} else {
			inputToRead = INPUT_BUFFER_SIZE;
			inputRemaining = 0;
		}

		long inputRead = [_source read:_inputBuffer + inputRemaining amount:INPUT_BUFFER_SIZE - inputRemaining];
		if(inputRead == 0) {
			memset(_inputBuffer + inputRemaining + inputRead, 0, MAD_BUFFER_GUARD);
			inputRead += MAD_BUFFER_GUARD;
			inputEOF = YES;
		}

		mad_stream_buffer(&_stream, _inputBuffer, inputRead + inputRemaining);
		_stream.error = MAD_ERROR_NONE;
		// DLog(@"Read stream.");
	}

	BOOL skippingBadFrame = NO;

	if(mad_frame_decode(&_frame, &_stream) == -1) {
		if(_stream.error == MAD_ERROR_BADDATAPTR) {
			skippingBadFrame = YES;
		} else if(MAD_RECOVERABLE(_stream.error)) {
			const uint8_t *buffer = _stream.this_frame;
			unsigned long buflen = _stream.bufend - _stream.this_frame;
			uint32_t id3_length = 0;

			// No longer need ID3Tag framework
			if(10 <= buflen && 0x49 == buffer[0] && 0x44 == buffer[1] && 0x33 == buffer[2]) {
				id3_length = (((buffer[6] & 0x7F) << (3 * 7)) | ((buffer[7] & 0x7F) << (2 * 7)) |
				              ((buffer[8] & 0x7F) << (1 * 7)) | ((buffer[9] & 0x7F) << (0 * 7)));

				// Add 10 bytes for ID3 header
				id3_length += 10;

				mad_stream_skip(&_stream, id3_length);
			}

			DLog(@"recoverable error");
			return 0;
		} else if(MAD_ERROR_BUFLEN == _stream.error && inputEOF) {
			DLog(@"EOF");
			return -1;
		} else if(MAD_ERROR_BUFLEN == _stream.error) {
			// DLog(@"Bufferlen");
			return 0;
		} else {
			// DLog(@"Unrecoverable stream error: %s", mad_stream_errorstr(&_stream));
			return -1;
		}
	}

	if(!_firstFrame || !(_foundXingHeader && _foundVBRIHeader)) {
		signed long frameDuration = mad_timer_count(_frame.header.duration, sampleRate);
		if((framesToSkip - 1152 * 4) >= frameDuration) {
			framesToSkip -= frameDuration;
			_framesDecoded += frameDuration;
			return 0;
		}
	}

	// DLog(@"Decoded buffer.");
	if(!skippingBadFrame) {
		mad_synth_frame(&_synth, &_frame);
	}
	// DLog(@"first frame: %i", _firstFrame);
	if(_firstFrame) {
		_firstFrame = NO;

		if(![_source seekable]) {
			sampleRate = _frame.header.samplerate;
			channels = MAD_NCHANNELS(&_frame.header);

			switch(_frame.header.layer) {
				case MAD_LAYER_I:
					layer = 1;
					break;
				case MAD_LAYER_II:
					layer = 2;
					break;
				case MAD_LAYER_III:
					layer = 3;
					break;
				default:
					break;
			}

			[self willChangeValueForKey:@"properties"];
			[self didChangeValueForKey:@"properties"];
		}
		// DLog(@"FIRST FRAME!!! %i %i", _foundXingHeader, _foundLAMEHeader);
		if(_foundXingHeader || _foundVBRIHeader) {
			// DLog(@"Skipping xing header.");
			return 0;
		}
	} else if(skippingBadFrame) {
		return 0;
	}

	return 1;
}

- (BOOL)syncFormat {
	float _sampleRate = _frame.header.samplerate;
	int _channels = MAD_NCHANNELS(&_frame.header);
	int _layer = 3;

	switch(_frame.header.layer) {
		case MAD_LAYER_I:
			_layer = 1;
			break;
		case MAD_LAYER_II:
			_layer = 2;
			break;
		case MAD_LAYER_III:
			_layer = 3;
			break;
		default:
			break;
	}

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
	if(!_firstFrame)
		[self syncFormat];

	id audioChunkClass = NSClassFromString(@"AudioChunk");
	AudioChunk *chunk = nil;

	for(;;) {
		long framesToCopy = _outputFrames;

		if(framesToCopy) {
			chunk = [[audioChunkClass alloc] initWithProperties:[self properties]];
			[chunk assignSamples:_outputBuffer frameCount:framesToCopy];
			_outputFrames = 0;
			break;
		}

		int r = [self decodeMPEGFrame];
		// DLog(@"Decoding frame: %i", r);
		if(r == 0) // Recoverable error.
			continue;
		else if(r == -1) // Unrecoverable error
			break;

		if([self writeOutput]) {
			return nil;
		}
		// DLog(@"Wrote output");

		[self syncFormat];
	}

	[self updateMetadata];

	// DLog(@"Read: %i/%i", bytesRead, size);
	return chunk;
}

- (void)close {
	if(_source) {
		[_source close];
		_source = nil;
	}

	if(_outputBuffer) {
		free(_outputBuffer);
		_outputBuffer = NULL;
		_currentOutputFrames = 0;
	}

	mad_synth_finish(&_synth);
	mad_frame_finish(&_frame);
	mad_stream_finish(&_stream);
}

- (long)seek:(long)frame {
	if(frame == _framesDecoded) {
		return frame;
	}

	if(frame > totalFrames)
		frame = totalFrames;

	framesToSkip = 0;

	if(_foundLAMEHeader || _foundiTunSMPB) {
		if(_framesDecoded < _startPadding) {
			framesToSkip = _startPadding;
		}
	}

	if(frame < _framesDecoded) {
		_framesDecoded = 0;
		_firstFrame = YES;
		if(_foundLAMEHeader || _foundiTunSMPB)
			framesToSkip = _startPadding;
		[_source seek:0 whence:SEEK_SET];

		mad_stream_buffer(&_stream, NULL, 0);
	}

	framesToSkip += frame - _framesDecoded;

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
		      @"totalFrames": @(totalFrames - (_startPadding + _endPadding)),
		      @"seekable": @([_source seekable]),
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
