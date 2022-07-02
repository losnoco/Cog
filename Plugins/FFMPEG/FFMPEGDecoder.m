//
//  FFMPEGDecoder.m
//  FFMPEG
//
//  Created by Andre Reffhaug on 2/26/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

// test
#import "FFMPEGDecoder.h"

#import "NSDictionary+Merge.h"
#import "TagLibID3v2Reader.h"

#include <pthread.h>

#import "Logging.h"

#import "HTTPSource.h"

#define ST_BUFF 2048

int ffmpeg_read(void *opaque, uint8_t *buf, int buf_size) {
	id source = (__bridge id)opaque;
	long sizeRead = [source read:buf amount:buf_size];
	if(sizeRead == 0) return AVERROR_EOF;
	return (int)sizeRead;
}

int ffmpeg_write(void *opaque, uint8_t *buf, int buf_size) {
	return -1;
}

int64_t ffmpeg_seek(void *opaque, int64_t offset, int whence) {
	id source = (__bridge id)opaque;
	if(whence & AVSEEK_SIZE) {
		if([source seekable]) {
			int64_t curOffset = [source tell];
			[source seek:0 whence:SEEK_END];
			int64_t size = [source tell];
			[source seek:curOffset whence:SEEK_SET];
			return size;
		}
		return -1;
	}
	whence &= ~(AVSEEK_SIZE | AVSEEK_FORCE);
	return [source seekable] ? ([source seek:offset whence:whence] ? [source tell] : -1) : -1;
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
	int errcode, i;
	AVStream *stream;

	source = s;

	formatCtx = NULL;
	totalFrames = 0;
	framesRead = 0;

	rawDSD = NO;

	BOOL isStream = NO;

	// register all available codecs

	if([[source.url fragment] length] == 0)
		subsong = 0;
	else
		subsong = [[source.url fragment] intValue];

	NSURL *url = [s url];
	if(([[url scheme] isEqualToString:@"http"] ||
	    [[url scheme] isEqualToString:@"https"]) &&
	   [[url pathExtension] isEqualToString:@"m3u8"]) {
		source = nil;
		[s close];

		isStream = YES;

		formatCtx = avformat_alloc_context();
		if(!formatCtx) {
			ALog(@"Unable to allocate AVFormat context");
			return NO;
		}

		NSString *urlString = [url absoluteString];
		if((errcode = avformat_open_input(&formatCtx, [urlString UTF8String], NULL, NULL)) < 0) {
			char errDescr[4096];
			av_strerror(errcode, errDescr, 4096);
			ALog(@"Error opening file, errcode = %d, error = %s", errcode, errDescr);
			return NO;
		}
	} else {
		buffer = av_malloc(32 * 1024);
		if(!buffer) {
			ALog(@"Out of memory!");
			return NO;
		}

		ioCtx = avio_alloc_context(buffer, 32 * 1024, 0, (__bridge void *)source, ffmpeg_read, ffmpeg_write, ffmpeg_seek);
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
			char errDescr[4096];
			av_strerror(errcode, errDescr, 4096);
			ALog(@"Error opening file, errcode = %d, error = %s", errcode, errDescr);
			return NO;
		}
	}

	if((errcode = avformat_find_stream_info(formatCtx, NULL)) < 0) {
		char errDescr[4096];
		av_strerror(errcode, errDescr, 4096);
		ALog(@"Can't find stream info, errcode = %d, error = %s", errcode, errDescr);
		return NO;
	}

	streamIndex = -1;
	metadataIndex = -1;
	attachedPicIndex = -1;
	AVCodecParameters *codecPar;

	for(i = 0; i < formatCtx->nb_streams; i++) {
		stream = formatCtx->streams[i];
		codecPar = stream->codecpar;
		if(streamIndex < 0 && codecPar->codec_type == AVMEDIA_TYPE_AUDIO) {
			DLog(@"audio codec found");
			streamIndex = i;
		} else if(codecPar->codec_id == AV_CODEC_ID_TIMED_ID3) {
			metadataIndex = i;
		} else if(stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
			attachedPicIndex = i;
		} else {
			stream->discard = AVDISCARD_ALL;
		}
	}

	if(streamIndex < 0) {
		ALog(@"no audio codec found");
		return NO;
	}

	stream = formatCtx->streams[streamIndex];
	codecPar = stream->codecpar;

	codecCtx = avcodec_alloc_context3(NULL);
	if(!codecCtx) {
		ALog(@"could not allocate codec context");
		return NO;
	}

	if((errcode = avcodec_parameters_to_context(codecCtx, codecPar)) < 0) {
		char errDescr[4096];
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
		char errDescr[4096];
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
	totalFrames = isStream ? 0 : av_rescale_q(formatCtx->duration, AV_TIME_BASE_Q, tb);
	bitrate = (int)((codecCtx->bit_rate) / 1000);
	framesRead = 0;
	endOfStream = NO;
	endOfAudio = NO;

	if(rawDSD) {
		totalFrames *= 8;
	}

	if(!isStream) {
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

	seekable = [s seekable];

	seekedToStart = !seekable;

	artist = @"";
	albumartist = @"";
	album = @"";
	title = @"";
	genre = @"";
	year = @(0);
	track = @(0);
	disc = @(0);
	replayGainAlbumGain = 0.0;
	replayGainAlbumPeak = 0.0;
	replayGainTrackGain = 0.0;
	replayGainTrackPeak = 0.0;
	volumeScale = 1.0;
	albumArt = [NSData data];
	id3Metadata = @{};
	metadataUpdated = NO;
	[self updateMetadata];

	prebufferedAudio = 0;
	prebufferedAudioData = NULL;

	if(attachedPicIndex >= 0) {
		int frameSize = rawDSD ? channels : channels * (bitsPerSample / 8);
		prebufferedAudioData = malloc(1024 * frameSize);
		[self readAudio:prebufferedAudioData frames:1024];
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
		av_free(lastDecodedFrame);
		lastDecodedFrame = NULL;
	}

	if(codecCtx) {
		avcodec_close(codecCtx);
		avcodec_free_context(&codecCtx);
		codecCtx = NULL;
	}

	if(formatCtx) {
		avformat_close_input(&(formatCtx));
		formatCtx = NULL;
	}

	if(ioCtx) {
		buffer = ioCtx->buffer;
		av_free(ioCtx);
		ioCtx = NULL;
	}

	if(buffer) {
		av_free(buffer);
		buffer = NULL;
	}
}

- (void)dealloc {
	[self close];
}

- (void)updateMetadata {
	const AVDictionaryEntry *tag = NULL;
	NSString *_artist = artist;
	NSString *_albumartist = albumartist;
	NSString *_album = album;
	NSString *_title = title;
	NSString *_genre = genre;
	NSNumber *_year = year;
	NSNumber *_track = track;
	NSNumber *_disc = disc;
	float _replayGainAlbumGain = replayGainAlbumGain;
	float _replayGainAlbumPeak = replayGainAlbumPeak;
	float _replayGainTrackGain = replayGainTrackGain;
	float _replayGainTrackPeak = replayGainTrackPeak;
	float _volumeScale = volumeScale;
	for(size_t i = 0; i < 2; ++i) {
		AVDictionary *metadata;
		if(i == 0) {
			metadata = formatCtx->metadata;
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
		while((tag = av_dict_get(metadata, "", tag, AV_DICT_IGNORE_SUFFIX))) {
			if(!strcasecmp(tag->key, "streamtitle")) {
				NSString *artistTitle = guess_encoding_of_string(tag->value);
				NSArray *splitValues = [artistTitle componentsSeparatedByString:@" - "];
				_artist = @"";
				_title = [splitValues objectAtIndex:0];
				if([splitValues count] > 1) {
					_artist = _title;
					_title = [splitValues objectAtIndex:1];
				}
			} else if(!strcasecmp(tag->key, "icy-url")) {
				_album = guess_encoding_of_string(tag->value);
			} else if(!strcasecmp(tag->key, "icy-genre") ||
			          !strcasecmp(tag->key, "genre")) {
				_genre = guess_encoding_of_string(tag->value);
			} else if(!strcasecmp(tag->key, "album")) {
				_album = guess_encoding_of_string(tag->value);
			} else if(!strcasecmp(tag->key, "album_artist")) {
				_albumartist = guess_encoding_of_string(tag->value);
			} else if(!strcasecmp(tag->key, "artist")) {
				_artist = guess_encoding_of_string(tag->value);
			} else if(!strcasecmp(tag->key, "title")) {
				_title = guess_encoding_of_string(tag->value);
			} else if(!strcasecmp(tag->key, "date")) {
				NSString *dateString = guess_encoding_of_string(tag->value);
				_year = @([dateString intValue]);
			} else if(!strcasecmp(tag->key, "track")) {
				NSString *trackString = guess_encoding_of_string(tag->value);
				_track = @([trackString intValue]);
			} else if(!strcasecmp(tag->key, "disc")) {
				NSString *discString = guess_encoding_of_string(tag->value);
				_disc = @([discString intValue]);
			} else if(!strcasecmp(tag->key, "replaygain_album_gain")) {
				NSString *rgValue = guess_encoding_of_string(tag->value);
				_replayGainAlbumGain = [rgValue floatValue];
			} else if(!strcasecmp(tag->key, "replaygain_album_peak")) {
				NSString *rgValue = guess_encoding_of_string(tag->value);
				_replayGainAlbumPeak = [rgValue floatValue];
			} else if(!strcasecmp(tag->key, "replaygain_track_gain")) {
				NSString *rgValue = guess_encoding_of_string(tag->value);
				_replayGainTrackGain = [rgValue floatValue];
			} else if(!strcasecmp(tag->key, "replaygain_track_peak")) {
				NSString *rgValue = guess_encoding_of_string(tag->value);
				_replayGainTrackPeak = [rgValue floatValue];
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
					_volumeScale = pow(10, volumeToUse / 20);
				}
			}
		}
	}

	Class sourceClass = [source class];
	if([sourceClass isEqual:NSClassFromString(@"HTTPSource")]) {
		HTTPSource *httpSource = (HTTPSource *)source;
		if([httpSource hasMetadata]) {
			NSDictionary *metadata = [httpSource metadata];
			_genre = [metadata valueForKey:@"genre"];
			_album = [metadata valueForKey:@"album"];
			_artist = [metadata valueForKey:@"artist"];
			_title = [metadata valueForKey:@"title"];
		}
	}

	if(![_artist isEqual:artist] ||
	   ![_albumartist isEqual:albumartist] ||
	   ![_album isEqual:album] ||
	   ![_title isEqual:title] ||
	   ![_genre isEqual:genre] ||
	   ![_year isEqual:year] ||
	   ![_track isEqual:track] ||
	   ![_disc isEqual:disc] ||
	   _replayGainAlbumGain != replayGainAlbumGain ||
	   _replayGainAlbumPeak != replayGainAlbumPeak ||
	   _replayGainTrackGain != replayGainTrackGain ||
	   _replayGainTrackPeak != replayGainTrackPeak ||
	   _volumeScale != volumeScale) {
		artist = _artist;
		albumartist = _albumartist;
		album = _album;
		title = _title;
		genre = _genre;
		year = _year;
		track = _track;
		disc = _disc;
		replayGainAlbumGain = _replayGainAlbumGain;
		replayGainAlbumPeak = _replayGainAlbumPeak;
		replayGainTrackGain = _replayGainTrackGain;
		replayGainTrackPeak = _replayGainTrackPeak;
		volumeScale = _volumeScale;
		if(![source seekable]) {
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
		if(![_id3Metadata isEqualTo:id3Metadata]) {
			id3Metadata = _id3Metadata;
			[self willChangeValueForKey:@"metadata"];
			[self didChangeValueForKey:@"metadata"];
		}
	}
}

- (void)updateArtwork {
	NSData *_albumArt = [NSData dataWithBytes:lastReadPacket->data length:lastReadPacket->size];
	if(![_albumArt isEqual:albumArt]) {
		albumArt = _albumArt;
		if(![source seekable]) {
			[self willChangeValueForKey:@"metadata"];
			[self didChangeValueForKey:@"metadata"];
		} else {
			metadataUpdated = YES;
		}
	}
}

- (int)readAudio:(void *)buf frames:(UInt32)frames {
	if(!seekedToStart) {
		[self seek:0];
	}

	int frameSize = rawDSD ? channels : channels * (bitsPerSample / 8);
	int bytesToRead = frames * frameSize;
	int bytesRead = 0;

	if(prebufferedAudio) {
		// A bit of ignored read-ahead to support embedded artwork
		int bytesBuffered = prebufferedAudio * frameSize;
		int bytesToCopy = (bytesBuffered > bytesToRead) ? bytesToRead : bytesBuffered;
		memcpy(buf, prebufferedAudioData, bytesToCopy);
		memmove(prebufferedAudioData, prebufferedAudioData + bytesToCopy, bytesBuffered - bytesToCopy);
		prebufferedAudio -= bytesToCopy / frameSize;
		bytesRead = bytesToCopy;

		int framesReadNow = bytesRead / frameSize;
		framesRead -= framesReadNow;
	}

	if(totalFrames && framesRead >= totalFrames)
		return 0;

	int dataSize = 0;

	int seekBytesSkip = 0;

	int errcode;

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
						char errDescr[4096];
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

	[self updateMetadata];

	int framesReadNow = bytesRead / frameSize;
	if(totalFrames && (framesRead + framesReadNow > totalFrames))
		framesReadNow = (int)(totalFrames - framesRead);

	framesRead += framesReadNow;

	return framesReadNow;
}

- (long)seek:(long)frame {
	if(!totalFrames)
		return -1;

	seekedToStart = YES;

	prebufferedAudio = 0;

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
	return [NSDictionary dictionaryByMerging:@{ @"artist": artist, @"albumartist": albumartist, @"album": album, @"title": title, @"genre": genre, @"year": year, @"track": track, @"disc": disc, @"replayGainAlbumGain": @(replayGainAlbumGain), @"replayGainAlbumPeak": @(replayGainAlbumPeak), @"replayGainTrackGain": @(replayGainTrackGain), @"replayGainTrackPeak": @(replayGainTrackPeak), @"volume": @(volumeScale), @"albumArt": albumArt } with:id3Metadata];
}

+ (NSArray *)fileTypes {
	return @[@"wma", @"asf", @"tak", @"mp4", @"m4a", @"m4b", @"m4r", @"aac", @"mp3", @"mp2", @"m2a", @"mpa", @"ape", @"ac3", @"dts", @"dtshd", @"wav", @"tta", @"vqf", @"vqe", @"vql", @"ra", @"rm", @"rmj", @"mka", @"weba", @"dsf", @"dff", @"iff", @"dsdiff", @"wsd", @"aiff", @"aif"];
}

+ (NSArray *)mimeTypes {
	return @[@"application/wma", @"application/x-wma", @"audio/x-wma", @"audio/x-ms-wma", @"audio/x-tak", @"application/ogg", @"audio/aacp", @"audio/mpeg", @"audio/mp4", @"audio/x-mp3", @"audio/x-mp2", @"audio/x-matroska", @"audio/x-ape", @"audio/x-ac3", @"audio/x-dts", @"audio/x-dtshd", @"audio/x-at3", @"audio/wav", @"audio/tta", @"audio/x-tta", @"audio/x-twinvq", @"application/vnd.apple.mpegurl"];
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
		@[@"WebM Audio File", @"song.icns", @"weba"],
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
