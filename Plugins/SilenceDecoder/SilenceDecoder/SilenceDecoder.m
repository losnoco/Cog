//
//  SilenceDecoder.m
//  Cog
//
//  Created by Christopher Snowhill on 2/8/15.
//  Copyright 2015 __NoWork, LLC__. All rights reserved.
//

#import "SilenceDecoder.h"

#import "Logging.h"

#import "PlaylistController.h"

@implementation SilenceDecoder

enum { sample_rate = 44100 };
enum { channels = 2 };

- (BOOL)open:(id<CogSource>)s
{
    [self setSource:s];
    
    NSString * path = [[[s url] relativeString] substringFromIndex:10];
    
    length = [path intValue] * sample_rate;
    remain = length;
    
    [self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:0], @"bitrate",
		[NSNumber numberWithFloat:sample_rate], @"sampleRate",
		[NSNumber numberWithDouble:length], @"totalFrames",
		[NSNumber numberWithInt:32], @"bitsPerSample",
        [NSNumber numberWithBool:YES], @"floatingPoint",
		[NSNumber numberWithInt:channels], @"channels",
		[NSNumber numberWithBool:YES], @"seekable",
		@"host", @"endian",
		nil];
}

- (int)readAudio:(void *)buf frames:(UInt32)frames
{
    int total = frames;
    
    if (!IsRepeatOneSet())
    {
        if (total > remain)
            total = (int)remain;
    
        remain -= total;
    }
    
    memset(buf, 0, sizeof(float) * total * channels);
    
    return total;
}

- (long)seek:(long)frame
{
    if (frame > length)
        frame = length;
    
    remain = length - frame;
   
    return frame;
}

- (void)close
{
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
	return [NSArray array];
}

+ (NSArray *)mimeTypes 
{	
	return [NSArray arrayWithObject:@"audio/x-silence"];
}

+ (float)priority
{
    return 1.0;
}

@end
