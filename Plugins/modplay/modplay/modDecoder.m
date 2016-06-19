//
//  modDecoder.m
//  modplay
//
//  Created by Christopher Snowhill on 03/17/14.
//  Copyright 2014 __NoWork, Inc__. All rights reserved.
//

#import "modDecoder.h"

#import "PlaylistController.h"

@implementation modDecoder

BOOL s3m_probe_length( unsigned long * intro_length, unsigned long * loop_length, const void * src, unsigned long size, unsigned int subsong )
{
    void * st3play = st3play_Alloc( 44100, 0, 0 );
    if ( !st3play ) return NO;
    
    if ( !st3play_LoadModule( st3play, src, size ) )
    {
        st3play_Free( st3play );
        return NO;
    }
    
    st3play_PlaySong( st3play, subsong );
    
    unsigned long length_total = 0;
    unsigned long length_saved;
    
    const long length_safety = 44100 * 60 * 30;
    
    while ( st3play_GetLoopCount( st3play ) < 1 && length_total < length_safety )
    {
        st3play_RenderFloat( st3play, NULL, 512 );
        length_total += 512;
    }
    
    if ( st3play_GetLoopCount( st3play ) < 1 )
    {
        *loop_length = 0;
        *intro_length = 44100 * 60 * 3;
        st3play_Free( st3play );
        return YES;
    }
    
    length_saved = length_total;
    
    while ( st3play_GetLoopCount( st3play ) < 2 )
    {
        st3play_RenderFloat( st3play, NULL, 512 );
        length_total += 512;
    }
    
    st3play_Free( st3play );
    
    *loop_length = length_total - length_saved;

    if (length_saved > *loop_length)
        *intro_length = length_saved - *loop_length;
    else
        *intro_length = 0;
    
    return YES;
}

BOOL xm_probe_length( unsigned long * intro_length, unsigned long * loop_length, const void * src, unsigned long size, unsigned int subsong )
{
    void * ft2play = ft2play_Alloc( 44100, 0, 0 );
    if ( !ft2play ) return NO;
    
    if ( !ft2play_LoadModule( ft2play, src, size ) )
    {
        ft2play_Free( ft2play );
        return NO;
    }
    
    ft2play_PlaySong( ft2play, subsong );
    
    unsigned long length_total = 0;
    unsigned long length_saved;
    
    const long length_safety = 44100 * 60 * 30;
    
    while ( ft2play_GetLoopCount( ft2play ) < 1 && length_total < length_safety )
    {
        ft2play_RenderFloat( ft2play, NULL, 512 );
        length_total += 512;
    }
    
    if ( ft2play_GetLoopCount( ft2play ) < 1 )
    {
        *loop_length = 0;
        *intro_length = 44100 * 60 * 3;
        ft2play_Free( ft2play );
        return YES;
    }
    
    length_saved = length_total;
    
    while ( ft2play_GetLoopCount( ft2play ) < 2 )
    {
        ft2play_RenderFloat( ft2play, NULL, 512 );
        length_total += 512;
    }
    
    ft2play_Free( ft2play );
    
    *loop_length = length_total - length_saved;

    if (length_saved > *loop_length)
        *intro_length = length_saved - *loop_length;
    else
        *intro_length = 0;
        
    return YES;
}

- (id)init
{
    self = [super init];
    if (self) {
        player = NULL;
        data = NULL;
    }
    return self;
}

- (BOOL)open:(id<CogSource>)s
{
    [s seek:0 whence:SEEK_END];
    size = [s tell];
    [s seek:0 whence:SEEK_SET];

    data = malloc(size);
    [s read:data amount:size];
    
    type = TYPE_UNKNOWN;
    
    dataWasMo3 = 0;
    if ( size >= 3 && memcmp( data, "MO3", 3 ) == 0 )
    {
        void * in_data = data;
        unsigned usize = (unsigned) size;
        if (UNMO3_Decode(&in_data, &usize, 0) == 0)
        {
            free( data );
            data = in_data;
            size = usize;
            dataWasMo3 = 1;
        }
    }
    else if ( size >= 4 && memcmp( data, "\xC1\x83\x2A\x9E", 4 ) == 0 )
    {
        long out_size = size;
        void * out_data = unpackUmx( data, &out_size );
        if ( out_data )
        {
            free( data );
            data = out_data;
            size = out_size;
        }
    }
    
    if ( size >= (0x2C + 4) && memcmp( data + 0x2C, "SCRM", 4 ) == 0 )
        type = TYPE_S3M;
    else if ( size >= 17 && memcmp( data, "Extended Module: ", 17 ) == 0 )
        type = TYPE_XM;
    else
        return NO;
    
	if ([[[s url] fragment] length] == 0)
		track_num = 0;
	else
		track_num = [[[s url] fragment] intValue];

    unsigned long intro_length = 0, loop_length = 0;
    
    if ( type == TYPE_S3M && !s3m_probe_length(&intro_length, &loop_length, data, size, track_num) )
        return NO;
    else if ( type == TYPE_XM && !xm_probe_length(&intro_length, &loop_length, data, size, track_num) )
        return NO;

    framesLength = intro_length + loop_length * 2;
    totalFrames = framesLength + 44100 * 8;

    [self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];
    
	return YES;
}

