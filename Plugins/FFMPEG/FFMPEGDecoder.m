//
//  FFMPEGDecoder.m
//  FFMPEG
//
//  Created by Andre Reffhaug on 2/26/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "FFMPEGDecoder.h"

#import "NSDictionary+Merge.h"
#import "TagLibID3v2Reader.h"

#include <pthread.h>

#import "Logging.h"

#import "HTTPSource.h"

#define ST_BUFF 2048

@implementation FFMPEGReader

- (id)initWithFile:(id<CogSource>)f {
	self = [super init];
	if(self) {
		file = f;
		cachedSize = NO;
		size = 0;
	}
	return self;
}

- (id<CogSource>)file {
	return file;
}

- (int64_t)size {
	if(!cachedSize) {
		if([file seekable]) {
			int64_t curOffset = [file tell];
			[file seek:0 whence:SEEK_END];
			size = [file tell];
			[file seek:curOffset whence:SEEK_SET];
		} else {
			size = -1;
		}
		cachedSize = YES;
	}
	return size;
}

@end

int ffmpeg_read(void *opaque, uint8_t *buf, int buf_size) {
	FFMPEGReader *source = (__bridge FFMPEGReader*)opaque;
	long sizeRead = [[source file] read:buf amount:buf_size];
	if(sizeRead == 0) return AVERROR_EOF;
	return (int)sizeRead;
}

int ffmpeg_write(void *opaque, const uint8_t *buf, int buf_size) {
	return -1;
}

int64_t ffmpeg_seek(void *opaque, int64_t offset, int whence) {
	FFMPEGReader *source = (__bridge FFMPEGReader*)opaque;
	int64_t seekPos = 0;
	id<CogSource> file = [source file];
	size_t size = [file seekable] ? [source size] : [file tell];

	switch(whence) {
		case(AVSEEK_SIZE):
			return [source size];
		case(SEEK_SET):
			seekPos = offset;
			break;
		case(SEEK_CUR):
			seekPos = [file tell] + offset;
			break;
		case(SEEK_END):
			seekPos = [source size] - offset;
			break;
		default:
			return -1;
	}

	if(seekPos < 0 || seekPos > size) {
		return -1;
	}

	return [file seek:seekPos whence:SEEK_SET] ? [file tell] : -1;
}

@implementation FFMPEGDecoder

static uint8_t reverse_bits[0x100];

+ (void)initialize {
	if(self == [FFMPEGDecoder class]) {
		av_log_set_flags(AV_LOG_SKIP_REPEATED);
		av_log_set_level(AV_LOG_ERROR);

		for(int i = 0, j = 0; i < 0x100; i++) {
			reverse_bits[i] = (uint8_t)j;
			// "reverse-increment" of j
			for(int bitmask = 0x80;;) {
				if(((j ^= bitmask) & bitmask) != 0) break;
				if(bitmask == 1) break;
				bitmask >>= 1;
			}
		}
	}
}

- (id)init {
	self = [super init];
	if(self) {
		lastReadPacket = NULL;
		lastDecodedFrame = NULL;
		codecCtx = NULL;
		formatCtx = NULL;
		ioCtx = NULL;
		buffer = NULL;
	}
	return self;
}

