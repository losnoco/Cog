//
//  FFMPEGContainer.m
//  FFMPEG Plugin
//
//  Created by Christopher Snowhill on 6/25/22.
//

#import "FFMPEGContainer.h"

#import "FFMPEGDecoder.h"

#import "NSDictionary+Merge.h"

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

// Whether the loader reads each track of `url` from this plugin's decoder
// alone: no properties or metadata reader for the file, and no decoder that
// would be tried ahead of ours. Any of those would add to or replace what
// the decoder gives, from a parse of its own.
static BOOL decoderAloneReadsURL(NSURL *url, id<CogSource> source) {
	id<CogPluginController> pluginController = [NSClassFromString(@"PluginController") sharedPluginController];
	NSString *ext = [[url pathExtension] lowercaseString];

	if([[pluginController metadataReaders] objectForKey:ext])
		return NO;
	if([[pluginController propertiesReadersByExtension] objectForKey:ext])
		return NO;
	if([[pluginController propertiesReadersByMimeType] objectForKey:[[source mimeType] lowercaseString]])
		return NO;

	NSArray *decoders = [[pluginController decodersByExtension] objectForKey:ext];
	if(![decoders containsObject:NSStringFromClass([FFMPEGDecoder class])])
		return NO;
	for(NSString *classString in decoders) {
		Class decoder = NSClassFromString(classString);
		if(decoder != [FFMPEGDecoder class] && [decoder priority] >= [FFMPEGDecoder priority])
			return NO;
	}

	return YES;
}

static BOOL hasContent(id value) {
	if([value isKindOfClass:[NSString class]])
		return [value length] > 0;
	if([value isKindOfClass:[NSArray class]]) {
		for(id item in value) {
			if([item isKindOfClass:[NSString class]] && [item length])
				return YES;
		}
	}
	return NO;
}

// Every track's properties and tags, as the decoder gives them opened on
// each track's URL, from the format context the tracks were found in.
+ (NSDictionary *)trackInfoForTracks:(NSArray *)tracks formatContext:(AVFormatContext *)formatCtx {
	NSMutableDictionary *info = [NSMutableDictionary dictionaryWithCapacity:[tracks count]];
	int i = 0;
	for(NSURL *track in tracks) {
		FFMPEGDecoder *decoder = [FFMPEGDecoder new];
		if([decoder openTrack:i ofFormatContext:formatCtx]) {
			NSDictionary *properties = [decoder properties];
			NSMutableDictionary *entryInfo = [[NSDictionary dictionaryByMerging:properties with:[decoder metadata]] mutableCopy];

			// A cue sheet in the file's tags makes the loader read its
			// subsongs as cue sheet tracks, which merge differently.
			if(i == 0 && [tracks count] > 1 && hasContent([entryInfo objectForKey:@"cuesheet"])) {
				[decoder close];
				return nil;
			}

			// A later track of no length cannot seek to its start, so its
			// decoder reads on from where the last one stopped, not from
			// where it would in a file just opened.
			if(i == 0 || [[properties objectForKey:@"totalFrames"] longValue])
				info[track] = entryInfo;
		} else if(i == 0 && [tracks count] > 1) {
			// A first track that will not open cannot show whether the
			// file has a cue sheet either.
			[decoder close];
			return nil;
		}
		[decoder close];
		++i;
	}
	return info;
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url {
	return [self urlsForContainerURL:url trackInfo:NULL];
}

+ (NSArray *)urlsForContainerURL:(NSURL *)url trackInfo:(NSDictionary **)trackInfo {
	char errDescr[AV_ERROR_MAX_STRING_SIZE];

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
			av_strerror(errcode, errDescr, sizeof(errDescr));
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
			av_strerror(errcode, errDescr, sizeof(errDescr));
			ALog(@"Error opening file, errcode = %d, error = %s", errcode, errDescr);
			goto exit;
		}
	}

	if((errcode = avformat_find_stream_info(formatCtx, NULL)) < 0) {
		av_strerror(errcode, errDescr, sizeof(errDescr));
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

	if(subsongs <= 1) {
		[tracks addObject:url];
	} else {
		for(i = 0; i < subsongs; ++i) {
			[tracks addObject:[NSURL URLWithString:[[url absoluteString] stringByAppendingFormat:@"#%i", i]]];
		}
	}

	// Only from a local file opened as the decoder opens one: the loader
	// reads no properties from network streams.
	if(trackInfo && source && [url isFileURL] && [source seekable] && decoderAloneReadsURL(url, source)) {
		*trackInfo = [FFMPEGContainer trackInfoForTracks:tracks formatContext:formatCtx];
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
