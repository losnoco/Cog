//
//  HTTPSource.m
//  HTTPSource
//
//  Created by Vincent Spader on 3/1/07.
//  Replaced by Christopher Snowhill on 3/7/20.
//  Copyright 2020-2026 __LoSnoCo__. All rights reserved.
//

#import "HTTPSource.h"

#import "Logging.h"

#import "NSDictionary+Optional.h"

#import <stdlib.h>
#import <string.h>

@implementation HTTPSource

#define HTTP_STREAMING_BUFFER_SIZE_DEFAULT 0x40000
#define HTTP_STREAMING_BUFFER_SIZE_MIN 0x10000
#define HTTP_STREAMING_BUFFER_SIZE_MAX 0x8000000

#define RETRY_OVERLAP_SIZE_MAX HTTP_STREAMING_BUFFER_SIZE_DEFAULT
#define RETRY_OVERLAP_MIN_MATCH 512

static size_t http_streaming_buffer_size_from_defaults(void) {
	NSInteger requested = [[NSUserDefaults standardUserDefaults] integerForKey:@"httpStreamingBufferSize"];
	if(requested < HTTP_STREAMING_BUFFER_SIZE_MIN ||
	   requested > HTTP_STREAMING_BUFFER_SIZE_MAX ||
	   (requested & (requested - 1)) != 0) {
		return HTTP_STREAMING_BUFFER_SIZE_DEFAULT;
	}

	return (size_t)requested;
}

static void http_free_buffers(HTTPSource *fp) {
	free(fp->buffer);
	free(fp->retryOverlapBuffer);
	fp->buffer = NULL;
	fp->retryOverlapBuffer = NULL;
	fp->bufferSize = 0;
	fp->bufferMask = 0;
}

static BOOL http_allocate_buffers(HTTPSource *fp) {
	http_free_buffers(fp);

	fp->bufferSize = http_streaming_buffer_size_from_defaults();
	fp->bufferMask = (int64_t)(fp->bufferSize - 1);
	fp->buffer = calloc(1, fp->bufferSize);
	fp->retryOverlapBuffer = calloc(1, MIN(fp->bufferSize, (size_t)RETRY_OVERLAP_SIZE_MAX));
	if(!fp->buffer || !fp->retryOverlapBuffer) {
		http_free_buffers(fp);
		return NO;
	}

	DLog(@"urlsession: using %zu byte streaming buffer", fp->bufferSize);
	return YES;
}

static void http_retry_overlap_clear(HTTPSource *fp) {
	fp->retryOverlapSize = 0;
	fp->retryOverlapOffset = 0;
	fp->retryOverlapMatched = 0;
}

static void http_retry_overlap_snapshot_locked(HTTPSource *fp) {
	int64_t tail = fp->pos + fp->remaining;
	if(fp->retryOverlapSize > 0 || tail <= 0) {
		return;
	}

	fp->retryOverlapSize = (int32_t)MIN((int64_t)MIN(fp->bufferSize, (size_t)RETRY_OVERLAP_SIZE_MAX), tail);
	fp->retryOverlapOffset = 0;
	fp->retryOverlapMatched = 0;

	size_t overlapSize = (size_t)fp->retryOverlapSize;
	size_t readpos = (size_t)((tail - fp->retryOverlapSize) & fp->bufferMask);
	size_t part1 = fp->bufferSize - readpos;
	part1 = MIN(part1, overlapSize);
	memcpy(fp->retryOverlapBuffer, fp->buffer + readpos, part1);

	if(part1 < overlapSize) {
		memcpy(fp->retryOverlapBuffer + part1, fp->buffer, overlapSize - part1);
	}
}

static size_t http_retry_overlap_match_length(const uint8_t *a, size_t asize, const uint8_t *b, size_t bsize) {
	size_t size = MIN(asize, bsize);
	size_t i = 0;
	while(i < size && a[i] == b[i]) {
		i++;
	}
	return i;
}

