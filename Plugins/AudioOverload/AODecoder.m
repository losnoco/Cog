//
//  AODecoder.m
//  AudioOverload
//
//  Created by Vincent Spader on 2/28/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "AODecoder.h"

#import <AudioOverload/ao.h>
#import <AudioOverload/eng_protos.h>
#import "GlobalLock.h"

// WARNING: THIS IS IN NO WAY THREAD SAFE. WE SHOULD PROBABLY TAKE A GLOBAL LOCK AROUND THIS PLUGIN. This may cause all kinds of nastyness, but we'll see.
static GlobalLock *globalLock;

@implementation AODecoder

static struct 
{ 
	uint32_t sig; 
	char *name; 
	int32_t (*start)(uint8_t *, uint32_t); 
	int32_t (*gen)(int16_t *, uint32_t); 
	int32_t (*stop)(void); 
	int32_t (*command)(int32_t, int32_t); 
	uint32_t rate; 
	int32_t (*fillinfo)(ao_display_info *); 
} types[] = {
{ 0x50534641, "Capcom QSound (.qsf)", qsf_start, qsf_gen, qsf_stop, qsf_command, 60, qsf_fill_info },
{ 0x50534611, "Sega Saturn (.ssf)", ssf_start, ssf_gen, ssf_stop, ssf_command, 60, ssf_fill_info },
{ 0x50534601, "Sony PlayStation (.psf)", psf_start, psf_gen, psf_stop, psf_command, 60, psf_fill_info },
{ 0x53505500, "Sony PlayStation (.spu)", spu_start, spu_gen, spu_stop, spu_command, 60, spu_fill_info },
{ 0x50534602, "Sony PlayStation 2 (.psf2)", psf2_start, psf2_gen, psf2_stop, psf2_command, 60, psf2_fill_info },
{ 0x50534612, "Sega Dreamcast (.dsf)", dsf_start, dsf_gen, dsf_stop, dsf_command, 60, dsf_fill_info },

{ 0xffffffff, "", NULL, NULL, NULL, NULL, 0, NULL }
};

static id currentSource;

int ao_get_lib(char *fn, uint8 **buf, uint64 *length)
{
	NSString *fileName = [NSString stringWithUTF8String:fn];
	NSString *path = [[[[currentSource url] path] stringByDeletingLastPathComponent] stringByAppendingPathComponent:fileName];
	NSURL *url = [NSURL fileURLWithPath:path];
	
	id audioSourceClass = NSClassFromString(@"AudioSource");
	id<CogSource> source = [audioSourceClass audioSourceForURL:url];

	NSLog(@"Loading auxiliary file %s, at %@", fn, url);
	
	if (![source open:url]) {
		NSLog(@"Error: Could not open file: %@", url);
		return AO_FAIL;
	}
	
	if (![source seekable]) {
		NSLog(@"Error source not seekable or not a file url");
		return AO_FAIL;
	}
	
	[source seek:0 whence:SEEK_END];
	long size = [source tell];
	[source seek:0 whence:SEEK_SET];
	
	uint8_t *filebuf = malloc(size);
	if (!filebuf) {
		[source close];
		NSLog(@"ERROR: could not allocate %d bytes of memory\n", size);
		return AO_FAIL;
	}
	
	long amountRead = 0;
	while (amountRead < size) {
		int read = [source read:filebuf+amountRead amount:size - amountRead];
		amountRead += read;
	}
	[source close];
	
	*buf = filebuf;
	*length = (uint64)size;
	
	NSLog(@"Aux file successfully loaded!");
	return AO_SUCCESS;
}


+ (void)initialize
{
	if ([self class] == [AODecoder class]) {
		globalLock = [[GlobalLock alloc] init];

		ao_set_get_lib_callback(ao_get_lib);
	}
}

- (NSDictionary *)metadata
{
	ao_display_info info;
	if ((*types[type].fillinfo)(&info) == AO_SUCCESS)
	{
		NSMutableDictionary *dict = [NSMutableDictionary dictionary];
		
		for (int i = 1 ; i < 9; i++) {
			NSString *key = [[NSString alloc] initWithUTF8String:info.title[i]];
			NSString *value = [[NSString alloc] initWithUTF8String:info.info[i]];
			
			if ([key hasPrefix:@"Name"]) {
				[dict setObject:value forKey:@"title"];
			}
			else if ([key hasPrefix:@"Artist"]) {
				[dict setObject:value forKey:@"artist"];
			}
			else if ([key hasPrefix:@"Game"]) {
				[dict setObject:value forKey:@"album"];
			}
			else if ([key hasPrefix:@"Year"]) {
				[dict setObject:value forKey:@"year"];
			}
			
			[key release];
			[value release];
		}		
		
		return dict;
	}
	
	return nil;
}