- (BOOL)open:(id<CogSource>)s {
	char errDescr[4096];

	int errcode, i;
	AVStream *stream;

	source = s;

	formatCtx = NULL;
	totalFrames = 0;
	framesRead = 0;

	rawDSD = NO;

	isHLS = NO;

	// register all available codecs

	if([[source.url fragment] length] == 0)
		subsong = 0;
	else
		subsong = [[source.url fragment] intValue];

	NSURL *url = [s url];

	BOOL isHTTP = [[url scheme] isEqualToString:@"http"] ||
				  [[url scheme] isEqualToString:@"https"];
	BOOL isM3U = [[url pathExtension] isEqualToString:@"m3u8"];

	if(isHTTP && isM3U) {
		source = nil;
		[s close];

		isHLS = YES;

		formatCtx = avformat_alloc_context();
		if(!formatCtx) {
			ALog(@"Unable to allocate AVFormat context");
			return NO;
		}

		NSString *urlString = [url absoluteString];
		if((errcode = avformat_open_input(&formatCtx, [urlString UTF8String], NULL, NULL)) < 0) {
			av_strerror(errcode, errDescr, 4096);
			ALog(@"Error opening file, errcode = %d, error = %s", errcode, errDescr);
			return NO;
		}
	} else if(!isM3U) {
		buffer = av_malloc(32 * 1024);
		if(!buffer) {
			ALog(@"Out of memory!");
			return NO;
		}

		reader = [[FFMPEGReader alloc] initWithFile:source];

		ioCtx = avio_alloc_context(buffer, 32 * 1024, 0, (__bridge void *)reader, ffmpeg_read, ffmpeg_write, ffmpeg_seek);
		if(!ioCtx) {
			ALog(@"Unable to create AVIO context");
			return NO;
		}

		formatCtx = avformat_alloc_context();
		if(!formatCtx) {
			ALog(@"Unable to allocate AVFormat context");
			return NO;
		}

		formatCtx->pb = ioCtx;

		if((errcode = avformat_open_input(&formatCtx, "", NULL, NULL)) < 0) {
			av_strerror(errcode, errDescr, 4096);
			ALog(@"Error opening file, errcode = %d, error = %s", errcode, errDescr);
			return NO;
		}
	} else {
		return NO;
	}

	if((errcode = avformat_find_stream_info(formatCtx, NULL)) < 0) {
		av_strerror(errcode, errDescr, 4096);
		ALog(@"Can't find stream info, errcode = %d, error = %s", errcode, errDescr);
		return NO;
	}

	streamIndex = -1;
	metadataIndex = -1;
	attachedPicIndex = -1;
	AVCodecParameters *codecPar;

	NSMutableArray *pictures = [[NSMutableArray alloc] init];

	for(i = 0; i < formatCtx->nb_streams; i++) {
		stream = formatCtx->streams[i];
		codecPar = stream->codecpar;
		if(streamIndex < 0 && codecPar->codec_type == AVMEDIA_TYPE_AUDIO) {
			DLog(@"audio codec found");
			streamIndex = i;
		} else if(codecPar->codec_id == AV_CODEC_ID_TIMED_ID3) {
			metadataIndex = i;
		} else if(stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
			[pictures addObject:@(i)];
		} else {
			stream->discard = AVDISCARD_ALL;
		}
	}

	if(streamIndex < 0) {
		ALog(@"no audio codec found");
		return NO;
	}

	if([pictures count]) {
		if([pictures count] == 1) {
			attachedPicIndex = [pictures[0] intValue];
		} else {
			// Find the first attached picture with "front" or "cover" in its filename
			for(NSNumber *picture in pictures) {
				i = [picture intValue];
				stream = formatCtx->streams[i];
				AVDictionary *metadata = stream->metadata;
				AVDictionaryEntry *filename = av_dict_get(metadata, "filename", NULL, 0);
				if(filename) {
					if(strcasestr(filename->value, "front") ||
					   strcasestr(filename->value, "cover")) {
						attachedPicIndex = i;
						break;
					}
				}
			}
			// Or else find the first attached picture
			if(attachedPicIndex < 0) {
				attachedPicIndex = [pictures[0] intValue];
			}
		}
	}

	stream = formatCtx->streams[streamIndex];
	codecPar = stream->codecpar;

	codecCtx = avcodec_alloc_context3(NULL);
	if(!codecCtx) {
		ALog(@"could not allocate codec context");
		return NO;
	}

	if((errcode = avcodec_parameters_to_context(codecCtx, codecPar)) < 0) {
		av_strerror(errcode, errDescr, 4096);
		ALog(@"Can't copy codec parameters to context, errcode = %d, error = %s", errcode, errDescr);
		return NO;
	}

	enum AVCodecID codec_id = codecCtx->codec_id;
	const AVCodec *codec = NULL;
	AVDictionary *dict = NULL;

	switch(codec_id) {
		case AV_CODEC_ID_DSD_LSBF:
		case AV_CODEC_ID_DSD_MSBF:
		case AV_CODEC_ID_DSD_LSBF_PLANAR:
		case AV_CODEC_ID_DSD_MSBF_PLANAR:
			rawDSD = YES;
			rawDSDReverseBits = codec_id == AV_CODEC_ID_DSD_LSBF || codec_id == AV_CODEC_ID_DSD_LSBF_PLANAR;
			rawDSDPlanar = codec_id == AV_CODEC_ID_DSD_LSBF_PLANAR || codec_id == AV_CODEC_ID_DSD_MSBF_PLANAR;
			break;
		case AV_CODEC_ID_MP3:
			codec = avcodec_find_decoder_by_name("mp3float");
			break;
		case AV_CODEC_ID_MP2:
			codec = avcodec_find_decoder_by_name("mp2float");
			break;
		case AV_CODEC_ID_MP1:
			codec = avcodec_find_decoder_by_name("mp1float");
			break;
		case AV_CODEC_ID_AAC:
			codec = avcodec_find_decoder_by_name("libfdk_aac");
			av_dict_set_int(&dict, "drc_level", -2, 0); // disable DRC
			av_dict_set_int(&dict, "level_limit", 0, 0); // disable peak limiting
			break;
		case AV_CODEC_ID_ALAC:
			codec = avcodec_find_decoder_by_name("alac");
			break;
		case AV_CODEC_ID_AC3:
			codec = avcodec_find_decoder_by_name("ac3");
			break;
		case AV_CODEC_ID_EAC3:
			codec = avcodec_find_decoder_by_name("eac3");
			break;
		default:
			break;
	}

	if(!codec && !rawDSD)
		codec = avcodec_find_decoder(codec_id);

	if(@available(macOS 10.15, *)) {
	} else {
		if(codec && codec->name) {
			const char *name = codec->name;
			size_t name_len = strlen(name);
			if(name_len > 3) {
				name += name_len - 3;
				if(!strcmp(name, "_at")) {
					ALog(@"AudioToolbox decoder picked on old macOS, disabling: %s", codec->name);
					codec = NULL; // Disable AudioToolbox codecs on Mojave and older
				}
			}
		}
	}

	if(!codec && !rawDSD) {
		ALog(@"codec not found");
		av_dict_free(&dict);
		return NO;
	}

	if(!rawDSD && (errcode = avcodec_open2(codecCtx, codec, &dict)) < 0) {
		av_dict_free(&dict);
		av_strerror(errcode, errDescr, 4096);
		ALog(@"could not open codec, errcode = %d, error = %s", errcode, errDescr);
		return NO;
	}

	av_dict_free(&dict);

	// Bah, their skipping is broken
	if(!rawDSD) codecCtx->flags2 |= AV_CODEC_FLAG2_SKIP_MANUAL;

	lastDecodedFrame = av_frame_alloc();
	av_frame_unref(lastDecodedFrame);
	lastReadPacket = malloc(sizeof(AVPacket));
	av_new_packet(lastReadPacket, 0);
	readNextPacket = YES;
	bytesConsumedFromDecodedFrame = INT_MAX;
	seekFrame = -1;

	if(!rawDSD) {
		AVChannelLayout *layout = &codecCtx->ch_layout;
		frequency = codecCtx->sample_rate;
		channels = layout->nb_channels;
		channelConfig = (uint32_t)layout->u.mask;
		floatingPoint = NO;

		switch(codecCtx->sample_fmt) {
			case AV_SAMPLE_FMT_U8:
			case AV_SAMPLE_FMT_U8P:
				bitsPerSample = 8;
				break;

			case AV_SAMPLE_FMT_S16:
			case AV_SAMPLE_FMT_S16P:
				bitsPerSample = 16;
				break;

			case AV_SAMPLE_FMT_S32:
			case AV_SAMPLE_FMT_S32P:
				bitsPerSample = 32;
				break;

			case AV_SAMPLE_FMT_FLT:
			case AV_SAMPLE_FMT_FLTP:
				bitsPerSample = 32;
				floatingPoint = YES;
				break;

			case AV_SAMPLE_FMT_DBL:
			case AV_SAMPLE_FMT_DBLP:
				bitsPerSample = 64;
				floatingPoint = YES;
				break;

			default:
				return NO;
		}
	} else {
		AVChannelLayout *layout = &codecCtx->ch_layout;
		frequency = codecPar->sample_rate * 8;
		channels = layout->nb_channels;
		channelConfig = (uint32_t)layout->u.mask;
		bitsPerSample = 1;
		floatingPoint = NO;
	}

	lossy = NO;
	if(floatingPoint)
		lossy = YES;

	if(!floatingPoint) {
		switch(codec_id) {
			case AV_CODEC_ID_MP2:
			case AV_CODEC_ID_MP3:
			case AV_CODEC_ID_AAC:
			case AV_CODEC_ID_AC3:
			// case AV_CODEC_ID_DTS: // lossy will return float, caught above, lossless will be integer
			case AV_CODEC_ID_VORBIS:
			case AV_CODEC_ID_DVAUDIO:
			case AV_CODEC_ID_WMAV1:
			case AV_CODEC_ID_WMAV2:
			case AV_CODEC_ID_MACE3:
			case AV_CODEC_ID_MACE6:
			case AV_CODEC_ID_VMDAUDIO:
			case AV_CODEC_ID_MP3ADU:
			case AV_CODEC_ID_MP3ON4:
			case AV_CODEC_ID_WESTWOOD_SND1:
			case AV_CODEC_ID_GSM:
			case AV_CODEC_ID_QDM2:
			case AV_CODEC_ID_COOK:
			case AV_CODEC_ID_TRUESPEECH:
			case AV_CODEC_ID_SMACKAUDIO:
			case AV_CODEC_ID_QCELP:
			case AV_CODEC_ID_DSICINAUDIO:
			case AV_CODEC_ID_IMC:
			case AV_CODEC_ID_MUSEPACK7:
			case AV_CODEC_ID_MLP:
			case AV_CODEC_ID_GSM_MS:
			case AV_CODEC_ID_ATRAC3:
			case AV_CODEC_ID_NELLYMOSER:
			case AV_CODEC_ID_MUSEPACK8:
			case AV_CODEC_ID_SPEEX:
			case AV_CODEC_ID_WMAVOICE:
			case AV_CODEC_ID_WMAPRO:
			case AV_CODEC_ID_ATRAC3P:
			case AV_CODEC_ID_EAC3:
			case AV_CODEC_ID_SIPR:
			case AV_CODEC_ID_MP1:
			case AV_CODEC_ID_TWINVQ:
			case AV_CODEC_ID_MP4ALS:
			case AV_CODEC_ID_ATRAC1:
			case AV_CODEC_ID_BINKAUDIO_RDFT:
			case AV_CODEC_ID_BINKAUDIO_DCT:
			case AV_CODEC_ID_AAC_LATM:
			case AV_CODEC_ID_QDMC:
			case AV_CODEC_ID_CELT:
			case AV_CODEC_ID_G723_1:
			case AV_CODEC_ID_G729:
			case AV_CODEC_ID_8SVX_EXP:
			case AV_CODEC_ID_8SVX_FIB:
			case AV_CODEC_ID_BMV_AUDIO:
			case AV_CODEC_ID_RALF:
			case AV_CODEC_ID_IAC:
			case AV_CODEC_ID_ILBC:
			case AV_CODEC_ID_OPUS:
			case AV_CODEC_ID_COMFORT_NOISE:
			case AV_CODEC_ID_METASOUND:
			case AV_CODEC_ID_PAF_AUDIO:
			case AV_CODEC_ID_ON2AVC:
			case AV_CODEC_ID_DSS_SP:
			case AV_CODEC_ID_CODEC2:
			case AV_CODEC_ID_FFWAVESYNTH:
			case AV_CODEC_ID_SONIC:
			case AV_CODEC_ID_SONIC_LS:
			case AV_CODEC_ID_EVRC:
			case AV_CODEC_ID_SMV:
			case AV_CODEC_ID_4GV:
			case AV_CODEC_ID_INTERPLAY_ACM:
			case AV_CODEC_ID_XMA1:
			case AV_CODEC_ID_XMA2:
			case AV_CODEC_ID_ATRAC3AL:
			case AV_CODEC_ID_ATRAC3PAL:
			case AV_CODEC_ID_DOLBY_E:
			case AV_CODEC_ID_APTX:
			case AV_CODEC_ID_SBC:
			case AV_CODEC_ID_ATRAC9:
			case AV_CODEC_ID_HCOM:
			case AV_CODEC_ID_ACELP_KELVIN:
			case AV_CODEC_ID_MPEGH_3D_AUDIO:
			case AV_CODEC_ID_SIREN:
			case AV_CODEC_ID_HCA:
			case AV_CODEC_ID_FASTAUDIO:
				lossy = YES;
				break;

			default:
				break;
		}
	}

	// totalFrames = codecCtx->sample_rate * ((float)formatCtx->duration/AV_TIME_BASE);
	AVRational tb = { .num = 1, .den = codecCtx->sample_rate };
	totalFrames = isHLS ? 0 : av_rescale_q(formatCtx->duration, AV_TIME_BASE_Q, tb);
	bitrate = (int)((codecCtx->bit_rate) / 1000);
	framesRead = 0;
	endOfStream = NO;
	endOfAudio = NO;

	sampleBufferSize = MAX(codecCtx->sample_rate / 200, 1024); // Minimum 1024 samples, or maximum 5 milliseconds
	sampleBufferSize *= rawDSD ? channels : channels * (bitsPerSample / 8);
	sampleBuffer = av_malloc(sampleBufferSize);
	if(!sampleBuffer) {
		ALog(@"Out of memory!");
		return NO;
	}

	metadataUpdateInterval = codecCtx->sample_rate;
	metadataUpdateCount = 0;

	if(rawDSD) {
		totalFrames *= 8;
	}

	if(!isHLS) {
		if(stream->start_time && stream->start_time != AV_NOPTS_VALUE)
			skipSamples = av_rescale_q(stream->start_time, stream->time_base, tb);
		if(skipSamples < 0)
			skipSamples = 0;
	} else {
		skipSamples = 0;
	}

	if(subsong < formatCtx->nb_chapters) {
		AVChapter *chapter = formatCtx->chapters[subsong];
		startTime = av_rescale_q(chapter->start, chapter->time_base, tb);
		endTime = av_rescale_q(chapter->end, chapter->time_base, tb);
		skipSamples = startTime;
		totalFrames = endTime - startTime;
	}

	seekFrame = skipSamples; // Skip preroll if necessary

	if(totalFrames < 0)
		totalFrames = 0;

	seekable = !isHLS && [s seekable];

	seekedToStart = !seekable;

	id3Metadata = [[NSDictionary alloc] init];
	metaDict = [NSDictionary dictionary];
	albumArt = [NSData data];
	metadataUpdated = NO;
	[self updateMetadata];

	prebufferedChunk = nil;

	if(attachedPicIndex >= 0) {
		prebufferedChunk = [self readAudio];
	}

	return YES;
}