static BOOL http_retry_overlap_find_locked(HTTPSource *fp, const uint8_t *ptr, size_t size, size_t *inputSkip, size_t *overlapOffset) {
	if(fp->retryOverlapSize <= 0 || size == 0) {
		return NO;
	}

	size_t minMatch = MIN((size_t)RETRY_OVERLAP_MIN_MATCH, (size_t)fp->retryOverlapSize);
	minMatch = MIN(minMatch, size);
	if(minMatch < 32) {
		return NO;
	}

	size_t bestInputSkip = 0;
	size_t bestOverlapOffset = 0;
	size_t bestMatch = 0;
	const uint8_t firstInputByte = ptr[0];

	for(size_t offset = 0; offset + minMatch <= fp->retryOverlapSize; offset++) {
		if(fp->retryOverlapBuffer[offset] != firstInputByte ||
		   memcmp(fp->retryOverlapBuffer + offset, ptr, minMatch)) {
			continue;
		}

		size_t match = http_retry_overlap_match_length(fp->retryOverlapBuffer + offset,
		                                               fp->retryOverlapSize - offset,
		                                               ptr,
		                                               size);
		if(match > bestMatch) {
			bestInputSkip = 0;
			bestOverlapOffset = offset;
			bestMatch = match;
		}
	}

	const uint8_t firstOverlapByte = fp->retryOverlapBuffer[0];
	for(size_t offset = 1; offset + minMatch <= size; offset++) {
		if(ptr[offset] != firstOverlapByte ||
		   memcmp(fp->retryOverlapBuffer, ptr + offset, minMatch)) {
			continue;
		}

		size_t match = http_retry_overlap_match_length(fp->retryOverlapBuffer,
		                                               fp->retryOverlapSize,
		                                               ptr + offset,
		                                               size - offset);
		if(match > bestMatch) {
			bestInputSkip = offset;
			bestOverlapOffset = 0;
			bestMatch = match;
		}
	}

	if(bestMatch < minMatch) {
		return NO;
	}

	*inputSkip = bestInputSkip;
	*overlapOffset = bestOverlapOffset;
	return YES;
}

static size_t http_retry_overlap_discard_locked(HTTPSource *fp, const uint8_t *ptr, size_t size) {
	if(fp->retryOverlapSize <= 0 || size == 0) {
		return 0;
	}

	size_t discarded = 0;

	if(!fp->retryOverlapMatched) {
		size_t inputSkip = 0;
		size_t overlapOffset = 0;
		if(!http_retry_overlap_find_locked(fp, ptr, size, &inputSkip, &overlapOffset)) {
			http_retry_overlap_clear(fp);
			return 0;
		}

		fp->retryOverlapMatched = 1;
		fp->retryOverlapOffset = (int32_t)overlapOffset;
		discarded = inputSkip;
		ptr += inputSkip;
		size -= inputSkip;
		DLog(@"urlsession: retry overlap matched at buffered offset %d", fp->retryOverlapOffset);
	}

	while(size > 0 && fp->retryOverlapOffset < fp->retryOverlapSize) {
		size_t cmp = MIN(size, (size_t)(fp->retryOverlapSize - fp->retryOverlapOffset));
		size_t match = http_retry_overlap_match_length(fp->retryOverlapBuffer + fp->retryOverlapOffset,
		                                               cmp,
		                                               ptr,
		                                               cmp);
		discarded += match;
		ptr += match;
		size -= match;
		fp->retryOverlapOffset += (int32_t)match;

		if(match < cmp) {
			http_retry_overlap_clear(fp);
			break;
		}
	}

	if(fp->retryOverlapOffset >= fp->retryOverlapSize) {
		http_retry_overlap_clear(fp);
	}

	return discarded;
}

static size_t http_data_write_wrapper(HTTPSource *fp, char *ptr, size_t size) {
	size_t avail = size;
	size_t consumed = 0;
	while(avail > 0) {
		[fp->mutex lock];
		if(fp->status == STATUS_SEEK || fp->status == STATUS_RETRY) {
			DLog(@"urlsession restart request, aborting current request");
			[fp->mutex unlock];
			return consumed;
		}
		if(fp->need_abort) {
			fp->status = STATUS_ABORTED;
			DLog(@"urlsession STATUS_ABORTED in the middle of packet");
			[fp->mutex unlock];
			break;
		}

		size_t discard = http_retry_overlap_discard_locked(fp, (const uint8_t *)ptr, avail);
		if(discard > 0) {
			ptr += discard;
			avail -= discard;
			consumed += discard;
		}

		if(avail == 0) {
			[fp->mutex unlock];
			break;
		}

		int32_t sz = (int32_t)(fp->bufferSize / 2) - fp->remaining; // number of bytes free in buffer
		                                                            // don't allow to fill more than half -- used for seeking backwards

		if(sz > 5000) { // wait until there are at least 5k bytes free
			size_t cp = MIN(avail, (size_t)sz);
			size_t writepos = (size_t)((fp->pos + fp->remaining) & fp->bufferMask);
			// copy 1st portion (before end of buffer
			size_t part1 = fp->bufferSize - writepos;
			// may not be more than total
			part1 = MIN(part1, cp);
			memcpy(fp->buffer + writepos, ptr, part1);
			ptr += part1;
			avail -= part1;
			consumed += part1;
			fp->remaining += part1;
			cp -= part1;
			if(cp > 0) {
				memcpy(fp->buffer, ptr, cp);
				ptr += cp;
				avail -= cp;
				consumed += cp;
				fp->remaining += cp;
			}
		}
		[fp->mutex unlock];
		usleep(3000);
	}
	return consumed;
}