- (long long)retrieveTotalFrames
{
	long long frames = 0;
	
	ao_display_info info;
	if ((*types[type].fillinfo)(&info) == AO_SUCCESS)
	{
		for (int i = 1 ; i < 9; i++) {
			NSString *key = [[NSString alloc] initWithUTF8String:info.title[i]];
			NSString *value = [[NSString alloc] initWithUTF8String:info.info[i]];
			
			if ([key hasPrefix:@"Length"]) {
				NSArray *components = [value componentsSeparatedByString:@":"];
				
				long totalSeconds = 0;
				long multiplier = 1;
				for (id component in [components reverseObjectEnumerator]) {
					totalSeconds += [component integerValue] * multiplier;
					multiplier *=60;
				}
				
				frames = (totalSeconds * 44100);
			}
			
			[key release];
			[value release];
		}		
	}
	
	return frames;
}

- (BOOL)openUnderLock:(id<CogSource>)source
{
	if (![source seekable] || ![[source url] isFileURL]) {
		return NO;
	}
	
	currentSource = [source retain];
	
	[source seek:0 whence:SEEK_END];
	long size = [source tell];
	[source seek:0 whence:SEEK_SET];
	
	buffer = malloc(size);
	if (!buffer) {
		return NO;
	}
	
	long amountRead = 0;
	while (amountRead < size) {
		int read = [source read:buffer+amountRead amount:size - amountRead];
		amountRead += read;
	}
	
	[source close];
	
	type = 0;
	int filesig = buffer[0]<<24 | buffer[1]<<16 | buffer[2]<<8 | buffer[3];
	while (types[type].sig != 0xffffffff)
	{
		if (filesig == types[type].sig)
		{
			break;
		}
		else
		{
			type++;
		}
	}
	
	// now did we identify it above or just fall through?
	if (types[type].sig != 0xffffffff) {
		NSLog(@"File identified as %s\n", types[type].name);
	}
	else
	{
		NSLog(@"ERROR: File is unknown, signature bytes are %02x %02x %02x %02x\n", buffer[0], buffer[1], buffer[2], buffer[3]);

		return NO;
	}
	
	
	if ((*types[type].start)(buffer, size) != AO_SUCCESS)
	{
		NSLog(@"ERROR: Engine rejected file!\n");
		
		return NO;
	}

	totalFrames = [self retrieveTotalFrames];
	
	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];
	
	return YES;
}

- (BOOL)open:(id<CogSource>)source
{
	[globalLock lock];
	
	BOOL ret = [self openUnderLock:source];
	if (!ret) {
		[self close];
	}
	return ret;
}


- (int)readAudio:(void *)buf frames:(UInt32)frames
{
	if (totalFrames > 0 && framesRead > totalFrames) {
		[self close];
		return 0;
	}
	
	const int maxFrames = 1470; // The Dreamcast decoder starts smashing things if you try decoding more than this
	
	int amountRead = 0;
	while (amountRead < frames) {
		int requestAmount = frames - amountRead;
		if (requestAmount > maxFrames) {
			requestAmount = maxFrames;
		}
		
		(*types[type].gen)((int16_t *)((uint32_t *)buf + amountRead), requestAmount);
		
		amountRead += requestAmount;
	}

	framesRead += frames;

	return frames;
}

- (void)closeUnderLock
{
	(*types[type ].stop)();
	if (NULL != buffer) {
		free(buffer);
		buffer = NULL;
	}
	
	[currentSource release];
	currentSource = nil;
}

- (void)close
{
	if (!closed) {
		closed = YES;
		
		[self closeUnderLock];
		
		[globalLock unlock];
	}
}

- (long)seek:(long)frame
{
	return frame;
}

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
			[NSNumber numberWithInt:2], @"channels",
			[NSNumber numberWithInt:16], @"bitsPerSample",
			[NSNumber numberWithFloat:44100], @"sampleRate",
			[NSNumber numberWithInteger:totalFrames], @"totalFrames",
			[NSNumber numberWithInt:0], @"bitrate",
			[NSNumber numberWithBool:NO], @"seekable",
			nil];
}

+ (NSDictionary *)metadataForURL:(NSURL *)url
{
	[globalLock lock];
	id audioSourceClass = NSClassFromString(@"AudioSource");
	id<CogSource> source = [audioSourceClass audioSourceForURL:url];
	[source open:url];
	
	AODecoder *decoder = [[self alloc] init];
	if (![decoder openUnderLock:source]) {
		[globalLock unlock];
		return nil;
	}
	
	NSDictionary *metadata = [decoder metadata];
	
	[decoder closeUnderLock];
	[decoder release];
	
	[globalLock unlock];
	
	return metadata;
}

+ (NSArray *)fileTypes
{
	return [NSArray arrayWithObjects:@"psf",@"minipsf",@"psf2", @"minipsf2", @"spu", @"ssf", @"minissf", @"dsf", @"minidsf", @"qsf", @"miniqsf", nil];
}

+ (NSArray *)mimeTypes
{
	return [NSArray arrayWithObjects:@"audio/x-psf", nil];
}



@end