- (BOOL)decoderInitialize
{
    int resampling_int = -1;
    NSString * resampling = [[NSUserDefaults standardUserDefaults] stringForKey:@"resampling"];
    if ([resampling isEqualToString:@"zoh"])
        resampling_int = 0;
    else if ([resampling isEqualToString:@"blep"])
        resampling_int = 1;
    else if ([resampling isEqualToString:@"linear"])
        resampling_int = 2;
    else if ([resampling isEqualToString:@"blam"])
        resampling_int = 3;
    else if ([resampling isEqualToString:@"cubic"])
        resampling_int = 4;
    else if ([resampling isEqualToString:@"sinc"])
        resampling_int = 5;
    
    if ( type == TYPE_S3M )
    {
        player = st3play_Alloc( 44100, resampling_int, 2 );
        if ( !player )
            return NO;

        if ( !st3play_LoadModule( player, data, size ) )
            return NO;

        st3play_PlaySong( player, track_num );
    }
    else if ( type == TYPE_XM )
    {
        player = ft2play_Alloc( 44100, resampling_int, 2 );
        if ( !player )
            return NO;
        
        if ( !ft2play_LoadModule( player, data, size ) )
            return NO;
        
        ft2play_PlaySong( player, track_num );
    }

    framesRead = 0;
    
    return YES;
}

- (void)decoderShutdown
{
    if ( player )
    {
        if ( type == TYPE_S3M )
            st3play_Free( player );
        else if ( type == TYPE_XM )
            ft2play_Free( player );
        player = NULL;
    }
}

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:0], @"bitrate",
		[NSNumber numberWithFloat:44100], @"sampleRate",
		[NSNumber numberWithDouble:totalFrames], @"totalFrames",
		[NSNumber numberWithInt:32], @"bitsPerSample",
        [NSNumber numberWithBool:YES], @"floatingPoint",
		[NSNumber numberWithInt:2], @"channels",
		[NSNumber numberWithBool:YES], @"seekable",
		@"host", @"endian",
		nil];
}

- (int)readAudio:(void *)buf frames:(UInt32)frames
{
    BOOL repeat_one = IsRepeatOneSet();
    
    if ( !repeat_one && framesRead >= totalFrames )
        return 0;
    
    if ( !player )
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

        float * sampleBuf = ( float * ) buf + total * 2;
        
        if ( type == TYPE_S3M )
            st3play_RenderFloat( player, sampleBuf, framesToRender );
        else if ( type == TYPE_XM )
            ft2play_RenderFloat( player, sampleBuf, framesToRender );
    
        if ( !repeat_one && framesRead + framesToRender > framesLength ) {
            long fadeStart = ( framesLength > framesRead ) ? framesLength : framesRead;
            long fadeEnd = ( framesRead + framesToRender < totalFrames ) ? framesRead + framesToRender : totalFrames;
            const long fadeTotal = totalFrames - framesLength;
            for ( long fadePos = fadeStart; fadePos < fadeEnd; ++fadePos ) {
                const double scale = (double)( fadeTotal - ( fadePos - framesLength ) ) / (double)fadeTotal;
                const long offset = fadePos - framesRead;
                float * samples = sampleBuf + offset * 2;
                samples[ 0 ] = samples[ 0 ] * scale;
                samples[ 1 ] = samples[ 1 ] * scale;
            }
            
            framesToRender = (int)(fadeEnd - framesRead);
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
    if ( frame < framesRead || !player )
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
        if ( type == TYPE_S3M )
            st3play_RenderFloat( player, NULL, frames_todo );
        else if ( type == TYPE_XM )
            ft2play_RenderFloat( player, NULL, frames_todo );
        framesRead += frames_todo;
    }
    
    framesRead = frame;
    
    return frame;
}

- (void)close
{
    [self decoderShutdown];
	
    if (data) {
        if ( dataWasMo3 )
            UNMO3_Free( data );
        else
            free( data );
        data = NULL;
    }
}

- (void)dealloc
{
    [self close];
}

+ (NSArray *)fileTypes
{	
	return [NSArray arrayWithObjects:@"s3m", @"s3z", @"xm", @"xmz", @"mo3", @"umx", nil];
}

+ (NSArray *)mimeTypes 
{	
	return [NSArray arrayWithObjects:@"audio/x-s3m", @"audio/x-xm", nil];
}

+ (float)priority
{
    return 1.5;
}

@end