static int http_parse_shoutcast_meta(HTTPSource *fp, const char *meta, size_t size) {
	//    DLog (@"reading %d bytes of metadata\n", size);
	@autoreleasepool {
		DLog(@"%s", meta);
		const char *e = meta + size;
		const char strtitle[] = "StreamTitle='";
		char title[4096] = "";
		while(meta < e) {
			if(!memcmp(meta, strtitle, sizeof(strtitle) - 1)) {
				meta += sizeof(strtitle) - 1;
				const char *substr_end = meta;
				while(substr_end < e - 1 && (*substr_end != '\'' || *(substr_end + 1) != ';')) {
					substr_end++;
				}
				if(substr_end >= e) {
					return -1; // end of string not found
				}
				size_t s = substr_end - meta;
				s = MIN(sizeof(title) - 1, s);
				memcpy(title, meta, s);
				title[s] = 0;
				DLog(@"got stream title: %s\n", title);
				{
					char *tit = strstr(title, " - ");
					if(tit) {
						*tit = 0;
						tit += 3;

						if(!strncmp(tit, "text=\"", 6)) { // Hack for a certain stream
							char *titfirst = tit + 6;
							char *titlast = strchr(titfirst, '"');
							if(titlast) {
								*titlast = 0;
							}
							tit = titfirst;
						}

						const char *orig_title = [fp->title UTF8String];
						const char *orig_artist = [fp->artist UTF8String];

						if(!orig_title || strcasecmp(orig_title, tit)) {
							fp->title = guess_encoding_of_string(tit);
							fp->gotmetadata = 1;
						}
						if(!orig_artist || strcasecmp(orig_artist, title)) {
							fp->artist = guess_encoding_of_string(title);
							fp->gotmetadata = 1;
						}
					} else {
						const char *orig_title = [fp->title UTF8String];
						if(!orig_title || strcasecmp(orig_title, title)) {
							fp->artist = @"";
							fp->title = guess_encoding_of_string(title);
							fp->gotmetadata = 1;
						}
					}
				}
				return 0;
			}
			while(meta < e && *meta != ';') {
				meta++;
			}
			if(meta < e) {
				meta++;
			}
		}
		return -1;
	}
}

static const uint8_t *parse_header(const uint8_t *p, const uint8_t *e, uint8_t *key, int keysize, uint8_t *value, int valuesize) {
	size_t sz; // will hold length of extracted string
	const uint8_t *v; // pointer to current character
	keysize--;
	valuesize--;
	*key = 0;
	*value = 0;
	v = p;
	// find :
	while(v < e && *v != 0x0d && *v != 0x0a && *v != ':') {
		v++;
	}
	if(*v != ':') {
		// skip linebreaks
		while(v < e && (*v == 0x0d || *v == 0x0a)) {
			v++;
		}
		return v;
	}
	// copy key
	sz = v - p;
	sz = MIN(keysize, sz);
	memcpy(key, p, sz);
	key[sz] = 0;

	// skip whitespace
	v++;
	while(v < e && (*v == 0x20 || *v == 0x08)) {
		v++;
	}
	if(*v == 0x0d || *v == 0x0a) {
		// skip linebreaks
		while(v < e && (*v == 0x0d || *v == 0x0a)) {
			v++;
		}
		return v;
	}
	p = v;

	// find linebreak
	while(v < e && *v != 0x0d && *v != 0x0a) {
		v++;
	}

	// copy value
	sz = v - p;
	sz = MIN(valuesize, sz);
	memcpy(value, p, sz);
	value[sz] = 0;

	return v;
}

