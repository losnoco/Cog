//
//  DumbFile.m
//  Cog
//
//  Created by Vincent Spader on 5/29/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "DumbDecoder.h"

@implementation DumbDecoder

int skipCallback(void *f, long n)
{
	DumbDecoder *decoder = (DumbDecoder *)f;
	
	if (![[decoder source] seek:n whence: SEEK_CUR]) 
	{
		return 1; //Non-zero is error
	}
	
	return 0; //Zero for success
}

int getCharCallback(void *f)
{
	DumbDecoder *decoder = (DumbDecoder *)f;

	unsigned char c;
	
	if ([[decoder source] read:&c amount:1] < 1)
	{
		return -1;
	}
	
	return c;
}

long readCallback(char *ptr, long n, void *f)
{
	DumbDecoder *decoder = (DumbDecoder *)f;
	
	return [[decoder source] read:ptr amount:n];
}

void closeCallback(void *f)
{
//	DumbDecoder *decoder = (DumbDecoder *)f;
	NSLog(@"CLOSE"); //I DO NOTHING
}

int seekCallback(void *f, long n)
{
    DumbDecoder *decoder = (DumbDecoder *)f;
    
    if (![[decoder source] seekable]) return -1;
    
    if ([[decoder source] seek:n whence:SEEK_SET]) return 0;
    else return -1;
}

long getsizeCallback(void *f)
{
    DumbDecoder *decoder = (DumbDecoder *)f;
    
    if (![[decoder source] seekable]) return 0;
    
    long current_offset = [[decoder source] tell];
    
    [[decoder source] seek:0 whence:SEEK_END];
    
    long size = [[decoder source] tell];
    
    [[decoder source] seek:current_offset whence:SEEK_SET];
    
    return size;
}

- (BOOL)open:(id<CogSource>)s
{
	[self setSource:s];
	
	dfs.open = NULL;
	dfs.skip = skipCallback;
	dfs.getc = getCharCallback;
	dfs.getnc = readCallback;
	dfs.close = closeCallback;
    dfs.seek = seekCallback;
    dfs.get_size = getsizeCallback;

//	dumb_register_stdfiles();

	df = dumbfile_open_ex(self, &dfs);
	if (!df)
	{
		NSLog(@"EX Failed");
		return NO;
	}

	NSString *ext = [[[[s url] path] pathExtension] lowercaseString];
    duh = dumb_read_any(df, [ext isEqualToString:@"mod"] ? 0 : 1, 0);
	if (!duh)
	{
		NSLog(@"Failed to create duh");
		return NO;
	}
	
	length = duh_get_length(duh);

	
	dsr = duh_start_sigrenderer(duh, 0, 2 /* stereo */, 0 /* start from the beginning */);
	if (!dsr) 
	{
		NSLog(@"Failed to create dsr");
		return NO;
	}
	
    DUMB_IT_SIGRENDERER * itsr = duh_get_it_sigrenderer( dsr );
    dumb_it_set_ramp_style( itsr, 2 );
    
    [self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:0], @"bitrate",
		[NSNumber numberWithFloat:44100], @"sampleRate",
		[NSNumber numberWithDouble:((length / 65.536f)*44.1000)], @"totalFrames",
		[NSNumber numberWithInt:16], @"bitsPerSample", //Samples are short
		[NSNumber numberWithInt:2], @"channels", //output from gme_play is in stereo
		[NSNumber numberWithBool:[source seekable]], @"seekable",
		@"host", @"endian",
		nil];
}

- (int)readAudio:(void *)buf frames:(UInt32)frames
{
	if (duh_sigrenderer_get_position(dsr) > length) {
		return 0;
	}
	
	return duh_render(dsr, 16 /* shorts */, 0 /* not unsigned */, 1.0 /* volume */, 65536.0f / 44100.0f /* 65536 hz? */, frames, buf);
}

- (long)seek:(long)frame
{
	double pos = (double)duh_sigrenderer_get_position(dsr) / 65.536f;
	double seekPos = frame/44.100;
	
	if (seekPos < pos) {
		//Reset. Dumb cannot seek backwards. It's dumb.
		[self cleanUp];
		
		[source seek:0 whence:SEEK_SET];
		[self open:source];
		
		pos = 0.0;
	}
	
	int numSamples = (seekPos - pos)/1000 * 44100;
	
	duh_sigrenderer_generate_samples(dsr, 1.0f, 65536.0f / 44100.0f, numSamples, NULL);
   
   return frame;
}

- (void)cleanUp
{
	if (dsr) {
		duh_end_sigrenderer(dsr);
		dsr = NULL;
	}
	if (duh) {
		unload_duh(duh);
		duh = NULL;
	}
	if (df) {
		dumbfile_close(df);
		df = NULL;
	}
}

- (void)close
{
	[self cleanUp];
	
	if (source) {
		[source close];
		[self setSource:nil];
	}
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

+ (NSArray *)fileTypes 
{	
	return [NSArray arrayWithObjects:@"it", @"xm", @"s3m", @"mod", @"stm", @"ptm", @"mtm", @"669", @"psm", @"am", @"dsm", @"amf", @"okt", @"okta", nil];
}

+ (NSArray *)mimeTypes 
{	
	return [NSArray arrayWithObjects:@"audio/x-it", @"audio/x-xm", @"audio/x-s3m", @"audio/x-mod", nil];
}

@end
