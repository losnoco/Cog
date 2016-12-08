//
//  VGMDecoder.m
//  vgmstream
//
//  Created by Christopher Snowhill on 02/25/14.
//  Copyright 2014 __NoWork, Inc__. All rights reserved.
//

#import "VGMDecoder.h"

#import "PlaylistController.h"

typedef struct _COGSTREAMFILE {
	STREAMFILE sf;
    void *file;
	off_t offset;
	char name[PATH_LIMIT];
} COGSTREAMFILE;

static void cogsf_seek(COGSTREAMFILE *this, off_t offset) {
    NSObject* _file = (__bridge NSObject *)(this->file);
    id<CogSource> __unsafe_unretained file = (id) _file;
	if ([file seek:offset whence:SEEK_SET] != 0)
		this->offset = offset;
	else
		this->offset = [file tell];
}

static off_t cogsf_get_size(COGSTREAMFILE *this) {
    NSObject* _file = (__bridge NSObject *)(this->file);
    id<CogSource> __unsafe_unretained file = (id) _file;
    off_t offset = [file tell];
    [file seek:0 whence:SEEK_END];
    off_t size = [file tell];
    [file seek:offset whence:SEEK_SET];
    return size;
}

static off_t cogsf_get_offset(COGSTREAMFILE *this) {
	return this->offset;
}

static void cogsf_get_name(COGSTREAMFILE *this, char *buffer, size_t length) {
	strncpy(buffer, this->name, length);
	buffer[length-1]='\0';
}

static size_t cogsf_read(COGSTREAMFILE *this, uint8_t *dest, off_t offset, size_t length) {
    NSObject* _file = (__bridge NSObject *)(this->file);
    id<CogSource> __unsafe_unretained file = (id) _file;
	size_t read;
	if (this->offset != offset)
		cogsf_seek(this, offset);
	read = [file read:dest amount:length];
	if (read > 0)
		this->offset += read;
	return read;
}

static void cogsf_close(COGSTREAMFILE *this) {
    CFBridgingRelease(this->file);
	free(this);
}

static STREAMFILE *cogsf_create_from_path(const char *path);
static STREAMFILE *cogsf_open(COGSTREAMFILE *this, const char *const filename,size_t buffersize) {
	if (!filename) return NULL;
	return cogsf_create_from_path(filename);
}

static STREAMFILE *cogsf_create(id file, const char *path) {
	COGSTREAMFILE *streamfile = malloc(sizeof(COGSTREAMFILE));
    
	if (!streamfile) return NULL;
    
	memset(streamfile,0,sizeof(COGSTREAMFILE));
	streamfile->sf.read = (void*)cogsf_read;
	streamfile->sf.get_size = (void*)cogsf_get_size;
	streamfile->sf.get_offset = (void*)cogsf_get_offset;
	streamfile->sf.get_name = (void*)cogsf_get_name;
	streamfile->sf.get_realname = (void*)cogsf_get_name;
	streamfile->sf.open = (void*)cogsf_open;
	streamfile->sf.close = (void*)cogsf_close;
	streamfile->file = (void*)CFBridgingRetain(file);
	streamfile->offset = 0;
	strncpy(streamfile->name, path, sizeof(streamfile->name));
    
	return &streamfile->sf;
}

STREAMFILE *cogsf_create_from_path(const char *path) {
    id<CogSource> source;
    NSString * urlString = [NSString stringWithUTF8String:path];
    NSURL * url = [NSURL URLWithString:[urlString stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding]];
    
    id audioSourceClass = NSClassFromString(@"AudioSource");
    source = [audioSourceClass audioSourceForURL:url];
    
    if (![source open:url])
        return 0;
    
    if (![source seekable])
        return 0;

	return cogsf_create(source, path);
}

VGMSTREAM *init_vgmstream_from_cogfile(const char *path) {
	STREAMFILE *sf;
	VGMSTREAM *vgm;
    
	sf = cogsf_create_from_path(path);
    
    if (sf) {
        vgm = init_vgmstream_from_STREAMFILE(sf);
        cogsf_close((COGSTREAMFILE *)sf);
    }
    
	return vgm;
}

@implementation VGMDecoder

- (BOOL)open:(id<CogSource>)s
{
    stream = init_vgmstream_from_cogfile([[[[s url] absoluteString] stringByReplacingPercentEscapesUsingEncoding:NSUTF8StringEncoding] UTF8String]);
    if ( !stream )
        return NO;
    
    sampleRate = stream->sample_rate;
    channels = stream->channels;
    totalFrames = get_vgmstream_play_samples( 2.0, 10.0, 10.0, stream );
    framesFade = stream->loop_flag ? sampleRate * 10 : 0;
    framesLength = totalFrames - framesFade;
    
    framesRead = 0;
    
    bitrate = get_vgmstream_average_bitrate(stream);
    
    [self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];
    
	return YES;
}

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
            [NSNumber numberWithInt:bitrate / 1000], @"bitrate",
            [NSNumber numberWithInt:sampleRate], @"sampleRate",
            [NSNumber numberWithDouble:totalFrames], @"totalFrames",
            [NSNumber numberWithInt:16], @"bitsPerSample",
            [NSNumber numberWithBool:NO], @"floatingPoint",
            [NSNumber numberWithInt:channels], @"channels",
            [NSNumber numberWithBool:YES], @"seekable",
            @"host", @"endian",
            nil];
}