- (void)close {
	if(lastReadPacket) {
		av_packet_unref(lastReadPacket);
		free(lastReadPacket);
		lastReadPacket = NULL;
	}

	if(lastDecodedFrame) {
		av_frame_unref(lastDecodedFrame);
		av_freep(&lastDecodedFrame);
	}

	if(codecCtx) {
		avcodec_free_context(&codecCtx);
	}

	if(formatCtx) {
		avformat_close_input(&formatCtx);
	}

	if(ioCtx) {
		buffer = ioCtx->buffer;
		avio_context_free(&ioCtx);
	}

	if(sampleBuffer) {
		av_freep(&sampleBuffer);
	}

	if(buffer) {
		av_freep(&buffer);
	}
}

- (void)dealloc {
	[self close];
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

- (void)updateMetadata {
	NSMutableDictionary *_metaDict = [[NSMutableDictionary alloc] init];
	const AVDictionaryEntry *tag = NULL;
	for(size_t i = 0; i < 4; ++i) {
		AVDictionary *metadata;
		if(i == 0) {
			metadata = formatCtx->metadata;
			if(!metadata) continue;
		} else if(i == 1) {
			if(formatCtx->nb_programs > 0) {
				AVProgram *program = formatCtx->programs[0];
				if(!program) continue;
				metadata = program->metadata;
				if(!metadata) continue;
			} else {
				continue;
			}
		} else if(i == 2) {
			AVStream *stream = formatCtx->streams[streamIndex];
			metadata = stream->metadata;
			if(!metadata) continue;
		} else {
			if(subsong < formatCtx->nb_chapters) {
				metadata = formatCtx->chapters[subsong]->metadata;
				if(!metadata) continue;
			} else {
				break;
			}
		}
		tag = NULL;
		while((tag = av_dict_iterate(metadata, tag))) {
			@autoreleasepool {
				if(!strcasecmp(tag->key, "streamtitle")) {
					NSString *artistTitle = guess_encoding_of_string(tag->value);
					NSArray *splitValues = [artistTitle componentsSeparatedByString:@" - "];
					NSString *_artist = @"";
					NSString *_title = [splitValues objectAtIndex:0];
					if([splitValues count] > 1) {
						_artist = _title;
						_title = [splitValues objectAtIndex:1];
						setDictionary(_metaDict, @"artist", _artist);
						setDictionary(_metaDict, @"title", _title);
					} else {
						setDictionary(_metaDict, @"title", _title);
					}
				} else if(!strcasecmp(tag->key, "unsynced lyrics") ||
						  !strcasecmp(tag->key, "lyrics")) {
					setDictionary(_metaDict, @"unsyncedlyrics", guess_encoding_of_string(tag->value));
				} else if(!strcasecmp(tag->key, "icy-url")) {
					setDictionary(_metaDict, @"album", guess_encoding_of_string(tag->value));
				} else if(!strcasecmp(tag->key, "icy-genre")) {
					setDictionary(_metaDict, @"genre", guess_encoding_of_string(tag->value));
				} else if(!strcasecmp(tag->key, "title")) {
					NSString *_tag = guess_encoding_of_string(tag->value);
					if(i == 0 && formatCtx->nb_chapters > 1) {
						setDictionary(_metaDict, @"album", _tag);
					} else {
						setDictionary(_metaDict, @"title", _tag);
					}
				} else if(!strcasecmp(tag->key, "variant_bitrate")) {
					NSString *bitrate = guess_encoding_of_string(tag->value);
					long nbitrate = [bitrate integerValue];
					setDictionary(_metaDict, @"bitrate", [@(nbitrate / 1000) stringValue]);
				} else if(!strcasecmp(tag->key, "date_recorded")) {
					setDictionary(_metaDict, @"date", guess_encoding_of_string(tag->value));
				} else if(!strcasecmp(tag->key, "replaygain_gain")) {
					// global or chapter gain
					NSString *tagName;
					if(i == 0)
						tagName = @"replaygain_album_gain";
					else
						tagName = @"replaygain_track_gain";
					setDictionary(_metaDict, tagName, guess_encoding_of_string(tag->value));
				} else if(!strcasecmp(tag->key, "replaygain_peak")) {
					// global or chapter peak
					NSString *tagName;
					if(i == 0)
						tagName = @"replaygain_album_peak";
					else
						tagName = @"replaygain_track_peak";
					setDictionary(_metaDict, tagName, guess_encoding_of_string(tag->value));
				} else if(!strcasecmp(tag->key, "iTunNORM")) {
					NSString *tagString = guess_encoding_of_string(tag->value);
					NSArray *tag = [tagString componentsSeparatedByString:@" "];
					NSMutableArray *wantedTag = [[NSMutableArray alloc] init];
					for(size_t i = 0; i < [tag count]; ++i) {
						NSString *tagValue = tag[i];
						if([tagValue length] == 8) {
							[wantedTag addObject:tagValue];
						}
					}
					if([wantedTag count] >= 10) {
						NSScanner *scanner1 = [NSScanner scannerWithString:wantedTag[0]];
						NSScanner *scanner2 = [NSScanner scannerWithString:wantedTag[1]];
						unsigned int hexvalue1 = 0, hexvalue2 = 0;
						[scanner1 scanHexInt:&hexvalue1];
						[scanner2 scanHexInt:&hexvalue2];
						float volume1 = -log10((double)(hexvalue1) / 1000) * 10;
						float volume2 = -log10((double)(hexvalue2) / 1000) * 10;
						float volumeToUse = MIN(volume1, volume2);
						NSNumber *_volumeScale = @(pow(10, volumeToUse / 20));
						setDictionary(_metaDict, @"volume", [_volumeScale stringValue]);
					}
				} else {
					setDictionary(_metaDict, guess_encoding_of_string(tag->key), guess_encoding_of_string(tag->value));
				}
			}
		}
	}

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
					[_metaDict setObject:@[_genre] forKey:@"genre"];
				}
				if(_album && [_album length]) {
					[_metaDict setObject:@[_album] forKey:@"album"];
				}
				if(_artist && [_artist length]) {
					[_metaDict setObject:@[_artist] forKey:@"artist"];
				}
				if(_title && [_title length]) {
					[_metaDict setObject:@[_title] forKey:@"title"];
				}
			}
		}
	}

	if(![_metaDict isEqualToDictionary:metaDict]) {
		@autoreleasepool {
			metaDict = _metaDict;
		}
		if(!seekable) {
			[self willChangeValueForKey:@"metadata"];
			[self didChangeValueForKey:@"metadata"];
		} else {
			metadataUpdated = YES;
		}
	}
}

