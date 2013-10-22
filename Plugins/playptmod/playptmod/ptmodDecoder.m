//
//  ptmodDecoder.m
//  playptmod
//
//  Created by Christopher Snowhill on 10/22/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "ptmodDecoder.h"

#import "Logging.h"

@implementation ptmodDecoder

BOOL probe_length( unsigned long * intro_length, unsigned long * loop_length, int test_vblank, const void * src, unsigned long size, unsigned int subsong )
{
    void * ptmod = playptmod_Create( 44100 );
    if ( !ptmod ) return NO;
    
    playptmod_Config( ptmod, PTMOD_OPTION_CLAMP_PERIODS, 0 );
    playptmod_Config( ptmod, PTMOD_OPTION_VSYNC_TIMING, test_vblank );
    
    if ( !playptmod_LoadMem( ptmod, src, size ) )
    {
        playptmod_Free( ptmod );
        return NO;
    }
    
    playptmod_Play( ptmod, subsong );
    
    unsigned long length_total = 0;
    unsigned long length_saved;
    
    while ( playptmod_LoopCounter( ptmod ) < 1 )
    {
        playptmod_Render( ptmod, NULL, 512 );
        length_total += 512;
    }
    
    length_saved = length_total;
    
    while ( playptmod_LoopCounter( ptmod ) < 2 )
    {
        playptmod_Render( ptmod, NULL, 512 );
        length_total += 512;
    }
    
    playptmod_Stop( ptmod );
    playptmod_Free( ptmod );
    
    *loop_length = length_total - length_saved;
    *intro_length = length_saved - *loop_length;
    
    return YES;
}

- (BOOL)open:(id<CogSource>)s
{
    [s seek:0 whence:SEEK_END];
    size = [s tell];
    [s seek:0 whence:SEEK_SET];

    data = malloc(size);
    [s read:data amount:size];

	if ([[[s url] fragment] length] == 0)
		track_num = 0;
	else
		track_num = [[[s url] fragment] intValue];

    unsigned long normal_intro_length, normal_loop_length, vblank_intro_length, vblank_loop_length;
    
    if ( !probe_length(&normal_intro_length, &normal_loop_length, 0, data, size, track_num) )
        return NO;
    if ( !probe_length(&vblank_intro_length, &vblank_loop_length, 1, data, size, track_num) )
        return NO;

    isVblank = ( vblank_intro_length + vblank_loop_length ) < ( normal_intro_length + normal_loop_length );

    unsigned long intro_length = isVblank ? vblank_intro_length : normal_intro_length;
    unsigned long loop_length = isVblank ? vblank_loop_length : normal_loop_length;

    framesLength = intro_length + loop_length * 2;
    totalFrames = framesLength + 44100 * 8;

    [self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];
    
	return YES;
}

- (BOOL)decoderInitialize
{
    ptmod = playptmod_Create( 44100 );
    if ( !ptmod )
        return NO;

    playptmod_Config( ptmod, PTMOD_OPTION_CLAMP_PERIODS, 0 );
    playptmod_Config( ptmod, PTMOD_OPTION_VSYNC_TIMING, isVblank );

    if ( !playptmod_LoadMem( ptmod, data, size ) )
        return NO;

    playptmod_Play( ptmod, track_num );

    framesRead = 0;
    
    return YES;
}

- (void)decoderShutdown
{
    if ( ptmod )
    {
        playptmod_Stop( ptmod );
        playptmod_Free( ptmod );
        ptmod = NULL;
    }
}

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:0], @"bitrate",
		[NSNumber numberWithFloat:44100], @"sampleRate",
		[NSNumber numberWithDouble:totalFrames], @"totalFrames",
		[NSNumber numberWithInt:16], @"bitsPerSample", //Samples are short
        [NSNumber numberWithBool:NO], @"floatingPoint",
		[NSNumber numberWithInt:2], @"channels", //output from gme_play is in stereo
		[NSNumber numberWithBool:YES], @"seekable",
		@"host", @"endian",
		nil];
}

- (int)readAudio:(void *)buf frames:(UInt32)frames
{
    if ( framesRead >= totalFrames )
        return 0;
    
    if ( !ptmod )
    {
        if ( ![self decoderInitialize] )
            return 0;
    }
    
    int total = 0;
    while ( total < frames ) {
        int framesToRender = 512;
        if ( framesToRender > totalFrames - framesRead )
            framesToRender = totalFrames - framesRead;
        if ( framesToRender > frames - total )
            framesToRender = frames - total;

        int16_t * sampleBuf = ( int16_t * ) buf + total * 2;
        
        playptmod_Render( ptmod, sampleBuf, framesToRender );
    
        if ( framesRead + framesToRender > framesLength ) {
            long fadeStart = ( framesLength > framesRead ) ? framesLength : framesRead;
            long fadeEnd = ( framesRead + framesToRender < totalFrames ) ? framesRead + framesToRender : totalFrames;
            const long fadeTotal = totalFrames - framesLength;
            for ( long fadePos = fadeStart; fadePos < fadeEnd; ++fadePos ) {
                const long scale = ( fadeTotal - ( fadePos - framesLength ) );
                const long offset = fadePos - framesRead;
                int16_t * samples = sampleBuf + offset * 2;
                samples[ 0 ] = samples[ 0 ] * scale / fadeTotal;
                samples[ 1 ] = samples[ 1 ] * scale / fadeTotal;
            }
            
            framesToRender = fadeEnd - framesRead;
        }

        if ( !framesToRender )
            break;
        
        total += framesToRender;
        framesRead += framesToRender;
    }
    
    return total;
}

- (long)seek:(long)frame
{
    if ( frame < framesRead || !ptmod )
    {
        [self decoderShutdown];
        if ( ![self decoderInitialize] )
            return 0;
    }

    playptmod_Render( ptmod, NULL, frame - framesRead );
    
    framesRead = frame;
    
    return frame;
}

- (void)close
{
    [self decoderShutdown];
	
    if (data) {
        free( data );
        data = NULL;
    }
}

+ (NSArray *)fileTypes
{	
	return [NSArray arrayWithObjects:@"mod", @"mdz", @"stk", @"m15", @"fst", nil];
}

+ (NSArray *)mimeTypes 
{	
	return [NSArray arrayWithObjects:@"audio/x-mod", nil];
}

+ (float)priority
{
    return 1.5;
}

@end