- (int)readAudio:(void *)buf frames:(UInt32)frames
{
    BOOL repeatone = IsRepeatOneSet();
    
    if (!repeatone) {
        if (framesRead >= totalFrames) return 0;
        else if (framesRead + frames > totalFrames)
            frames = totalFrames - framesRead;
    }

    sample * sbuf = (sample *) buf;
    
    render_vgmstream( sbuf, frames, stream );
    
    if ( !repeatone && framesFade && framesRead + frames > framesLength ) {
        long fadeStart = (framesLength > framesRead) ? framesLength : framesRead;
        long fadeEnd = (framesRead + frames) > totalFrames ? totalFrames : (framesRead + frames);
        long fadePos;
        
        int64_t fadeScale = (int64_t)(totalFrames - fadeStart) * INT_MAX / framesFade;
        int64_t fadeStep = INT_MAX / framesFade;
        sbuf += (fadeStart - framesRead) * 2;
        for (fadePos = fadeStart; fadePos < fadeEnd; ++fadePos) {
            sbuf[ 0 ] = (int16_t)((int64_t)(sbuf[ 0 ]) * fadeScale / INT_MAX);
            sbuf[ 1 ] = (int16_t)((int64_t)(sbuf[ 1 ]) * fadeScale / INT_MAX);
            sbuf += 2;
            fadeScale -= fadeStep;
            if (fadeScale <= 0) break;
        }
        frames = (UInt32)(fadePos - framesRead);
    }
    
    framesRead += frames;
    
    return frames;
}

- (long)seek:(long)frame
{
    // Constrain the seek offset to within the loop, if any
    if(stream->loop_flag && (stream->loop_end_sample - stream->loop_start_sample) && frame >= stream->loop_end_sample) {
        frame -= stream->loop_start_sample;
        frame %= (stream->loop_end_sample - stream->loop_start_sample);
        frame += stream->loop_start_sample;
    }
    
    if (frame < framesRead) {
        reset_vgmstream( stream );
        framesRead = 0;
    }
    
    while (framesRead < frame) {
        sample buffer[1024];
        long max_sample_count = 1024 / channels;
        long samples_to_skip = frame - framesRead;
        if ( samples_to_skip > max_sample_count )
            samples_to_skip = max_sample_count;
        render_vgmstream( buffer, (int)samples_to_skip, stream );
        framesRead += samples_to_skip;
    }
    
    return framesRead;
}

- (void)close
{
    close_vgmstream( stream );
    stream = NULL;
}

- (void)dealloc
{
    [self close];
}

+ (NSArray *)fileTypes 
{	
	return [NSArray arrayWithObjects:@"2dx9", @"aaap", @"aax", @"acm", @"adp", @"adpcm", @"ads", @"adx", @"afc", @"agsc", @"ahx",@"aifc", @"aiff", @"aix", @"amts", @"as4", @"asd", @"asf", @"asr", @"ass", @"ast", @"at3", @"aud", @"aus", @"baf", @"baka", @"bar", @"bcstm", @"bcwav", @"bfstm", @"bfwav", @"bfwavnsmbu", @"bg00", @"bgw", @"bh2pcm", @"bmdx", @"bns", @"bnsf", @"bo2", @"brstm", @"caf", @"capdsp", @"ccc", @"cfn", @"cnk", @"dcs", @"dcsw", @"ddsp", @"de2", @"dmsg", @"dsp", @"dvi", @"dxh", @"eam", @"emff", @"enth", @"fag", @"filp", @"fsb", @"fwav", @"gca", @"gcm", @"gcsw", @"gcw", @"genh", @"gms", @"gsp", @"hca", @"hgc1", @"his", @"hps", @"hwas", @"idsp", @"idvi", @"ikm", @"ild", @"int", @"isd", @"ish", @"ivaud", @"ivb", @"joe", @"kces", @"kcey", @"khv", @"kraw", @"leg", @"logg", @"lps", @"lsf", @"lwav", @"matx", @"mcg", @"mi4", @"mib", @"mic", @"mihb", @"mpdsp", @"msa", @"mss", @"msvp", @"mtaf", @"mus", @"musc", @"musx", @"mwv", @"myspd", @"ndp", @"npsf", @"nus3bank", @"nwa", @"omu", @"otm", @"p3d", @"pcm", @"pdt", @"pnb", @"pos", @"psh", @"psw", @"raw", @"rkv", @"rnd", @"rrds", @"rsd", @"rsf", @"rstm", @"rwar", @"rwav", @"rws", @"rwsd", @"rwx", @"rxw", @"s14", @"sab", @"sad", @"sap", @"sc", @"scd", @"sd9", @"sdt", @"seg", @"sfl", @"sfs", @"sgb", @"sgd", @"sgx", @"sl3", @"sli", @"smp", @"smpl", @"snd", @"sng", @"sns", @"spd", @"sps", @"spsd", @"spt", @"spw", @"ss2", @"ss7", @"ssm", @"sss", @"ster", @"sth", @"stm", @"stma", @"str", @"strm", @"sts", @"stx", @"svag", @"svs", @"swav", @"swd", @"tec", @"thp", @"tk5", @"tydsp", @"um3", @"vag", @"vas", @"vgs", @"vig", @"vjdsp", @"voi", @"vpk", @"vs", @"vsf", @"waa", @"wac", @"wad", @"wam", @"was", @"wavm", @"wb", @"wii", @"wp2", @"wsd", @"wsi", @"wvs", @"xa", @"xa2", @"xa30", @"xma", @"xmu", @"xss", @"xvas", @"xwav", @"xwb", @"xwma", @"ydsp", @"ymf", @"zsd", @"zwdsp", @"vgmstream", @"vgms", nil];
}

+ (NSArray *)mimeTypes 
{	
	return nil;
}

+ (float)priority
{
    return 0.0;
}

@end
