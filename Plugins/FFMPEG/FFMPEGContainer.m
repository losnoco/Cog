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
		return @[url];
	}

	NSMutableArray *tracks = [NSMutableArray new];

	id audioSourceClass = NSClassFromString(@"AudioSource");
	id<CogSource> source = [audioSourceClass audioSourceForURL:url];

	if(![source open:url])
		return @[];

	int errcode, i;
	AVStream *stream;

	AVFormatContext *formatCtx = NULL;
	AVIOContext *ioCtx = NULL;

	uint8_t *buffer = NULL;

	FFMPEGReader *reader = nil;

	// register all available codecs

	if(([[url scheme] isEqualToString:@"http"] ||
	    [[url scheme] isEqualToString:@"https"]) &&
	   [[url pathExtension] isEqualToString:@"m3u8"]) {
		[source close];
		source = nil;

		formatCtx = avformat_alloc_context();
		if(!formatCtx) {
			ALog(@"Unable to allocate AVFormat context");
			goto exit;
		}

		NSString *urlString = [url absoluteString];
		if((errcode = avformat_open_input(&formatCtx, [urlString UTF8String], NULL, NULL)) < 0) {
			av_strerror(errcode, errDescr, 4096);
			ALog(@"Error opening file, errcode = %d, error = %s", errcode, errDescr);
			goto exit;
		}
	} else {
		buffer = av_malloc(32 * 1024);
		if(!buffer) {
			ALog(@"Out of memory!");
			goto exit;
		}

		reader = [[FFMPEGReader alloc] initWithFile:source];

		ioCtx = avio_alloc_context(buffer, 32 * 1024, 0, (__bridge void *)reader, ffmpeg_read, ffmpeg_write, ffmpeg_seek);
		if(!ioCtx) {
			ALog(@"Unable to create AVIO context");
			goto exit;
		}

		formatCtx = avformat_alloc_context();
		if(!formatCtx) {
			ALog(@"Unable to allocate AVFormat context");
			goto exit;
		}

		formatCtx->pb = ioCtx;

		if((errcode = avformat_open_input(&formatCtx, "", NULL, NULL)) < 0) {
			av_strerror(errcode, errDescr, 4096);
			ALog(@"Error opening file, errcode = %d, error = %s", errcode, errDescr);
			goto exit;
		}
	}

	if((errcode = avformat_find_stream_info(formatCtx, NULL)) < 0) {
		av_strerror(errcode, errDescr, 4096);
		ALog(@"Can't find stream info, errcode = %d, error = %s", errcode, errDescr);
		goto exit;
	}

	int streamIndex = -1;
	//int metadataIndex = -1;
	//int attachedPicIndex = -1;
	AVCodecParameters *codecPar;

	for(i = 0; i < formatCtx->nb_streams; i++) {
		stream = formatCtx->streams[i];
		codecPar = stream->codecpar;
		if(streamIndex < 0 && codecPar->codec_type == AVMEDIA_TYPE_AUDIO) {
			DLog(@"audio codec found");
			streamIndex = i;
		} else if(codecPar->codec_id == AV_CODEC_ID_TIMED_ID3) {
			//metadataIndex = i;
		} else if(stream->disposition & AV_DISPOSITION_ATTACHED_PIC) {
			//attachedPicIndex = i;
		} else {
			stream->discard = AVDISCARD_ALL;
		}
	}

	if(streamIndex < 0) {
		ALog(@"no audio codec found");
		goto exit;
	}

	int subsongs = formatCtx->nb_chapters;
	if(subsongs < 1) subsongs = 1;

	for(i = 0; i < subsongs; ++i) {
		[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%i", i]]];
	}

exit:
	if(formatCtx) {
		avformat_close_input(&(formatCtx));
	}
	if(ioCtx) {
		buffer = ioCtx->buffer;
		avio_context_free(&ioCtx);
	}
	if(buffer) {
		av_freep(&buffer);
	}
	if(source) {
		[source close];
		source = nil;
	}

	return [NSArray arrayWithArray:tracks];
}

@end