- (void)updateID3Metadata {
	NSData *tag = [NSData dataWithBytes:lastReadPacket->data length:lastReadPacket->size];
	Class tagReader = NSClassFromString(@"TagLibID3v2Reader");
	if(tagReader && [tagReader respondsToSelector:@selector(metadataForTag:)]) {
		NSDictionary *_id3Metadata = [tagReader metadataForTag:tag];
		if(![_id3Metadata isEqualToDictionary:id3Metadata]) {
			id3Metadata = _id3Metadata;
			[self willChangeValueForKey:@"metadata"];
			[self didChangeValueForKey:@"metadata"];
		}
	}
}

- (void)updateArtwork {
	NSData *_albumArt = [NSData dataWithBytes:lastReadPacket->data length:lastReadPacket->size];
	if(![_albumArt isEqualToData:albumArt]) {
		albumArt = _albumArt;
		if(!seekable) {
			[self willChangeValueForKey:@"metadata"];
			[self didChangeValueForKey:@"metadata"];
		} else {
			metadataUpdated = YES;
		}
	}
}

- (AudioChunk *)readAudio {
	char errDescr[4096];

	if(!seekedToStart) {
		[self seek:0];
	}

	if(prebufferedChunk) {
		// A bit of ignored read-ahead to support embedded artwork
		AudioChunk *chunk = prebufferedChunk;
		prebufferedChunk = nil;
		return chunk;
	}

	if(totalFrames && framesRead >= totalFrames)
		return nil;

	int frameSize = rawDSD ? channels : channels * (bitsPerSample / 8);
	int bytesToRead = sampleBufferSize;
	int bytesRead = 0;

	void *buf = (void *)sampleBuffer;

	int dataSize = 0;

	int seekBytesSkip = 0;

	int errcode = 0;

	int8_t *targetBuf = (int8_t *)buf;
	memset(buf, 0, bytesToRead);

	while(bytesRead < bytesToRead) {
		// buffer size needed to hold decoded samples, in bytes
		int planeSize;
		int planar;
		if(!rawDSD) {
			planar = av_sample_fmt_is_planar(codecCtx->sample_fmt);
			dataSize = av_samples_get_buffer_size(&planeSize, channels,
			                                      lastDecodedFrame->nb_samples,
			                                      codecCtx->sample_fmt, 1);
		} else {
			planar = 0;
			dataSize = endOfStream ? 0 : lastReadPacket->size;
		}

		if(dataSize < 0)
			dataSize = 0;

		while(readNextPacket && !endOfAudio) {
			// consume next chunk of encoded data from input stream
			if(!endOfStream) {
				av_packet_unref(lastReadPacket);
				if((errcode = av_read_frame(formatCtx, lastReadPacket)) < 0) {
					if(errcode == AVERROR_EOF) {
						DLog(@"End of stream");
						endOfStream = YES;
					}
					if(formatCtx->pb && formatCtx->pb->error) break;
				}

				if(lastReadPacket->stream_index == metadataIndex) {
					[self updateID3Metadata];
					continue;
				} else if(lastReadPacket->stream_index == attachedPicIndex) {
					[self updateArtwork];
					continue;
				}

				if(lastReadPacket->stream_index != streamIndex)
					continue;
			}

			if(!rawDSD) {
				if((errcode = avcodec_send_packet(codecCtx, endOfStream ? NULL : lastReadPacket)) < 0) {
					if(errcode == AVERROR(EAGAIN)) {
						continue;
					}
				}
			}

			readNextPacket = NO; // we probably won't need to consume another chunk
		}

		if(dataSize <= bytesConsumedFromDecodedFrame) {
			if(endOfStream && endOfAudio)
				break;

			bytesConsumedFromDecodedFrame = 0;

			if(!rawDSD) {
				if((errcode = avcodec_receive_frame(codecCtx, lastDecodedFrame)) < 0) {
					if(errcode == AVERROR_EOF) {
						endOfAudio = YES;
						break;
					} else if(errcode == AVERROR(EAGAIN)) {
						// Read another packet
						readNextPacket = YES;
						continue;
					} else {
						av_strerror(errcode, errDescr, 4096);
						ALog(@"Error receiving frame, errcode = %d, error = %s", errcode, errDescr);
						return 0;
					}
				}

				// Something has been successfully decoded
				dataSize = av_samples_get_buffer_size(&planeSize, channels,
				                                      lastDecodedFrame->nb_samples,
				                                      codecCtx->sample_fmt, 1);
			} else {
				dataSize = lastReadPacket->size;
				if(endOfStream) {
					endOfAudio = YES;
					break;
				} else if(dataSize <= bytesConsumedFromDecodedFrame) {
					readNextPacket = YES;
					continue;
				}

				if(rawDSDPlanar) {
					uint8_t tempBuf[dataSize];
					size_t samples = dataSize / channels;
					uint8_t *packetData = lastReadPacket->data;
					for(size_t i = 0; i < samples; ++i) {
						for(size_t j = 0; j < channels; ++j) {
							tempBuf[i * channels + j] = packetData[j * samples + i];
						}
					}
					memmove(packetData, tempBuf, sizeof(tempBuf));
				}

				if(rawDSDReverseBits) {
					uint8_t *packetData = lastReadPacket->data;
					for(size_t i = 0; i < dataSize; ++i) {
						packetData[i] = reverse_bits[packetData[i]];
					}
				}
			}

			if(dataSize < 0)
				dataSize = 0;

			// FFmpeg seeking by packet is usually inexact, so skip up to
			// target sample using packet timestamp
			// New: Moved here, because sometimes preroll packets also
			// trigger EAGAIN above, so ask for the next packet's timestamp
			// instead
			if(seekFrame >= 0 && errcode >= 0) {
				DLog(@"Seeking to frame %lld", seekFrame);
				AVRational tb = { .num = 1, .den = codecCtx->sample_rate };
				int64_t packetBeginFrame = av_rescale_q(
				lastReadPacket->dts,
				formatCtx->streams[streamIndex]->time_base,
				tb);

				if(packetBeginFrame < seekFrame) {
					seekBytesSkip += (int)((seekFrame - packetBeginFrame) * frameSize);
				}

				seekFrame = -1;
			}

			int minSkipped = FFMIN(dataSize, seekBytesSkip);
			bytesConsumedFromDecodedFrame += minSkipped;
			seekBytesSkip -= minSkipped;
		}

		if(!rawDSD) {
			AVChannelLayout *layout = &codecCtx->ch_layout;
			int _channels = layout->nb_channels;
			uint32_t _channelConfig = (uint32_t)layout->u.mask;
			float _frequency = codecCtx->sample_rate;

			if(_channels != channels ||
			   _channelConfig != channelConfig ||
			   _frequency != frequency) {
				if(bytesRead > 0) {
					break;
				} else {
					channels = _channels;
					channelConfig = _channelConfig;
					frequency = _frequency;
					[self willChangeValueForKey:@"properties"];
					[self didChangeValueForKey:@"properties"];
				}
			}
		}

		int toConsume = FFMIN((dataSize - bytesConsumedFromDecodedFrame), (bytesToRead - bytesRead));

		// copy decoded samples to Cog's buffer
		if(rawDSD) {
			memmove(targetBuf + bytesRead, (lastReadPacket->data + bytesConsumedFromDecodedFrame), toConsume);
		} else if(!planar || channels == 1) {
			memmove(targetBuf + bytesRead, (lastDecodedFrame->data[0] + bytesConsumedFromDecodedFrame), toConsume);
		} else {
			uint8_t *out = (uint8_t *)targetBuf + bytesRead;
			int bytesPerSample = bitsPerSample / 8;
			int bytesConsumedPerPlane = bytesConsumedFromDecodedFrame / channels;
			int toConsumePerPlane = toConsume / channels;
			for(int s = 0; s < toConsumePerPlane; s += bytesPerSample) {
				for(int ch = 0; ch < channels; ++ch) {
					memcpy(out, lastDecodedFrame->extended_data[ch] + bytesConsumedPerPlane + s, bytesPerSample);
					out += bytesPerSample;
				}
			}
		}

		bytesConsumedFromDecodedFrame += toConsume;
		bytesRead += toConsume;

		if(rawDSD && bytesConsumedFromDecodedFrame == dataSize) {
			av_packet_unref(lastReadPacket);
		}
	}

	int framesReadNow = bytesRead / frameSize;
	if(totalFrames && (framesRead + framesReadNow > totalFrames))
		framesReadNow = (int)(totalFrames - framesRead);

	double streamTimestamp = (double)(framesRead * (rawDSD ? 8UL : 1UL)) / frequency;

	framesRead += framesReadNow;

	metadataUpdateCount += framesReadNow;
	if(metadataUpdateCount >= metadataUpdateInterval) {
		metadataUpdateCount -= metadataUpdateInterval;
		[self updateMetadata];
	}

	id audioChunkClass = NSClassFromString(@"AudioChunk");
	AudioChunk *chunk = [[audioChunkClass alloc] initWithProperties:[self properties]];
	[chunk setStreamTimestamp:streamTimestamp];
	[chunk assignSamples:sampleBuffer frameCount:framesReadNow];

	return chunk;
}

