//
//  GameFile.m
//  Cog
//
//  Created by Vincent Spader on 5/29/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "GameDecoder.h"

#import "Logging.h"

@implementation GameDecoder

gme_err_t readCallback( void* data, void* out, long count )
{
    id source = (id)data;
	DLog(@"Amount: %li", count);
	int n = [source read:out amount:count];
	DLog(@"Read: %i", n);
	if (n <= 0) {
		
		DLog(@"ERROR!");
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
		ALog(@"GME: No type!");
		return NO;
	}
	
	emu = gme_new_emu(type, 44100);
	if (!emu)
	{
		ALog(@"GME: No new emu!");
		return NO;
	}
	
	[source seek:0 whence:SEEK_END];
	long size = [source tell];
	[source seek:0 whence:SEEK_SET];
	
	DLog(@"Size: %li", size);
	
	error = gme_load_custom(emu, readCallback, size, s);
	if (error) 
	{
		ALog(@"GME: ERROR Loding custom!");
		return NO;
	}
	
	int track_num = [[[source url] fragment] intValue]; //What if theres no fragment? Assuming we get 0.
	
	gme_info_t * info;
	error = gme_track_info( emu, &info, track_num );
	if (error)
	{
		ALog(@"Unable to get track info");
        return NO;
	}
	
	//As recommended
	if (info->length > 0) {
		DLog(@"Using length: %i", info->length);
		length = info->length;
	}
	else if (info->loop_length > 0) {
		DLog(@"Using loop length: %i", info->loop_length);
		length = info->intro_length + 2*info->loop_length;
	}
	else {
		length = 150000; 
		DLog(@"Setting default: %li", length);
	}
    
    gme_free_info( info );

	DLog(@"Length: %li", length);
	
	DLog(@"Track num: %i", track_num);
	error = gme_start_track(emu, track_num);
	if (error) 
	{
		ALog(@"GME: Error starting track");
		return NO;
	}
    
    gme_set_fade( emu, length, 8000 );
    
    length += 8000;
    

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];
	
	return YES;
}

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:0], @"bitrate",
		[NSNumber numberWithFloat:44100], @"sampleRate",
		[NSNumber numberWithLong:length*44.1], @"totalFrames",
		[NSNumber numberWithInt:sizeof(short)*8], @"bitsPerSample", //Samples are short
		[NSNumber numberWithInt:2], @"channels", //output from gme_play is in stereo
		[NSNumber numberWithBool:[source seekable]], @"seekable",
		@"host", @"endian",
		nil];
}

- (int)readAudio:(void *)buf frames:(UInt32)frames
{
	int numSamples = frames * 2; //channels = 2
	
	if (gme_track_ended(emu)) {
		return 0;
	}
	
	gme_play(emu, numSamples, (short int *)buf);
	
	//Some formats support length, but we'll add that in the future.
	//(From gme.txt) If track length, then use it. If loop length, play for intro + loop * 2. Otherwise, default to 2.5 minutes
	return frames; //GME will always generate samples. There's no real EOS.
}

- (long)seek:(long)frame
{
	gme_err_t error;
	error = gme_seek(emu, frame/44.1);
	if (error) {
		return -1;
	}
	
	return frame;
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
	return [NSArray arrayWithObjects:@"ay", @"gbs", @"hes", @"kss", @"nsf", @"nsfe", @"sap", @"sgc", @"spc", @"vgm", @"vgz", nil];
}

+ (NSArray *)mimeTypes 
{	
	return nil;
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
