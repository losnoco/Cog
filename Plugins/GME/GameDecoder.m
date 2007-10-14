//
//  GameFile.m
//  Cog
//
//  Created by Vincent Spader on 5/29/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "GameDecoder.h"
#import "GameContainer.h"

@implementation GameDecoder

gme_err_t readCallback( void* data, void* out, long count )
{
	GameDecoder *decoder = (GameDecoder *)data;
	NSLog(@"Amount: %li", count);
	int n = [[decoder source] read:out amount:count];
	NSLog(@"Read: %i", n);
	if (n <= 0) {
		
		NSLog(@"ERROR!");
		return (gme_err_t)1; //Return non-zero for error
	}
	
	return 0;  //Return 0 for no error
}

- (BOOL)open:(id<CogSource>)s
{
	[self setSource:s];
	
	//We need file-size to use GME
	if (![source seekable]) {
		return NO;
	}
	
	gme_err_t error;
	
	NSString *ext = [[[[source url] path] pathExtension] lowercaseString];
	
	gme_type_t type = gme_identify_extension([ext UTF8String]);
	if (!type) 
	{
		NSLog(@"No type!");
		return NO;
	}
	
	emu = gme_new_emu(type, 44100);
	if (!emu)
	{
		NSLog(@"No new emu!");
		return NO;
	}
	
	[source seek:0 whence:SEEK_END];
	long size = [source tell];
	[source seek:0 whence:SEEK_SET];
	
	NSLog(@"Size: %li", size);
	
	error = gme_load_custom(emu, readCallback, size, self);
	if (error) 
	{
		NSLog(@"ERROR Loding custom!");
		return NO;
	}
	
	int track_num = [[[source url] fragment] intValue]; //What if theres no fragment? Assuming we get 0.
	
	track_info_t info;
	error = gme_track_info( emu, &info, track_num );
	if (error)
	{
		NSLog(@"Unable to get track info");
	}
	
	//As recommended
	if (info.length > 0) {
		NSLog(@"Using length: %li", info.length);
		length = info.length;
	}
	else if (info.loop_length > 0) {
		NSLog(@"Using loop length: %li", info.loop_length);
		length = info.intro_length + 2*info.loop_length;
	}
	else {
		length = 150000; 
		NSLog(@"Setting default: %li", length);
	}

	NSLog(@"Length: %li", length);
	
	NSLog(@"Track num: %i", track_num);
	error = gme_start_track(emu, track_num);
	if (error) 
	{
		NSLog(@"Error starting track");
		return NO;
	}

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];
	
	return YES;
}

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:0], @"bitrate",
		[NSNumber numberWithFloat:44100], @"sampleRate",
		[NSNumber numberWithLong:length], @"length",
		[NSNumber numberWithInt:sizeof(short)*8], @"bitsPerSample", //Samples are short
		[NSNumber numberWithInt:2], @"channels", //output from gme_play is in stereo
		[NSNumber numberWithBool:[source seekable]], @"seekable",
		@"host", @"endian",
		nil];
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	int numSamples = size / (sizeof(short));
	
	if (gme_track_ended(emu) || length < gme_tell(emu)) {
		return 0;
	}
	
	gme_play(emu, numSamples, (short int *)buf);
	
	//Some formats support length, but we'll add that in the future.
	//(From gme.txt) If track length, then use it. If loop length, play for intro + loop * 2. Otherwise, default to 2.5 minutes
	return size; //GME will always generate samples. There's no real EOS.
}

- (double)seekToTime:(double)milliseconds
{
	gme_err_t error;
	error = gme_seek(emu, milliseconds);
	if (error) {
		return -1;
	}
	
	return milliseconds;
}

- (void)close
{
	if (emu) {
		gme_delete(emu);
		emu = NULL;
	}
	if (source) {
		[source close];
		[self setSource:nil];
	}
}

+ (NSArray *)fileTypes 
{	
	return [GameContainer fileTypes];
}

+ (NSArray *)mimeTypes 
{	
	return [GameContainer fileTypes];
}

- (void)setSource:(id<CogSource>)s
{
	[s retain];
	[source release];
	source = s;
}

- (id<CogSource>)source
{
	return source;
}

@end