static size_t http_content_header_handler_int(void *ptr, size_t size, void *stream, int *end_of_headers) {
	//    DLog(@"http_content_header_handler\n");
	assert(stream);
	HTTPSource *fp = (__bridge HTTPSource *)stream;
	const uint8_t *p = ptr;
	const uint8_t *end = p + size;
	uint8_t key[256];
	uint8_t value[256];

	if(fp->length == 0) {
		fp->length = -1;
	}

	while(p < end) {
		if(p <= end - 4) {
			if(!memcmp(p, "\r\n\r\n", 4)) {
				p += 4;
				*end_of_headers = 1;
				return p - (uint8_t *)ptr;
			}
		}
		// skip linebreaks
		while(p < end && (*p == 0x0d || *p == 0x0a)) {
			p++;
		}
		@autoreleasepool {
			p = parse_header(p, end, key, sizeof(key), value, sizeof(value));
			DLog(@"%skey=%s value=%s\n", fp->icyheader ? "[icy] " : "", key, value);
			if(!strcasecmp((char *)key, "Content-Type")) {
				fp->content_type = guess_encoding_of_string((const char *)value);
			} else if(!strcasecmp((char *)key, "Content-Length")) {
				char *end;
				fp->length = strtol((const char *)value, &end, 10);
			} else if(!strcasecmp((char *)key, "icy-name")) {
				fp->title = guess_encoding_of_string((const char *)value);
				fp->gotmetadata = 1;
			} else if(!strcasecmp((char *)key, "icy-genre")) {
				fp->genre = guess_encoding_of_string((const char *)value);
				fp->gotmetadata = 1;
			} else if(!strcasecmp((char *)key, "icy-metaint")) {
				// printf ("icy-metaint: %d\n", atoi (value));
				char *end;
				fp->icy_metaint = (int)strtoul((const char *)value, &end, 10);
				fp->wait_meta = fp->icy_metaint;
			} else if(!strcasecmp((char *)key, "icy-url")) {
				fp->album = guess_encoding_of_string((const char *)value);
				fp->gotmetadata = 1;
			}

			// for icy streams, reset length
			if(!strncasecmp((char *)key, "icy-", 4)) {
				fp->length = -1;
				fp->continuousStream = 1;
			}
		}
	}
	if(!fp->icyheader) {
		fp->gotsomeheader = 1;
	}
	return p - (uint8_t *)ptr;
}

static size_t handle_icy_headers(NSData *httpResponseData, HTTPSource *fp) {
	size_t avail = httpResponseData.length;
	if(avail == 0) return 0;

	const char *ptr = (const char *)httpResponseData.bytes;
	size_t consumed = 0;

	// Check if that's ICY
	if(!fp->icyheader && avail >= 10 && !memcmp(ptr, "ICY 200 OK", 10)) {
		DLog(@"icy headers in the response");
		ptr += 10;
		avail -= 10;
		consumed += 10;

		fp->icyheader = 1;
		fp->continuousStream = 1;

		// Check for termination marker
		if(avail >= 4 && !memcmp(ptr, "\r\n\r\n", 4)) {
			avail -= 4;
			ptr += 4;
			consumed += 4;
			fp->gotheader = 1;

			return consumed;
		}

		// Skip remaining linebreaks
		while(avail > 0 && (*ptr == '\r' || *ptr == '\n')) {
			avail--;
			ptr++;
			consumed++;
		}
	}

	if(fp->icyheader) {
		if(fp->nheaderpackets > 10) {
			DLog(@"urlsession: warning: seems like stream has unterminated ICY headers");
			fp->icy_metaint = 0;
			fp->wait_meta = 0;
			fp->gotheader = 1;
		} else if(avail) {
			fp->nheaderpackets++;

			int end = 0;
			size_t header_consumed = http_content_header_handler_int((void *)ptr, avail, (__bridge void *)fp, &end);
			avail -= header_consumed;
			ptr += header_consumed;
			consumed += header_consumed;
			fp->gotheader = end || (avail != 0);
		}
	} else {
		fp->gotheader = 1;
	}

	return consumed;
}

static size_t _handle_icy_metadata(size_t avail, HTTPSource *fp, char *ptr, int *error) {
	size_t size = avail;
	while(fp->icy_metaint > 0) {
		if(fp->metadata_size > 0) {
			if(fp->metadata_size > fp->metadata_have_size) {
				DLog(@"metadata fetch mode, avail: %zu, metadata_size: %zu, metadata_have_size: %zu)", avail, fp->metadata_size, fp->metadata_have_size);
				size_t sz = (fp->metadata_size - fp->metadata_have_size);
				sz = MIN(sz, avail);
				size_t space = MAX_METADATA - fp->metadata_have_size;
				size_t copysize = MIN(space, sz);
				if(copysize > 0) {
					DLog(@"fetching %zu bytes of metadata (out of %zu)", sz, fp->metadata_size);
					memcpy(fp->metadata + fp->metadata_have_size, ptr, copysize);
				}
				avail -= sz;
				ptr += sz;
				fp->metadata_have_size += sz;
			}
			if(fp->metadata_size == fp->metadata_have_size) {
				size_t sz = fp->metadata_size;
				fp->metadata_size = fp->metadata_have_size = 0;
				if(http_parse_shoutcast_meta(fp, fp->metadata, sz) < 0) {
					fp->metadata_size = 0;
					fp->metadata_have_size = 0;
					fp->wait_meta = 0;
					fp->icy_metaint = 0;
					break;
				}
			}
		}
		if(fp->wait_meta < avail) {
			// read bytes remaining until metadata block
			size_t res1 = http_data_write_wrapper(fp, ptr, fp->wait_meta);
			if(res1 != fp->wait_meta) {
				*error = 1;
				return 0;
			}
			avail -= res1;
			ptr += res1;
			uint32_t sz = (uint32_t)(*((uint8_t *)ptr)) * 16;
			if(sz > MAX_METADATA) {
				DLog(@"metadata size %d is too large\n", sz);
				ptr += sz;
				fp->metadata_size = 0;
				fp->metadata_have_size = 0;
				fp->wait_meta = 0;
				fp->icy_metaint = 0;
				break;
			}
			// assert (sz < MAX_METADATA);
			ptr++;
			fp->metadata_size = sz;
			fp->metadata_have_size = 0;
			fp->wait_meta = fp->icy_metaint;
			avail--;
			if(sz != 0) {
				DLog(@"found metadata block at pos %lld, size: %d (avail=%zu)\n", fp->pos, sz, avail);
			}
		}
		if((!fp->metadata_size || !avail) && fp->wait_meta >= avail) {
			break;
		}
		if(avail < 0) {
			DLog(@"urlsession: something bad happened in metadata parser. can't continue streaming.\n");
			*error = 1;
			return 0;
		}
	}
	return size - avail;
}

