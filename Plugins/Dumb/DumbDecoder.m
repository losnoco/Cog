//
//  DumbFile.m
//  Cog
//
//  Created by Vincent Spader on 5/29/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#define _USE_SSE 1

#import "DumbDecoder.h"

#import "umx.h"
#import "j2b.h"
#import "mo3.h"

#import "Logging.h"

#import "PlaylistController.h"

@implementation DumbDecoder

struct MEMANDFREEFILE
{
	char *ptr, *ptr_begin;
	long left, size;
    int is_mo3;
};

static int dumb_maffile_skip(void *f, long n)
{
	struct MEMANDFREEFILE *m = f;
	if (n > m->left) return -1;
	m->ptr += n;
	m->left -= n;
	return 0;
}

static int dumb_maffile_getc(void *f)
{
	struct MEMANDFREEFILE *m = f;
	if (m->left <= 0) return -1;
	m->left--;
	return *(const unsigned char *)m->ptr++;
}

static long dumb_maffile_getnc(char *ptr, long n, void *f)
{
	struct MEMANDFREEFILE *m = f;
	if (n > m->left) n = m->left;
	memcpy(ptr, m->ptr, n);
	m->ptr += n;
	m->left -= n;
	return n;
}

static void dumb_maffile_close(void *f)
{
    struct MEMANDFREEFILE *m = f;
    if (m->is_mo3) freeMo3(m->ptr_begin);
    else free(m->ptr_begin);
	free(f);
}

static int dumb_maffile_seek(void *f, long n)
{
	struct MEMANDFREEFILE *m = f;
    
	m->ptr = m->ptr_begin + n;
	m->left = m->size - n;
    
	return 0;
}

static long dumb_maffile_get_size(void *f)
{
	struct MEMANDFREEFILE *m = f;
	return m->size;
}

static const DUMBFILE_SYSTEM maffile_dfs = {
	NULL,
	&dumb_maffile_skip,
	&dumb_maffile_getc,
	&dumb_maffile_getnc,
	&dumb_maffile_close,
	&dumb_maffile_seek,
	&dumb_maffile_get_size
};

DUMBFILE *dumbfile_open_memory_and_free(char *data, long size)
{
    int is_mo3 = 0;
    char * try_data = unpackMo3( data, &size );
    if ( try_data ) {
        free( data );
        data = try_data;
        is_mo3 = 1;
    }
    else {
        try_data = unpackUmx( data, &size );
        if ( try_data ) {
            free( data );
            data = try_data;
        }
        else {
            try_data = unpackJ2b( data, &size );
            if ( try_data ) {
                free( data );
                data = try_data;
            }
        }
    }
    
	struct MEMANDFREEFILE *m = malloc(sizeof(*m));
	if (!m) return NULL;
    
	m->ptr_begin = data;
	m->ptr = data;
	m->left = size;
	m->size = size;
    m->is_mo3 = is_mo3;
    
	return dumbfile_open_ex(m, &maffile_dfs);
}

+ (void)initialize
{
    if (self == [DumbDecoder class])
    {
        // do this here so we don't have to wait on it later
        _dumb_init_cubic();
        _dumb_init_sse();
    }
}

- (id)init
{
    self = [super init];
    if (self) {
        sampptr = NULL;
        dsr = NULL;
        duh = NULL;
    }
    return self;
}

int callbackLoop(void *data)
{
    long * loops = (long *) data;
    ++ *loops;
    return 0;
}

