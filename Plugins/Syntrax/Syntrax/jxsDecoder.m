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

BOOL probe_length( Song * synSong, unsigned long * intro_length, unsigned long * loop_length, unsigned int subsong )
{
    Player * synPlayer = playerCreate(44100);
    
    if (loadSong(synPlayer, synSong) < 0)
        return NO;
    
    initSubsong(synPlayer, subsong);
    
    unsigned long length_total = 0;
    unsigned long length_saved;
    
    const long length_safety = 44100 * 60 * 30;
    
    while ( !playerGetSongEnded(synPlayer) && playerGetLoopCount(synPlayer) < 1 && length_total < length_safety )
    {
        mixChunk(synPlayer, NULL, 512);
        length_total += 512;
    }
    
    if ( !playerGetSongEnded(synPlayer) && playerGetLoopCount(synPlayer) < 1 )
    {
        *loop_length = 0;
        *intro_length = 44100 * 60 * 3;
        playerDestroy(synPlayer);
        return YES;
    }
    
    length_saved = length_total;
    
    while ( !playerGetSongEnded(synPlayer) && playerGetLoopCount(synPlayer) < 2 )
    {
        mixChunk(synPlayer, NULL, 512);
        length_total += 512;
    }
    
    playerDestroy(synPlayer);
    
    *loop_length = length_total - length_saved;
    *intro_length = length_saved - *loop_length;
    
    return YES;
}

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
    
    synSong = File_loadSongMem(data, size);
    if (!synSong)
        return NO;
    
    free(data);
    
    unsigned long intro_length, loop_length;
    
    if ( !probe_length(synSong, &intro_length, &loop_length, track_num) )
        return NO;

    framesLength = intro_length + loop_length * 2;
    totalFrames = framesLength + 44100 * 8;

    [self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];
    
	return YES;
}

- (BOOL)decoderInitialize
{
    synPlayer = playerCreate(44100);
    if (!synPlayer)
        return NO;
    
    if (loadSong(synPlayer, synSong) < 0)
        return NO;
    
    initSubsong(synPlayer, track_num);

    framesRead = 0;
    
    return YES;
}

- (void)decoderShutdown
{
    if ( synPlayer )
    {
        playerDestroy(synPlayer);
        synPlayer = NULL;
    }
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
    
    if ( synPlayer && playerGetSongEnded(synPlayer) )
        return 0;
    
    if ( !repeat_one && framesRead >= totalFrames )
        return 0;
    
    if ( !synPlayer )
    {
        if ( ![self decoderInitialize] )
            return 0;
    }
    
    int total = 0;
    while ( total < frames ) {
        int framesToRender = 512;
        if ( !repeat_one && framesToRender > totalFrames - framesRead )
            framesToRender = (int)(totalFrames - framesRead);
        if ( framesToRender > frames - total )
            framesToRender = frames - total;

        int16_t * sampleBuf = ( int16_t * ) buf + total * 2;
        
        mixChunk(synPlayer, sampleBuf, framesToRender);
    
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
        
        if ( playerGetSongEnded(synPlayer) )
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
        mixChunk(synPlayer, NULL, frames_todo);
        framesRead += frames_todo;
    }
    
    framesRead = frame;
    
    return frame;
}

- (void)close
{
    [self decoderShutdown];
	
    if (synSong)
    {
        File_freeSong(synSong);
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