static size_t http_data_write(NSData *data, HTTPSource *fp) {
	size_t size = data.length;
	const char *ptr = (const char *)data.bytes;
	size_t avail = size;

	// DLog(@"http_data_write %d bytes, wait_meta=%d\n", (int)size, fp->wait_meta);
	[fp->mutex lock];
	fp->last_read_time = [NSDate date];
	[fp->mutex unlock];

	// Check for abort
	if(fp->need_abort) {
		fp->status = STATUS_ABORTED;
		DLog(@"urlsession STATUS_ABORTED at start of packet");
		return 0;
	}

	// Process the in-stream headers, if present
	if(!fp->gotheader) {
		size_t consumed = handle_icy_headers(data, fp);
		avail -= consumed;
		ptr += consumed;
		if(!avail) {
			return size;
		}
	}

	[fp->mutex lock];
	if(fp->status == STATUS_INITIAL && fp->gotheader) {
		fp->status = STATUS_READING;
	}
	[fp->mutex unlock];

	int error = 0;
	size_t consumed = _handle_icy_metadata(avail, fp, (char *)ptr, &error);
	if(error) {
		return 0;
	}
	avail -= consumed;
	ptr += consumed;

	// The remaining bytes are the normal stream, without metadata or headers
	if(avail) {
		// DLog(@"http_data_write_wrapper [2] %d\n", (int)avail);
		size_t res = http_data_write_wrapper(fp, (char *)ptr, avail);
		avail -= res;
		fp->wait_meta -= res;
	}
	return size - avail;
}

static void http_transport_reset(HTTPSource *fp) {
	fp->gotheader = 0;
	fp->icyheader = 0;
	fp->gotsomeheader = 0;
	fp->metadata_size = 0;
	fp->metadata_have_size = 0;
	fp->nheaderpackets = 0;
	fp->icy_metaint = 0;
	fp->wait_meta = 0;
}

static void http_stream_reset(HTTPSource *fp) {
	http_transport_reset(fp);
	fp->remaining = 0;
	fp->skipbytes = 0;
	http_retry_overlap_clear(fp);
}

static void http_clear_stream_metadata(HTTPSource *fp) {
	fp->album = @"";
	fp->artist = @"";
	fp->title = @"";
	fp->genre = @"";
}

static void http_schedule_retry_locked(HTTPSource *fp) {
	if(fp->continuousStream || fp->length < 0) {
		http_retry_overlap_snapshot_locked(fp);
	}
	fp->status = STATUS_RETRY;
}

- (void)urlSessionThreadEntry:(id)info {
	@autoreleasepool {
		// Create URLSession configuration
		NSURLSessionConfiguration *config = [NSURLSessionConfiguration defaultSessionConfiguration];
		config.timeoutIntervalForRequest = 10;
		config.requestCachePolicy = NSURLRequestReloadIgnoringLocalCacheData;

		// Create delegate queue
		NSOperationQueue *delegateQueue = [NSOperationQueue new];
		delegateQueue.name = @"org.cogx.cog.httpsource";
		delegateQueue.maxConcurrentOperationCount = 1; // Ensure serial operation
		delegateQueue.qualityOfService = NSQualityOfServiceUserInitiated;

		// Create session with self as delegate
		session = [NSURLSession sessionWithConfiguration:config delegate:self delegateQueue:delegateQueue];

		length = -1;
		self->status = STATUS_INITIAL;

		DLog(@"urlsession: started loading data %@", URL);

		// Start initial request
		[self startRequest];

		// Run the thread loop to handle seeks and retries
		for(;;) {
			[mutex lock];
			if(self->status == STATUS_SEEK || self->status == STATUS_RETRY) {
				BOOL retrying = self->status == STATUS_RETRY;
				DLog(@"urlsession: restarting for %@ to %lld\n", retrying ? @"retry" : @"seek", pos);
				skipbytes = 0;
				self->status = STATUS_INITIAL;

				if(dataTask) {
					[dataTask cancel];
					dataTask = nil;
				}

				if(retrying) {
					http_transport_reset(self);
				} else if(length < 0) {
					// icy -- need full restart
					http_stream_reset(self);
					pos = 0;
					content_type = nil;
					seektoend = 0;
				}

				[self startRequest];
			}

			if(self->status == STATUS_FINISHED || self->status == STATUS_ABORTED) {
				[mutex unlock];
				break;
			}
			[mutex unlock];

			usleep(10000); // 10ms sleep to reduce CPU usage
		}

		[session finishTasksAndInvalidate];

		[mutex lock];
		if(self->status == STATUS_ABORTED) {
			DLog(@"urlsession: thread ended due to abort signal");
		} else {
			DLog(@"urlsession: thread ended normally");
		}
		[mutex unlock];
	}
}