- (BOOL)open:(id<CogSource>)s
{
	[self setSource:s];
	
    [source seek:0 whence:SEEK_END];
    long size = [source tell];
    [source seek:0 whence:SEEK_SET];
    
    void * data = malloc(size);
    [source read:data amount:size];
	
	DUMBFILE * df = dumbfile_open_memory_and_free( data, size );
	if (!df)
	{
		ALog(@"Open failed for file: %@", [[s url] absoluteString]);
		return NO;
	}

    int subsong = 0;
    int startOrder = 0;
    
    NSURL * url = [s url];
	int track_num;
	if ([[url fragment] length] == 0)
		track_num = 0;
	else
		track_num = [[url fragment] intValue];

    if ( dumb_get_psm_subsong_count( df ) )
        subsong = track_num;
    else
        startOrder = track_num;
    
    dumbfile_seek( df, 0, SEEK_SET );
	
	NSString *ext = [[[s url] pathExtension] lowercaseString];
    duh = dumb_read_any(df, [ext isEqualToString:@"mod"] ? 0 : 1, subsong);
	if (!duh)
	{
		ALog(@"Failed to create duh");
        dumbfile_close(df);
		return NO;
	}
    dumbfile_close(df);
	
	length = dumb_it_build_checkpoints( duh_get_it_sigdata( duh ), startOrder );
	
	dsr = duh_start_sigrenderer(duh, 0, 2 /* stereo */, startOrder);
	if (!dsr) 
	{
		ALog(@"Failed to create dsr");
		return NO;
	}
	
    DUMB_IT_SIGRENDERER * itsr = duh_get_it_sigrenderer( dsr );

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

    dumb_it_set_resampling_quality( itsr, resampling_int );
    dumb_it_set_ramp_style(itsr, 2);
    dumb_it_set_loop_callback( itsr, callbackLoop, &loops);
    dumb_it_set_xm_speed_zero_callback( itsr, dumb_it_callback_terminate, 0);
    dumb_it_set_global_volume_zero_callback( itsr, dumb_it_callback_terminate, 0);
    
    loops = 0;
    fadeTotal = fadeRemain = 44100 * 8;
    
    sampptr = allocate_sample_buffer(2, 1024);
    
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
		[NSNumber numberWithInt:32], @"bitsPerSample", //Samples are short
        [NSNumber numberWithBool:YES], @"floatingPoint",
		[NSNumber numberWithInt:2], @"channels", //output from gme_play is in stereo
		[NSNumber numberWithBool:[source seekable]], @"seekable",
		@"host", @"endian",
		nil];
}

- (int)readAudio:(void *)buf frames:(UInt32)frames
{
    int total = 0;
    while ( total < frames ) {
        int framesToRender = 1024;
        if ( framesToRender > frames )
            framesToRender = frames;
        dumb_silence( sampptr[0], framesToRender * 2 );
        int rendered = (int) duh_sigrenderer_generate_samples( dsr, 1.0, 65536.0f / 44100.0f, framesToRender, sampptr );
        
        if (rendered <= 0)
            break;
        
        for ( int i = 0; i < rendered * 2; i++ ) {
            const float scale = 1.0 / 0x800000;
            ((float *)buf)[(total * 2) + i] = (float)sampptr[0][i] * scale;
        }
    
        if ( !IsRepeatOneSet() && loops >= 2 ) {
            float * sampleBuf = ( float * ) buf + total * 2;
            long fadeEnd = fadeRemain - rendered;
            if ( fadeEnd < 0 )
                fadeEnd = 0;
            float fadePosf = (float)fadeRemain / fadeTotal;
            const float fadeStep = 1.0 / fadeTotal;
            for ( long fadePos = fadeRemain; fadePos > fadeEnd; --fadePos, fadePosf -= fadeStep ) {
                long offset = (fadeRemain - fadePos) * 2;
                float sampleLeft = sampleBuf[ offset + 0 ];
                float sampleRight = sampleBuf[ offset + 1 ];
                sampleLeft *= fadePosf;
                sampleRight *= fadePosf;
                sampleBuf[ offset + 0 ] = sampleLeft;
                sampleBuf[ offset + 1 ] = sampleRight;
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
    if (sampptr) {
        destroy_sample_buffer(sampptr);
        sampptr = NULL;
    }
    
	if (dsr) {
		duh_end_sigrenderer(dsr);
		dsr = NULL;
	}
	if (duh) {
		unload_duh(duh);
		duh = NULL;
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
	return [NSArray arrayWithObjects:@"it", @"itz", @"xm", @"xmz", @"s3m", @"s3z", @"mod", @"mdz", @"stm", @"stz", @"ptm", @"mtm", @"669", @"psm", @"am", @"j2b", @"dsm", @"amf", @"okt", @"okta", @"umx", @"mo3", nil];
}

+ (NSArray *)mimeTypes 
{	
	return [NSArray arrayWithObjects:@"audio/x-it", @"audio/x-xm", @"audio/x-s3m", @"audio/x-mod", nil];
}

+ (float)priority
{
    return 1.0;
}

@end
