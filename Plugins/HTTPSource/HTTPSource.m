//
//  HTTPSource.m
//  HTTPSource
//
//  Created by Vincent Spader on 3/1/07.
//  Replaced by Christopher Snowhill on 3/7/20.
//  Copyright 2020-2025 __LoSnoCo__. All rights reserved.
//

#import "HTTPSource.h"

#import "Logging.h"

#import "NSDictionary+Optional.h"

#import <stdlib.h>
#import <string.h>

@implementation HTTPSource

static size_t http_curl_write_wrapper(HTTPSource *fp, void *ptr, size_t size) {
	size_t avail = size;
	while(avail > 0) {
		[fp->mutex lock];
		if(fp->status == STATUS_SEEK) {
			DLog(@"curl seek request, aborting current request");
			[fp->mutex unlock];
			return 0;
		}
		if(fp->need_abort) {
			fp->status = STATUS_ABORTED;
			DLog(@"curl STATUS_ABORTED in the middle of packet");
			[fp->mutex unlock];
			break;
		}
		int sz = BUFFER_SIZE / 2 - fp->remaining; // number of bytes free in buffer
		                                          // don't allow to fill more than half -- used for seeking backwards

		if(sz > 5000) { // wait until there are at least 5k bytes free
			size_t cp = MIN(avail, sz);
			int writepos = (fp->pos + fp->remaining) & BUFFER_MASK;
			// copy 1st portion (before end of buffer
			size_t part1 = BUFFER_SIZE - writepos;
			// may not be more than total
			part1 = MIN(part1, cp);
			memcpy(fp->buffer + writepos, ptr, part1);
			ptr += part1;
			avail -= part1;
			fp->remaining += part1;
			cp -= part1;
			if(cp > 0) {
				memcpy(fp->buffer, ptr, cp);
				ptr += cp;
				avail -= cp;
				fp->remaining += cp;
			}
		}
		[fp->mutex unlock];
		usleep(3000);
	}
	return size - avail;
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
		}

		// for icy streams, reset length
		if(!strncasecmp((char *)key, "icy-", 4)) {
			fp->length = -1;
		}
	}
	if(!fp->icyheader) {
		fp->gotsomeheader = 1;
	}
	return p - (uint8_t *)ptr;
}

static size_t handle_icy_headers(size_t avail, HTTPSource *fp, char *ptr) {
	size_t size = avail;

	// check if that's ICY
	if(!fp->icyheader && avail >= 10 && !memcmp(ptr, "ICY 200 OK", 10)) {
		DLog(@"icy headers in the stream");
		ptr += 10;
		avail -= 10;

		fp->icyheader = 1;

		// check for termination marker
		if(avail >= 4 && !memcmp(ptr, "\r\n\r\n", 4)) {
			avail -= 4;
			ptr += 4;
			fp->gotheader = 1;

			return size - avail;
		}

		// skip remaining linebreaks
		while(avail > 0 && (*ptr == '\r' || *ptr == '\n')) {
			avail--;
			ptr++;
		}
	}
	if(fp->icyheader) {
		if(fp->nheaderpackets > 10) {
			DLog(@"curl: warning: seems like stream has unterminated ICY headers");
			fp->icy_metaint = 0;
			fp->wait_meta = 0;
			fp->gotheader = 1;
		} else if(avail) {
			fp->nheaderpackets++;

			int end = 0;
			size_t consumed = http_content_header_handler_int(ptr, avail, (__bridge void *)fp, &end);
			avail -= consumed;
			ptr += consumed;
			fp->gotheader = end || (avail != 0);
		}
	} else {
		fp->gotheader = 1;
	}
	if(!avail) {
		return size;
	}

	return size - avail;
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
			size_t res1 = http_curl_write_wrapper(fp, ptr, fp->wait_meta);
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
			DLog(@"curl: something bad happened in metadata parser. can't continue streaming.\n");
			*error = 1;
			return 0;
		}
	}
	return size - avail;
}