- (void)startRequest {
	@autoreleasepool {
		redirectsRemaining = 10;

		NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:URL];
		[request setHTTPMethod:@"GET"];

		// Add ICY metadata request header
		[request setValue:@"1" forHTTPHeaderField:@"Icy-Metadata"];

		// Set User-Agent
		NSString *ua = [NSString stringWithFormat:@"Cog/%@", [[[NSBundle mainBundle] infoDictionary] valueForKey:(__bridge id)kCFBundleVersionKey]];
		[request setValue:ua forHTTPHeaderField:@"User-Agent"];

		// Set timeout
		request.timeoutInterval = 10;

		// Add Range header if seeking and we have content length
		int64_t requestPos = pos + remaining;
		if(requestPos > 0 && length > 0) {
			NSString *rangeHeader = [NSString stringWithFormat:@"bytes=%lld-", requestPos];
			[request setValue:rangeHeader forHTTPHeaderField:@"Range"];
		}

		// Create and start new task
		dataTask = [session dataTaskWithRequest:request];
		[dataTask resume];

		// Update last read time
		last_read_time = [NSDate date];

		DLog(@"urlsession: started request (status=%d)\n", self->status);
	}
}

- (void)URLSession:(NSURLSession *)session dataTask:(NSURLSessionDataTask *)dataTask didReceiveResponse:(NSURLResponse *)response
  completionHandler:(void (^)(NSURLSessionResponseDisposition disposition))completionHandler {
	[mutex lock];

	if(dataTask != self->dataTask) {
		DLog(@"urlsession: ignoring stale response");
		[mutex unlock];
		completionHandler(NSURLSessionResponseCancel);
		return;
	}

	DLog(@"urlsession: received response");

	NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *)response;
	NSDictionary *headers = httpResponse.allHeaderFields;

	// Check for ICY response
	NSString *contentType = httpResponse.MIMEType;
	if(contentType && ([contentType hasPrefix:@"audio/x-scpls"] ||
					   [contentType hasPrefix:@"audio/mpegurl"] ||
					   [contentType hasPrefix:@"application/x-scpls"])) {
		// Likely ICY stream, handle specially
		[mutex unlock];
		completionHandler(NSURLSessionResponseAllow);
		return;
	}

	// Process HTTP headers using existing logic
	NSMutableData *headerBlock = [NSMutableData new];

	for (NSString *key in headers) {
		NSString *value = headers[key];

		const char *key_c = [key UTF8String];
		const char *value_c = [value UTF8String];

		[headerBlock appendBytes:key_c length:strlen(key_c)];
		[headerBlock appendBytes:": " length:2];
		[headerBlock appendBytes:value_c length:strlen(value_c)];
		[headerBlock appendBytes:"\r\n" length:2];
	}

	[headerBlock appendBytes:"\r\n" length:2];

	// Process the header block
	int end = 0;
	http_content_header_handler_int((void *)[headerBlock bytes], [headerBlock length], (__bridge void *)self, &end);

	self->gotheader = 1;
	if(self->status == STATUS_INITIAL) {
		self->status = STATUS_READING;
	}

	[mutex unlock];

	completionHandler(NSURLSessionResponseAllow);
}

- (void)URLSession:(NSURLSession *)session dataTask:(NSURLSessionDataTask *)dataTask didReceiveData:(NSData *)data {
	[mutex lock];
	BOOL stale = (dataTask != self->dataTask || need_abort);
	[mutex unlock];
	if(stale) {
		DLog(@"urlsession: ignoring stale data packet");
		return;
	}

	http_data_write(data, self);
}