- (long)seek:(long)frame {
	if(!totalFrames)
		return -1;

	seekedToStart = YES;

	prebufferedChunk = nil;

	if(frame >= totalFrames) {
		framesRead = totalFrames;
		endOfStream = YES;
		endOfAudio = YES;
		return -1;
	}
	if(rawDSD) frame /= 8;
	AVRational tb = { .num = 1, .den = codecCtx->sample_rate };
	int64_t ts = av_rescale_q(frame + skipSamples, tb, formatCtx->streams[streamIndex]->time_base);
	int ret = avformat_seek_file(formatCtx, streamIndex, ts - 1000, ts, ts, 0);
	if(!rawDSD)
		avcodec_flush_buffers(codecCtx);
	else
		av_packet_unref(lastReadPacket);
	if(ret < 0) {
		framesRead = totalFrames;
		endOfStream = YES;
		endOfAudio = YES;
		return -1;
	}
	readNextPacket = YES; // so we immediately read next packet
	bytesConsumedFromDecodedFrame = INT_MAX; // so we immediately begin decoding next frame
	framesRead = frame;
	if(rawDSD) framesRead *= 8;
	seekFrame = frame + skipSamples;
	endOfStream = NO;
	endOfAudio = NO;

	if(rawDSD)
		return frame * 8;
	else
		return frame;
}

