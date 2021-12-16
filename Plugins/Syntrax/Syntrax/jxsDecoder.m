//
//  jxsDecoder.m
//  Syntrax-c
//
//  Created by Christopher Snowhill on 03/14/16.
//  Copyright 2016 __NoWork, Inc__. All rights reserved.
//

#import "jxsDecoder.h"

#import "Logging.h"

#import "PlaylistController.h"

@implementation jxsDecoder

- (id)init
{
    self = [super init];
    if (self) {
        synSong = NULL;
        synPlayer = NULL;
    }
    return self;
}

- (BOOL)open:(id<CogSource>)s
{
    [s seek:0 whence:SEEK_END];
    size_t size = [s tell];
    [s seek:0 whence:SEEK_SET];

    void * data = malloc(size);
    [s read:data amount:size];

	if ([[[s url] fragment] length] == 0)
		track_num = 0;
	else
		track_num = [[[s url] fragment] intValue];

    if (jxsfile_readSongMem(data, size, &synSong))
        return NO;
    
    free(data);

    synPlayer = jaytrax_init();
    if (!synPlayer)
        return NO;

    if (!jaytrax_loadSong(synPlayer, synSong))
        return NO;
    
    framesLength = jaytrax_getLength(synPlayer, track_num, 2, 44100);
    totalFrames = framesLength + ((synPlayer->playFlg) ? 44100 * 8 : 0);
    
    framesRead = 0;

    [self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];
    
	return YES;
}

- (BOOL)decoderInitialize
{
    jaytrax_changeSubsong(synPlayer, track_num);

    uint8_t interp = ITP_CUBIC;
    NSString * resampling = [[NSUserDefaults standardUserDefaults] stringForKey:@"resampling"];
    if ([resampling isEqualToString:@"zoh"])
        interp = ITP_NEAREST;
    else if ([resampling isEqualToString:@"blep"])
        interp = ITP_NEAREST;
    else if ([resampling isEqualToString:@"linear"])
        interp = ITP_LINEAR;
    else if ([resampling isEqualToString:@"blam"])
        interp = ITP_LINEAR;
    else if ([resampling isEqualToString:@"cubic"])
        interp = ITP_CUBIC;
    else if ([resampling isEqualToString:@"sinc"])
        interp = ITP_CUBIC;
    
    jaytrax_setInterpolation(synPlayer, interp);

    framesRead = 0;
    
    return YES;
}

- (void)decoderShutdown
{
    if (synPlayer)
    {
        jaytrax_stopSong(synPlayer);
    }
    
    framesRead = 0;
}

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:0], @"bitrate",
		[NSNumber numberWithFloat:44100], @"sampleRate",
		[NSNumber numberWithDouble:totalFrames], @"totalFrames",
		[NSNumber numberWithInt:16], @"bitsPerSample",
        [NSNumber numberWithBool:NO], @"floatingPoint",
		[NSNumber numberWithInt:2], @"channels",
		[NSNumber numberWithBool:YES], @"seekable",
        @"Syntrax", @"codec",
		@"host", @"endian",
		nil];
}

- (int)readAudio:(void *)buf frames:(UInt32)frames
{
    BOOL repeat_one = IsRepeatOneSet();
    
    if ( !repeat_one && framesRead >= totalFrames )
        return 0;
    
    if ( !framesRead )
    {
        if ( ![self decoderInitialize] )
            return 0;
    }
    
    if ( synPlayer && !synPlayer->playFlg )
        return 0;
    
    int total = 0;
    while ( total < frames ) {
        int framesToRender = 512;
        if ( !repeat_one && framesToRender > totalFrames - framesRead )
            framesToRender = (int)(totalFrames - framesRead);
        if ( framesToRender > frames - total )
            framesToRender = frames - total;

        int16_t * sampleBuf = ( int16_t * ) buf + total * 2;
        
        jaytrax_renderChunk(synPlayer, sampleBuf, framesToRender, 44100);
    
        if ( !repeat_one && framesRead + framesToRender > framesLength ) {
            long fadeStart = ( framesLength > framesRead ) ? framesLength : framesRead;
            long fadeEnd = ( framesRead + framesToRender < totalFrames ) ? framesRead + framesToRender : totalFrames;
            const long fadeTotal = totalFrames - framesLength;
            for ( long fadePos = fadeStart; fadePos < fadeEnd; ++fadePos ) {
                const long scale = ( fadeTotal - ( fadePos - framesLength ) );
                const long offset = fadePos - framesRead;
                int16_t * samples = sampleBuf + offset * 2;
                samples[ 0 ] = (int16_t)(samples[ 0 ] * scale / fadeTotal);
                samples[ 1 ] = (int16_t)(samples[ 1 ] * scale / fadeTotal);
            }
            
            framesToRender = (int)(fadeEnd - framesRead);
        }

        if ( !framesToRender )
            break;
        
        total += framesToRender;
        framesRead += framesToRender;

        if ( !synPlayer->playFlg )
            break;
    }
    
    return total;
}

- (long)seek:(long)frame
{
    if ( frame < framesRead || !synPlayer )
    {
        [self decoderShutdown];
        if ( ![self decoderInitialize] )
            return 0;
    }

    while ( framesRead < frame )
    {
        int frames_todo = INT_MAX;
        if ( frames_todo > frame - framesRead )
            frames_todo = (int)( frame - framesRead );
        jaytrax_renderChunk(synPlayer, NULL, frames_todo, 44100);
        framesRead += frames_todo;
    }
    
    framesRead = frame;
    
    return frame;
}

- (void)close
{
    [self decoderShutdown];
    
    if (synPlayer)
    {
        jaytrax_free(synPlayer);
        synPlayer = NULL;
    }
	
    if (synSong)
    {
        jxsfile_freeSong(synSong);
        synSong = NULL;
    }
}

- (void)dealloc
{
    [self close];
}

+ (NSArray *)fileTypes
{	
	return [NSArray arrayWithObjects:@"jxs", nil];
}

+ (NSArray *)mimeTypes 
{	
	return [NSArray arrayWithObjects:@"audio/x-jxs", nil];
}

+ (float)priority
{
    return 1.0;
}

@end
