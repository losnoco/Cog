//
//  ShnFile.mm
//  Cog
//
//  Created by Vincent Spader on 6/6/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "ShortenDecoder.h"

@implementation ShortenDecoder

- (BOOL)open:(id<CogSource>)source {
	NSURL *url = [source url];

	if(![[url scheme] isEqualToString:@"file"])
		return NO;

	decoder = new shn_reader;

	if(!decoder) {
		return NO;
	}

	decoder->open([[url path] UTF8String], true);

	bufferSize = decoder->shn_get_buffer_block_size(NUM_DEFAULT_BUFFER_BLOCKS);

	bool seekTable;
	decoder->file_info(NULL, &channels, &frequency, NULL, &bitsPerSample, &seekTable);

	seekable = seekTable == true ? YES : NO;

	totalFrames = (decoder->shn_get_song_length() * frequency) / 1000.0;

	decoder->go();

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (AudioChunk *)readAudio {
	long frames = 1024;
	long bytesPerFrame = channels * (bitsPerSample / 8);
	long amountRead;

	id audioChunkClass = NSClassFromString(@"AudioChunk");
	AudioChunk *chunk = [[audioChunkClass alloc] initWithProperties:[self properties]];

	uint8_t buffer[bytesPerFrame * 1024];
	void *buf = (void *)buffer;

	// For some reason a busy loop is causing pops when output is set to 48000. Probably CPU starvation, since the SHN decoder seems to use a multithreaded nonblocking approach.
	do {
		amountRead = decoder->read(buf, frames * bytesPerFrame);
	} while(amountRead == -1);

	[chunk assignSamples:buf frameCount:amountRead / bytesPerFrame];

	return chunk;
}

- (long)seek:(long)sample {
	unsigned int sec = sample / frequency;

	decoder->seek(sec);

	return sample;
}

- (void)close {
	if(decoder) {
		decoder->exit();
		delete decoder;
		decoder = NULL;
	}

	/*if (shn_cleanup_decoder(handle))
	    shn_unload(handle);*/
}

- (void)dealloc {
	[self close];
}

- (NSDictionary *)properties {
	return @{ @"channels": @(channels),
		      @"bitsPerSample": @(bitsPerSample),
		      @"sampleRate": @(frequency),
		      @"totalFrames": @(totalFrames),
		      @"seekable": @(seekable),
		      @"codec": @"Shorten",
		      @"endian": @"little",
		      @"encoding": @"lossless" };
}

- (NSDictionary *)metadata {
	return @{};
}

+ (NSArray *)fileTypes {
	return @[@"shn"];
}

+ (NSArray *)mimeTypes {
	return @[@"application/x-shorten"]; // This is basically useless, since we cant stream shorten yet
}

+ (float)priority {
	return 1.0;
}

+ (NSArray *)fileTypeAssociations {
	return @[
		@[@"Shorten File", @"shn.icns", @"shn"]
	];
}

@end
