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

- (BOOL)open:(id<CogSource>)s
{
	[self setSource:s];
	
	dfs.open = NULL;
	dfs.skip = skipCallback;
	dfs.getc = getCharCallback;
	dfs.getnc = readCallback;
	dfs.close = closeCallback;

	dumb_register_stdfiles();

	df = dumbfile_open_ex(self, &dfs);
	if (!df)
	{
		NSLog(@"EX Failed");
		return NO;
	}

	NSString *ext = [[[[s url] path] pathExtension] lowercaseString];
	if ([ext isEqualToString:@"it"])
		duh = dumb_read_it(df);
	else if ([ext isEqualToString:@"xm"]) 
		duh = dumb_read_xm(df);
	else if ([ext isEqualToString:@"s3m"])
		duh = dumb_read_s3m(df);
	else if ([ext isEqualToString:@"mod"])
		duh = dumb_read_mod(df);
	else {
		NSLog(@"DUH IS NUL!!!");
		duh = NULL;
	}
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
	
	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:0], @"bitrate",
		[NSNumber numberWithFloat:44100], @"sampleRate",
		[NSNumber numberWithDouble:length / 65.536f], @"length",
		[NSNumber numberWithInt:16], @"bitsPerSample", //Samples are short
		[NSNumber numberWithInt:2], @"channels", //output from gme_play is in stereo
		[NSNumber numberWithBool:[source seekable]], @"seekable",
		@"host", @"endian",
		nil];
}

- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
	if (duh_sigrenderer_get_position(dsr) > length) {
		return 0;
	}
	
	return 4*duh_render(dsr, 16 /* shorts */, 0 /* not unsigned */, 1.0 /* volume */, 65536.0f / 44100.0f /* 65536 hz? */, size/(2*2), buf);
}

- (double)seekToTime:(double)milliseconds
{
	double pos = (double)duh_sigrenderer_get_position(dsr) / 65.536f;
	double seekPos = milliseconds;
	
	if (seekPos < pos) {
		//Reset. Dumb cannot seek backwards. It's dumb.
		[self cleanUp];
		
		[source seek:0 whence:SEEK_SET];
		[self open:source];
		
		pos = 0.0;
	}
	
	int numSamples = (seekPos - pos)/1000 * 44100;
	
	duh_sigrenderer_generate_samples(dsr, 1.0f, 65536.0f / 44100.0f, numSamples, NULL);
   
   return milliseconds;
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

+ (NSArray *)fileTypes 
{	
	return [NSArray arrayWithObjects:@"it", @"xm", @"s3m", @"mod",nil];
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