- (NSDictionary *)properties {
	return @{ @"channels": @(channels),
		      @"channelConfig": @(channelConfig),
		      @"bitsPerSample": @(bitsPerSample),
		      @"unSigned": @(bitsPerSample == 8),
		      @"sampleRate": @(frequency),
		      @"floatingPoint": @(floatingPoint),
		      @"totalFrames": @(totalFrames),
		      @"bitrate": @(bitrate),
		      @"seekable": @(seekable),
		      @"codec": guess_encoding_of_string(avcodec_get_name(codecCtx->codec_id)),
		      @"endian": @"host",
		      @"encoding": lossy ? @"lossy" : @"lossless" };
}

- (NSDictionary *)metadata {
	NSDictionary *dict1 = @{ @"albumArt": albumArt };
	NSDictionary *dict2 = [dict1 dictionaryByMergingWith:metaDict];
	NSDictionary *dict3 = [dict2 dictionaryByMergingWith:id3Metadata];
	return dict3;
}

+ (NSArray *)fileTypes {
	return @[@"wma", @"asf", @"tak", @"mp4", @"m4a", @"m4b", @"m4r", @"aac", @"mp3", @"mp2", @"m2a", @"mpa", @"ape", @"ac3", @"dts", @"dtshd", @"wav", @"tta", @"vqf", @"vqe", @"vql", @"ra", @"rm", @"rmj", @"mka", @"mkv", @"weba", @"webm", @"dsf", @"dff", @"iff", @"dsdiff", @"wsd", @"aiff", @"aif"];
}

