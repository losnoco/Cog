//
//  HTTPSource.h
//  HTTPSource
//
//  Created by Vincent Spader on 3/1/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "Plugin.h"

#include <curl/curl.h>

#define BUFFER_SIZE 0x10000
#define BUFFER_MASK 0xffff

#define MAX_METADATA 1024

#define TIMEOUT 10 // in seconds

enum {
	STATUS_UNSTARTED = 0,
	STATUS_INITIAL = 1,
	STATUS_READING = 2,
	STATUS_FINISHED = 3,
	STATUS_ABORTED = 4,
	STATUS_SEEK = 5,
};

@interface HTTPSource : NSObject <CogSource> {
	NSURL *URL;

	int64_t pos; // position in stream; use "& BUFFER_MASK" to make it index into ringbuffer
	int64_t length;
	int32_t remaining; // remaining bytes in buffer read from stream
	int64_t skipbytes;
	uint8_t buffer[BUFFER_SIZE];

	NSLock *mutex;

	uint8_t nheaderpackets;
	NSString *content_type;
	CURL *curl;
	struct timeval last_read_time;
	uint8_t status;
	int icy_metaint;
	int wait_meta;

	char metadata[MAX_METADATA];
	size_t metadata_size; // size of metadata in stream
	size_t metadata_have_size; // amount which is already in metadata buffer

	char http_err[CURL_ERROR_SIZE];

	BOOL need_abort;

	NSString *album;
	NSString *artist;
	NSString *title;
	NSString *genre;

	// flags (bitfields to save some space)
	unsigned seektoend : 1; // indicates that next tell must return length
	unsigned gotheader : 1; // tells that all headers (including ICY) were processed (to start reading body)
	unsigned icyheader : 1; // tells that we're currently reading ICY headers
	unsigned gotsomeheader : 1; // tells that we got some headers before body started
	unsigned gotmetadata : 1; // got some metadata
}

- (BOOL)hasMetadata;
- (NSDictionary *)metadata;
@end