- (void)URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task didCompleteWithError:(NSError *)error {
	[mutex lock];

	if(task != self->dataTask) {
		DLog(@"urlsession: ignoring stale completion");
		[mutex unlock];
		return;
	}

	BOOL transferStarted = gotheader || pos > 0 || remaining > 0;
	if(need_abort) {
		self->status = STATUS_ABORTED;
	} else if(self->status == STATUS_SEEK || self->status == STATUS_RETRY) {
		// Preserve an explicitly requested restart until the worker thread handles it.
	} else if(error) {
		DLog(@"urlsession: task completed with error: %@", error.localizedDescription);
		if(transferStarted) {
			http_schedule_retry_locked(self);
		} else {
			self->status = STATUS_ABORTED;
		}
	} else {
		DLog(@"urlsession: task completed successfully");
		if(continuousStream && transferStarted) {
			http_schedule_retry_locked(self);
		} else if(self->status == STATUS_READING || self->status == STATUS_INITIAL) {
			self->status = STATUS_FINISHED;
		}
	}

	self->dataTask = nil;

	[mutex unlock];
}

- (void)URLSession:(NSURLSession *)session didReceiveChallenge:(NSURLAuthenticationChallenge *)challenge completionHandler:(void
 (^)(NSURLSessionAuthChallengeDisposition disposition, NSURLCredential *credential))completionHandler {
	if([challenge.protectionSpace.authenticationMethod isEqualToString:NSURLAuthenticationMethodServerTrust]) {
		BOOL sslVerify = ![[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"allowInsecureSSL"];

		if(!sslVerify) {
			// Allow insecure SSL
			NSURLCredential *credential = [NSURLCredential credentialForTrust:challenge.protectionSpace.serverTrust];
			completionHandler(NSURLSessionAuthChallengeUseCredential, credential);
			DLog(@"urlsession: allowing insecure SSL");
			return;
		} else {
			// Verify SSL properly
			SecTrustRef serverTrust = challenge.protectionSpace.serverTrust;
			SecTrustResultType result = kSecTrustResultInvalid;
			CFErrorRef error = nil;
			if(serverTrust) {
				CFDataRef exceptions = SecTrustCopyExceptions(serverTrust);
				if(exceptions) {
					SecTrustSetExceptions(serverTrust, exceptions);
					CFRelease(exceptions);
				}
				if(SecTrustEvaluateWithError(serverTrust, &error)) {
					result = kSecTrustResultProceed;
				} else {
					result = kSecTrustResultOtherError;
					CFRelease(error);
				}
			}

			if(result == kSecTrustResultProceed || result == kSecTrustResultUnspecified) {
				NSURLCredential *credential = [NSURLCredential credentialForTrust:challenge.protectionSpace.serverTrust];
				completionHandler(NSURLSessionAuthChallengeUseCredential, credential);
				DLog(@"urlsession: SSL verified");
				return;
			} else {
				completionHandler(NSURLSessionAuthChallengeCancelAuthenticationChallenge, nil);
				DLog(@"urlsession: SSL verification failed");
				return;
			}
		}
	}

	// Default handling for other authentication types
	completionHandler(NSURLSessionAuthChallengePerformDefaultHandling, nil);
}

- (void) URLSession:(NSURLSession *)session task:(NSURLSessionTask *)task willPerformHTTPRedirection:(NSHTTPURLResponse *)response newRequest:(NSURLRequest *)request completionHandler:(void (^)(NSURLRequest *))completionHandler {
	if(redirectsRemaining > 0) {
		--redirectsRemaining;
		completionHandler(request);
	} else {
		completionHandler(NULL);
	}
}

- (BOOL)open:(NSURL *)url {
	URL = url;

	if(!http_allocate_buffers(self)) {
		DLog(@"urlsession: failed to allocate streaming buffer");
		return NO;
	}

	mutex = [NSLock new];

	need_abort = NO;

	status = STATUS_UNSTARTED;

	pos = 0;
	length = 0;
	remaining = 0;
	skipbytes = 0;

	nheaderpackets = 0;
	content_type = nil;
	session = nil;
	dataTask = nil;
	last_read_time = nil;
	icy_metaint = 0;
	wait_meta = 0;

	memset(&metadata, 0, sizeof(metadata));
	metadata_size = 0;
	metadata_have_size = 0;

	need_abort = NO;

	album = @"";
	artist = @"";
	title = @"";
	genre = @"";
	gotmetadata = 0;

	seektoend = 0;
	gotheader = 0;
	icyheader = 0;
	gotsomeheader = 0;
	continuousStream = 0;
	http_retry_overlap_clear(self);

	[NSThread detachNewThreadSelector:@selector(urlSessionThreadEntry:) toTarget:self withObject:nil];

	// Wait for transfer to at least start
	while(status == STATUS_UNSTARTED) {
		usleep(3000);
	}

	// Now wait for it to either begin streaming, or complete if file is small enough to fit in the buffer
	while(status != STATUS_READING &&
	      status != STATUS_FINISHED &&
	      status != STATUS_ABORTED &&
	      !dataTask) {
		usleep(3000);
	}

	// Wait for the headers
	while(status != STATUS_FINISHED &&
		  status != STATUS_ABORTED &&
		  !gotheader) {
		usleep(3000);
	}

	if(!dataTask && status != STATUS_FINISHED)
		return NO;

	return YES;
}

- (NSString *)mimeType {
	DLog(@"Returning mimetype! %@", content_type);
	return content_type;
}

- (BOOL)hasMetadata {
	BOOL ret = !!gotmetadata;
	gotmetadata = 0;
	return ret;
}

- (NSDictionary *)metadata {
	const NSString* keys[] = {
		@"genre",
		@"album",
		@"artist",
		@"title"
	};
	const id values[] = {
		genre,
		album,
		artist,
		title
	};
	return [NSDictionary initWithOptionalObjects:values forKeys:keys count:sizeof(keys) / sizeof(keys[0])];
}

- (BOOL)seekable {
	return length > 0;
}

- (BOOL)seek:(long)position whence:(int)whence {
	seektoend = 0;
	if(whence == SEEK_END) {
		if(position == 0) {
			seektoend = 1;
			return YES;
		}
		DLog(@"urlsession: can't seek in stream relative to EOF");
		return NO;
	}
	[mutex lock];
	if(whence == SEEK_CUR) {
		whence = SEEK_SET;
		position = pos + position;
	}
	if(whence == SEEK_SET) {
		if(pos == position) {
			skipbytes = 0;
			[mutex unlock];
			return YES;
		} else if(pos < position && pos + (int64_t)bufferSize > position) {
			skipbytes = position - pos;
			[mutex unlock];
			return YES;
		} else if(pos - position >= 0 && pos - position <= (int64_t)bufferSize - remaining) {
			skipbytes = 0;
			remaining += pos - position;
			pos = position;
			[mutex unlock];
			return YES;
		}
	}
	// Reset stream and start over
	http_stream_reset(self);
	pos = position;
	status = STATUS_SEEK;

	[mutex unlock];
	return YES;
}

- (long)tell {
	if(seektoend) {
		return length;
	}
	return pos + skipbytes;
}

- (long)read:(void *)ptr amount:(long)amount {
	size_t sz = amount;
	while((remaining > 0 || (status != STATUS_FINISHED && status != STATUS_ABORTED)) && sz > 0) {
		// wait until data is available
		while((remaining == 0 || skipbytes > 0) && status != STATUS_FINISHED && status != STATUS_ABORTED) {
			//            DLog(@"urlsession: readwait, status: %d..\n", status);
			[mutex lock];
			if(status == STATUS_READING) {
				NSTimeInterval sec = [[NSDate date] timeIntervalSinceDate:last_read_time];
				if(sec > TIMEOUT) {
					DLog(@"urlsession: timed out, restarting read");
					http_schedule_retry_locked(self);
					[mutex unlock];
					http_clear_stream_metadata(self);
					usleep(3000);
					continue;
				}
			}
			int64_t skip = MIN(remaining, skipbytes);
			if(skip > 0) {
				//                DLog(@"skipping %lld bytes\n", skip);
				pos += skip;
				remaining -= skip;
				skipbytes -= skip;
			}
			[mutex unlock];
			usleep(3000);
		}
		//    DLog(@"buffer remaining: %d\n", remaining);
		[mutex lock];
		// DLog(@"http_read %lld/%lld/%d\n", pos, length, remaining);
		size_t cp = MIN(sz, (size_t)remaining);
		size_t readpos = (size_t)(pos & bufferMask);
		size_t part1 = bufferSize - readpos;
		part1 = MIN(part1, cp);
		//        DLog(@"readpos=%d, remaining=%d, req=%d, cp=%d, part1=%d, part2=%d\n", readpos, remaining, sz, cp, part1, cp-part1);
		memcpy(ptr, buffer + readpos, part1);
		remaining -= part1;
		pos += part1;
		sz -= part1;
		ptr += part1;
		cp -= part1;
		if(cp > 0) {
			memcpy(ptr, buffer, cp);
			remaining -= cp;
			pos += cp;
			sz -= cp;
			ptr += cp;
		}
		[mutex unlock];
	}
	if(status == STATUS_ABORTED) {
		return 0;
	}
	return amount - sz;
}

- (void)close {
	need_abort = YES;
	content_type = nil;
	[mutex lock];
	status = STATUS_ABORTED;
	[mutex unlock];
	if(dataTask) {
		[dataTask cancel];
	}
	while(dataTask != nil) {
		usleep(3000);
	}
}

- (void)dealloc {
	[self close];
	http_free_buffers(self);
}

- (NSURL *)url {
	return URL;
}

+ (NSArray *)schemes {
	return @[@"http", @"https"];
}

@end