+ (NSArray *)mimeTypes {
	return @[@"application/wma", @"application/x-wma", @"audio/x-wma", @"audio/x-ms-wma", @"audio/x-tak", @"application/ogg", @"audio/aac", @"audio/aacp", @"audio/mpeg", @"audio/mp4", @"audio/x-mp3", @"audio/x-mp2", @"audio/x-matroska", @"audio/x-ape", @"audio/x-ac3", @"audio/x-dts", @"audio/x-dtshd", @"audio/x-at3", @"audio/wav", @"audio/tta", @"audio/x-tta", @"audio/x-twinvq", @"application/vnd.apple.mpegurl", @"audio/mpegurl"];
}

+ (NSArray *)fileTypeAssociations {
	return @[
		@[@"Windows Media Audio File", @"song.icns", @"wma", @"asf"],
		@[@"TAK Audio File", @"song.icns", @"tak"],
		@[@"MPEG-4 Audio File", @"m4a.icns", @"mp4", @"m4a", @"m4b", @"m4r"],
		@[@"MPEG-4 AAC Audio File", @"song.icns", @"aac"],
		@[@"MPEG Audio File", @"mp3.icns", @"mp3", @"m2a", @"mpa"],
		@[@"Monkey's Audio File", @"ape.icns", @"ape"],
		@[@"AC-3 Audio File", @"song.icns", @"ac3"],
		@[@"DTS Audio File", @"song.icns", @"dts"],
		@[@"DTS-HD MA Audio File", @"song.icns", @"dtshd"],
		@[@"True Audio File", @"song.icns", @"tta"],
		@[@"TrueVQ Audio File", @"song.icns", @"vqf", @"vqe", @"vql"],
		@[@"Real Audio File", @"song.icns", @"ra", @"rm", @"rmj"],
		@[@"Matroska Audio File", @"song.icns", @"mka"],
		@[@"Matroska Video File", @"song.icns", @"mkv"],
		@[@"WebM Audio File", @"song.icns", @"weba"],
		@[@"WebM Media File", @"song.icns", @"webm"],
		@[@"DSD Stream File", @"song.icns", @"dsf"],
		@[@"Interchange File Format", @"song.icns", @"iff", @"dsdiff"],
		@[@"Wideband Single-bit Data", @"song.icns", @"wsd"],
		@[@"Audio Interchange File Format", @"aiff.icns", @"aiff", @"aif"]
	];
}

+ (float)priority {
	return 1.5;
}

@end