static size_t http_curl_write(void *_ptr, size_t size, size_t nmemb, void *stream) {
	char *ptr = _ptr;
	size_t avail = size * nmemb;
	HTTPSource *fp = (__bridge HTTPSource *)stream;

	//    DLog(@"http_curl_write %d bytes, wait_meta=%d\n", size * nmemb, fp->wait_meta);
	gettimeofday(&fp->last_read_time, NULL);
	if(fp->need_abort) {
		fp->status = STATUS_ABORTED;
		DLog(@"curl STATUS_ABORTED at start of packet");
		return 0;
	}

	// process the in-stream headers, if present
	if(!fp->gotheader) {
		size_t consumed = handle_icy_headers(avail, fp, ptr);
		avail -= consumed;
		ptr += consumed;
		if(!avail) {
			return nmemb * size;
		}
	}

	[fp->mutex lock];
	if(fp->status == STATUS_INITIAL && fp->gotheader) {
		fp->status = STATUS_READING;
	}
	[fp->mutex unlock];

	int error = 0;
	size_t consumed = _handle_icy_metadata(avail, fp, ptr, &error);
	if(error) {
		return 0;
	}
	avail -= consumed;
	ptr += consumed;

	// the remaining bytes are the normal stream, without metadata or headers
	if(avail) {
		//        DLog(@"http_curl_write_wrapper [2] %d\n", avail);
		size_t res = http_curl_write_wrapper(fp, ptr, avail);
		avail -= res;
		fp->wait_meta -= res;
	}
	return nmemb * size - avail;
}

static size_t http_content_header_handler(void *ptr, size_t size, size_t nmemb, void *stream) {
	int end = 0;
	return http_content_header_handler_int(ptr, size * nmemb, stream, &end);
}

static int http_curl_control(void *stream, double dltotal, double dlnow, double ultotal, double ulnow) {
	HTTPSource *fp = (__bridge HTTPSource *)stream;

	[fp->mutex lock];

	struct timeval tm;
	gettimeofday(&tm, NULL);
	float sec = tm.tv_sec - fp->last_read_time.tv_sec;
	long response;
	curl_easy_getinfo(fp->curl, CURLINFO_RESPONSE_CODE, &response);
	// DLog ("http_curl_control: status = %d, response = %d, interval: %f seconds\n", fp ? fp->status : -1, (int)response, sec);
	if(fp->status == STATUS_READING && sec > TIMEOUT) {
		DLog(@"http_curl_control: timed out, restarting read");
		memcpy(&fp->last_read_time, &tm, sizeof(struct timeval));
		http_stream_reset(fp);
		fp->status = STATUS_SEEK;
	} else if(fp->status == STATUS_SEEK) {
		DLog(@"curl STATUS_SEEK in progress callback");
		[fp->mutex unlock];
		return -1;
	}
	if(fp->need_abort) {
		fp->status = STATUS_ABORTED;
		DLog(@"curl STATUS_ABORTED in progress callback");
		[fp->mutex unlock];
		return -1;
	}
	[fp->mutex unlock];
	return 0;
}

static void http_stream_reset(HTTPSource *fp) {
	fp->gotheader = 0;
	fp->icyheader = 0;
	fp->gotsomeheader = 0;
	fp->remaining = 0;
	fp->metadata_size = 0;
	fp->metadata_have_size = 0;
	fp->skipbytes = 0;
	fp->nheaderpackets = 0;
	fp->icy_metaint = 0;
	fp->wait_meta = 0;
}

