//
//  FFMPEGContainer.m
//  FFMPEG Plugin
//
//  Created by Christopher Snowhill on 6/25/22.
//

#import "FFMPEGContainer.h"

#import "FFMPEGDecoder.h"

#import "Logging.h"

@implementation FFMPEGContainer

+ (NSArray *)fileTypes {
	return [FFMPEGDecoder fileTypes];
}

+ (NSArray *)mimeTypes {
	return [FFMPEGDecoder fileTypes];
}

+ (float)priority {
	return [FFMPEGDecoder priority];
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url {
	char errDescr[4096];

	if([url fragment]) {
		// input url already has fragment defined - no need to expand further
		return [NSMutableArray arrayWithObject:url];
	}

	id audioSourceClass = NSClassFromString(@"AudioSource");
	id<CogSource> source = [audioSourceClass audioSourceForURL:url];

	if(![source open:url])
		return [NSArray array];

	int errcode, i;
	AVStream *stream;

	AVFormatContext *formatCtx = NULL;
	AVIOContext *ioCtx = NULL;

	BOOL isStream = NO;

	uint8_t *buffer = NULL;

	// register all available codecs

	if(([[url scheme] isEqualToString:@"http"] ||
	    [[url scheme] isEqualToString:@"https"]) &&
	   [[url pathExtension] isEqualToString:@"m3u8"]) {
		[source close];
		source = nil;

		isStream = YES;

		formatCtx = avformat_alloc_context();
		if(!formatCtx) {
			ALog(@"Unable to allocate AVFormat context");
			return [NSArray array];
		}

		NSString *urlString = [url absoluteString];
		if((errcode = avformat_open_input(&formatCtx, [urlString UTF8String], NULL, NULL)) < 0) {
			av_strerror(errcode, errDescr, 4096);
			ALog(@"Error opening file, errcode = %d, error = %s", errcode, errDescr);
			return [NSArray array];
		}
	} else {
		buffer = av_malloc(32 * 1024);
		if(!buffer) {
			ALog(@"Out of memory!");
			[source close];
			source = nil;
			return [NSArray array];
		}

		ioCtx = avio_alloc_context(buffer, 32 * 1024, 0, (__bridge void *)source, ffmpeg_read, ffmpeg_write, ffmpeg_seek);
		if(!ioCtx) {
			ALog(@"Unable to create AVIO context");
			av_free(buffer);
			[source close];
			source = nil;
			return [NSArray array];
		}

		formatCtx = avformat_alloc_context();
		if(!formatCtx) {
			ALog(@"Unable to allocate AVFormat context");
			buffer = ioCtx->buffer;
			av_free(ioCtx);
			av_free(buffer);
			[source close];
			source = nil;
			return [NSArray array];
		}

		formatCtx->pb = ioCtx;

		if((errcode = avformat_open_input(&formatCtx, "", NULL, NULL)) < 0) {
			av_strerror(errcode, errDescr, 4096);
			ALog(@"Error opening file, errcode = %d, error = %s", errcode, errDescr);
			avformat_close_input(&(formatCtx));
			buffer = ioCtx->buffer;
			av_free(ioCtx);
			av_free(buffer);
			[source close];
			source = nil;
			return [NSArray array];
		}
	}

	if((errcode = avformat_find_stream_info(formatCtx, NULL)) < 0) {
		av_strerror(errcode, errDescr, 4096);
		ALog(@"Can't find stream info, errcode = %d, error = %s", errcode, errDescr);
		avformat_close_input(&(formatCtx));
		if(ioCtx) {
			buffer = ioCtx->buffer;
			av_free(ioCtx);
		}
		if(buffer) {
			av_free(buffer);
		}
		if(source) {
			[source close];
			source = nil;
		}
		return [NSArray array];
	}

	int streamIndex = -1;
	int metadataIndex = -1;
	int attachedPicIndex = -1;
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
		avformat_close_input(&(formatCtx));
		if(ioCtx) {
			buffer = ioCtx->buffer;
			av_free(ioCtx);
		}
		if(buffer) {
			av_free(buffer);
		}
		if(source) {
			[source close];
			source = nil;
		}
		return [NSArray array];
	}

	NSMutableArray *tracks = [NSMutableArray array];

	int subsongs = formatCtx->nb_chapters;
	if(subsongs < 1) subsongs = 1;

	for(i = 0; i < subsongs; ++i) {
		[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%i", i]]];
	}

	avformat_close_input(&(formatCtx));
	if(ioCtx) {
		buffer = ioCtx->buffer;
		av_free(ioCtx);
	}
	if(buffer) {
		av_free(buffer);
	}
	if(source) {
		[source close];
		source = nil;
	}

	return tracks;
}

@end
