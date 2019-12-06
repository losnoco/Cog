//
//  SidDecoder.mm
//  sidplay
//
//  Created by Christopher Snowhill on 12/8/14.
//  Copyright 2014 __NoWork, Inc__. All rights reserved.
//

#import "SidDecoder.h"

#import <sidplayfp/residfp.h>

#import "roms.hpp"

#import "Logging.h"

#import "PlaylistController.h"

@implementation SidDecoder

- (BOOL)open:(id<CogSource>)s
{
	[self setSource:s];
	
    [source seek:0 whence:SEEK_END];
    long size = [source tell];
    [source seek:0 whence:SEEK_SET];
    
    void * data = malloc(size);
    [source read:data amount:size];
	
    tune = new SidTune((const uint_least8_t *)data, (uint_least32_t)size);
    
    if (!tune->getStatus())
        return NO;
    
    NSURL * url = [s url];
	int track_num;
	if ([[url fragment] length] == 0)
		track_num = 1;
	else
		track_num = [[url fragment] intValue];

    const SidTuneInfo * info = tune->getInfo();
    
    n_channels = info->sidChips();
    
    length = 3 * 60 * 44100;

    tune->selectSong( track_num );
    
    engine = new sidplayfp;
    
    engine->setRoms( kernel, basic, chargen );
    
    if ( !engine->load( tune ) )
        return NO;
    
    ReSIDfpBuilder * _builder = new ReSIDfpBuilder("ReSIDfp");
    builder = _builder;
    
    if (_builder)
    {
        _builder->create((engine->info()).maxsids());
        if (_builder->getStatus())
        {
            _builder->filter(true);
            _builder->filter6581Curve(0.5);
            _builder->filter8580Curve(0.5);
        }
        if (!_builder->getStatus())
            return NO;
    }
    else return NO;
    
    SidConfig conf = engine->config();
    conf.frequency = 44100;
    conf.playback = (info->sidChips() > 1) ? SidConfig::STEREO : SidConfig::MONO;
    conf.sidEmulation = builder;
    if (!engine->config(conf))
        return NO;

    renderedTotal = 0;
    fadeTotal = fadeRemain = 44100 * 8;
    
    [self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:0], @"bitrate",
		[NSNumber numberWithFloat:44100], @"sampleRate",
		[NSNumber numberWithDouble:length], @"totalFrames",
		[NSNumber numberWithInt:16], @"bitsPerSample", //Samples are short
        [NSNumber numberWithBool:NO], @"floatingPoint",
		[NSNumber numberWithInt:n_channels], @"channels", //output from gme_play is in stereo
		[NSNumber numberWithBool:[source seekable]], @"seekable",
		@"host", @"endian",
		nil];
}

- (int)readAudio:(void *)buf frames:(UInt32)frames
{
    int total = 0;
    int16_t * sampleBuffer = (int16_t*)buf;
    while ( total < frames ) {
        int framesToRender = 1024;
        if ( framesToRender > frames )
            framesToRender = frames;
        int rendered = engine->play( sampleBuffer + total * n_channels, framesToRender * n_channels ) / n_channels;
        
        if (rendered <= 0)
            break;
        
        renderedTotal += rendered;
        
        if ( !IsRepeatOneSet() && renderedTotal >= length ) {
            int16_t * sampleBuf = ( int16_t * ) buf + total * n_channels;
            long fadeEnd = fadeRemain - rendered;
            if ( fadeEnd < 0 )
                fadeEnd = 0;
            float fadePosf = (float)fadeRemain / (float)fadeTotal;
            const float fadeStep = 1.0f / (float)fadeTotal;
            for ( long fadePos = fadeRemain; fadePos > fadeEnd; --fadePos, fadePosf -= fadeStep ) {
                long offset = (fadeRemain - fadePos) * n_channels;
                float sampleLeft = sampleBuf[ offset + 0 ];
                sampleLeft *= fadePosf;
                sampleBuf[ offset + 0 ] = (int16_t)sampleLeft;
                if (n_channels == 2)
                {
                    float sampleRight = sampleBuf[ offset + 1 ];
                    sampleRight *= fadePosf;
                    sampleBuf[ offset + 1 ] = (int16_t)sampleRight;
                }
            }
            rendered = (int)(fadeRemain - fadeEnd);
            fadeRemain = fadeEnd;
        }
        
        total += rendered;
        
        if ( rendered < framesToRender )
            break;
    }
    
    return total;
}

- (long)seek:(long)frame
{
	if (frame < renderedTotal) {
        engine->load(tune);
        renderedTotal = 0;
	}

    int16_t sampleBuffer[1024 * 2];
    
    long remain = ( frame - renderedTotal ) % 32;
    frame /= 32;
    renderedTotal /= 32;
    engine->fastForward( 100 * 32 );
    
    while ( renderedTotal < frame )
    {
        long todo = frame - renderedTotal;
        if ( todo > 1024 )
            todo = 1024;
        int done = engine->play( sampleBuffer, (uint_least32_t)(todo * n_channels) ) / n_channels;
        
        if ( done < todo )
        {
            if ( engine->error() )
                return -1;
            
            renderedTotal = length;
            break;
        }
        
        renderedTotal += todo;
    }
    
    renderedTotal *= 32;
    engine->fastForward( 100 );
    
    if ( remain )
        renderedTotal += engine->play( sampleBuffer, (uint_least32_t)(remain * n_channels) ) / n_channels;
   
   return renderedTotal;
}

- (void)cleanUp
{
    if (builder)
    {
        delete builder;
        builder = NULL;
    }
    
    if (engine)
    {
        delete engine;
        engine = NULL;
    }
    
    if (tune)
    {
        delete tune;
        tune = NULL;
    }
}

- (void)close
{
	[self cleanUp];
}

- (void)dealloc
{
    [self close];
}

- (void)setSource:(id<CogSource>)s
{
	source = s;
}

- (id<CogSource>)source
{
	return source;
}

+ (NSArray *)fileTypes 
{	
	return [NSArray arrayWithObjects:@"sid", @"mus", nil];
}

+ (NSArray *)mimeTypes 
{	
	return nil;
}

+ (float)priority
{
    return 0.5;
}

@end