- (void)threadEntry:(id)info {
	@autoreleasepool {
		CURL *curl;
		curl = curl_easy_init();
		length = -1;
		self->curl = curl;
		self->status = STATUS_INITIAL;

		int status;

		DLog(@"curl: started loading data %@", URL);
		for(;;) {
			struct curl_slist *headers = NULL;
			struct curl_slist *ok_aliases = curl_slist_append(NULL, "ICY 200 OK");

			BOOL sslVerify = ![[[NSUserDefaultsController sharedUserDefaultsController] defaults] boolForKey:@"allowInsecureSSL"];

			curl_easy_reset(curl);
			curl_easy_setopt(curl, CURLOPT_URL, [[URL absoluteString] UTF8String]);
			NSString *ua = [NSString stringWithFormat:@"Cog/%@", [[[NSBundle mainBundle] infoDictionary] valueForKey:(__bridge NSString *)kCFBundleVersionKey]];
			curl_easy_setopt(curl, CURLOPT_USERAGENT, [ua UTF8String]);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_curl_write);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (__bridge void *)self);
			curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, http_err);
			curl_easy_setopt(curl, CURLOPT_BUFFERSIZE, BUFFER_SIZE / 2);
			curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
			curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, http_content_header_handler);
			curl_easy_setopt(curl, CURLOPT_HEADERDATA, (__bridge void *)self);
			curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
			curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, http_curl_control);
			curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
			curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, (__bridge void *)self);
			// enable up to 10 redirects
			curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
			curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10);

			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);

			headers = curl_slist_append(headers, "Icy-Metadata:1");
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_HTTP200ALIASES, ok_aliases);
			if(pos > 0 && length >= 0) {
				curl_easy_setopt(curl, CURLOPT_RESUME_FROM, (long)pos);
			}
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, (long)sslVerify);
			//        fp->status = STATUS_INITIAL;
			DLog(@"curl: calling curl_easy_perform (status=%d)...\n", self->status);
			gettimeofday(&last_read_time, NULL);
			status = curl_easy_perform(curl);
			DLog(@"curl: curl_easy_perform retval=%d\n", status);
			if(status != 0) {
				DLog(@"curl error:\n%s\n", http_err);
			}
			[mutex lock];
			if(self->status != STATUS_SEEK) {
				DLog(@"curl: break loop\n");
				[mutex unlock];
				break;
			} else {
				DLog(@"curl: restart loop\n");
				skipbytes = 0;
				self->status = STATUS_INITIAL;
				DLog(@"seeking to %lld\n", pos);
				if(length < 0) {
					// icy -- need full restart
					pos = 0;
					content_type = nil;
					seektoend = 0;
					gotheader = 0;
					icyheader = 0;
					gotsomeheader = 0;
					wait_meta = 0;
					icy_metaint = 0;
				}
			}
			[mutex unlock];
			curl_slist_free_all(headers);
			curl_slist_free_all(ok_aliases);
		}
		curl_easy_cleanup(curl);

		[mutex lock];
		if(self->status == STATUS_ABORTED) {
			DLog(@"curl: thread ended due to abort signal");
			self->curl = NULL;
		} else {
			DLog(@"curl: thread ended normally");
			self->status = STATUS_FINISHED;
		}
		[mutex unlock];
	}
}

- (BOOL)open:(NSURL *)url {
	URL = url;

	mutex = [[NSLock alloc] init];

	need_abort = NO;

	status = STATUS_UNSTARTED;

	pos = 0;
	length = 0;
	remaining = 0;
	skipbytes = 0;
	memset(buffer, 0, sizeof(buffer));

	nheaderpackets = 0;
	content_type = nil;
	curl = NULL;
	memset(&last_read_time, 0, sizeof(last_read_time));
	icy_metaint = 0;
	wait_meta = 0;

	memset(&metadata, 0, sizeof(metadata));
	metadata_size = 0;
	metadata_have_size = 0;

	memset(&http_err, 0, sizeof(http_err));

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

	[NSThread detachNewThreadSelector:@selector(threadEntry:) toTarget:self withObject:nil];

	// Wait for transfer to at least start
	while(status == STATUS_UNSTARTED) {
		usleep(3000);
	}

	// Now wait for it to either begin streaming, or complete if file is small enough to fit in the buffer
	while(status != STATUS_READING &&
	      status != STATUS_FINISHED &&
	      curl) {
		usleep(3000);
	}

	if(!curl)
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
		DLog(@"curl: can't seek in curl stream relative to EOF");
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
		} else if(pos < position && pos + BUFFER_SIZE > position) {
			skipbytes = position - pos;
			[mutex unlock];
			return YES;
		} else if(pos - position >= 0 && pos - position <= BUFFER_SIZE - remaining) {
			skipbytes = 0;
			remaining += pos - position;
			pos = position;
			[mutex unlock];
			return YES;
		}
	}
	// reset stream, and start over
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
			//            DLog(@"curl: readwait, status: %d..\n", status);
			[mutex lock];
			if(status == STATUS_READING) {
				struct timeval tm;
				gettimeofday(&tm, NULL);
				float sec = tm.tv_sec - last_read_time.tv_sec;
				if(sec > TIMEOUT) {
					DLog(@"http_read: timed out, restarting read");
					memcpy(&last_read_time, &tm, sizeof(struct timeval));
					http_stream_reset(self);
					status = STATUS_SEEK;
					[mutex unlock];
					album = @"";
					artist = @"";
					title = @"";
					genre = @"";
					return 0;
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
		size_t cp = MIN(sz, remaining);
		int64_t readpos = pos & BUFFER_MASK;
		size_t part1 = BUFFER_SIZE - readpos;
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
	while(curl != NULL && status != STATUS_FINISHED) {
		usleep(3000);
	}
}

- (void)dealloc {
	[self close];
}

- (NSURL *)url {
	return URL;
}

+ (NSArray *)schemes {
	return @[@"http", @"https"];
}

@end
