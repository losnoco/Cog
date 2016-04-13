//
//  MIDIDecoder.mm
//  MIDI
//
//  Created by Christopher Snowhill on 10/15/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import "MIDIDecoder.h"

#import "AUPlayer.h"
#import "BMPlayer.h"
#import "SCPlayer.h"

#import "Logging.h"

#import <midi_processing/midi_processor.h>

#import "PlaylistController.h"

static OSType getOSType(const char * in_)
{
    const unsigned char * in = (const unsigned char *) in_;
    OSType v = (in[0] << 24) + (in[1] << 16) + (in[2] << 8) + in[3];
    return v;
}

@implementation MIDIDecoder

+ (NSInteger)testExtensions:(NSString *)pathMinusExtension extensions:(NSArray *)extensionsToTest
{
    NSInteger i = 0;
    for (NSString * extension in extensionsToTest)
    {
        if ([[NSFileManager defaultManager] fileExistsAtPath:[pathMinusExtension stringByAppendingPathExtension:extension]])
            return i;
        ++i;
    }
    return -1;
}

- (BOOL)open:(id<CogSource>)s
{
	//We need file-size to use midi_processing
	if (![s seekable]) {
		return NO;
	}
    
    std::vector<uint8_t> file_data;
    
    [s seek:0 whence:SEEK_END];
    size_t size = [s tell];
    [s seek:0 whence:SEEK_SET];
    file_data.resize( size );
    [s read:&file_data[0] amount:size];
    
    if ( !midi_processor::process_file(file_data, [[[s url] pathExtension] UTF8String], midi_file) )
        return NO;
    
	int track_num = [[[s url] fragment] intValue]; //What if theres no fragment? Assuming we get 0.
    
    midi_file.scan_for_loops( true, true, true );

    framesLength = midi_file.get_timestamp_end( track_num, true );
    
    unsigned long loopStart = midi_file.get_timestamp_loop_start( track_num, true );
    unsigned long loopEnd = midi_file.get_timestamp_loop_end( track_num, true );
    
    if ( loopStart == ~0UL ) loopStart = 0;
    if ( loopEnd == ~0UL ) loopEnd = framesLength;
    
    if ( loopStart != 0 || loopEnd != framesLength )
    {
        // two loops and a fade
        framesLength = loopStart + ( loopEnd - loopStart ) * 2;
        framesFade = 8000;
        isLooped = YES;
    }
    else
    {
        framesLength += 1000;
        framesFade = 0;
        isLooped = NO;
    }
    
    framesLength = framesLength * 441 / 10;
    framesFade = framesFade * 441 / 10;
    
    totalFrames = framesLength + framesFade;
    
    NSString * soundFontPath = @"";
    
    if ( [[s url] isFileURL] )
    {
        // Let's check for a SoundFont
        NSArray * extensions = [NSArray arrayWithObjects:@"sflist", @"sf2pack", @"sf2", nil];
        NSString * filePath = [[s url] path];
        NSString * fileNameBase = [filePath lastPathComponent];
        filePath = [filePath stringByDeletingLastPathComponent];
        soundFontPath = [filePath stringByAppendingPathComponent:fileNameBase];
        NSInteger extFound;
        if ((extFound = [MIDIDecoder testExtensions:soundFontPath extensions:extensions]) < 0)
        {
            fileNameBase = [fileNameBase stringByDeletingPathExtension];
            soundFontPath = [filePath stringByAppendingPathComponent:fileNameBase];
            if ((extFound = [MIDIDecoder testExtensions:soundFontPath extensions:extensions]) < 0)
            {
                fileNameBase = [filePath lastPathComponent];
                soundFontPath = [filePath stringByAppendingPathComponent:fileNameBase];
                extFound = [MIDIDecoder testExtensions:soundFontPath extensions:extensions];
            }
        }
        if (extFound >= 0)
        {
            soundFontPath = [soundFontPath stringByAppendingPathExtension:[extensions objectAtIndex:extFound]];
        }
        else
            soundFontPath = @"";
    }
	
	DLog(@"Length: %li", totalFrames);
	
	DLog(@"Track num: %i", track_num);

    NSString * plugin = [[NSUserDefaults standardUserDefaults] stringForKey:@"midi.plugin"];
    if (!plugin || [plugin isEqualToString:@"BASSMIDI"])
    {
        bmplayer = new BMPlayer;
    
        bool resampling_sinc = false;
        NSString * resampling = [[NSUserDefaults standardUserDefaults] stringForKey:@"resampling"];
        if ([resampling isEqualToString:@"sinc"])
            resampling_sinc = true;

        bmplayer->setSincInterpolation( resampling_sinc );
        bmplayer->setSampleRate( 44100 );

        if ( [soundFontPath length] )
            bmplayer->setFileSoundFont( [soundFontPath UTF8String] );
        
        player = bmplayer;
    }
    else
    {
        const char * cplugin = [plugin UTF8String];
        OSType componentSubType;
        OSType componentManufacturer;
        
        componentSubType = getOSType(cplugin);
        componentManufacturer = getOSType(cplugin + 4);
        
        if (componentManufacturer == 'rolD' && componentSubType == 'Sc55')
        {
            SCPlayer * scplayer = new SCPlayer;

            SCPlayer::sc_mode mode = SCPlayer::sc_sc55;
            NSString * flavor = [[NSUserDefaults standardUserDefaults] stringForKey:@"midi.flavor"];
            if ([flavor isEqualToString:@"gm"])
                mode = SCPlayer::sc_gm;
            else if ([flavor isEqualToString:@"gm2"])
                mode = SCPlayer::sc_gm2;
            else if ([flavor isEqualToString:@"sc55"])
                mode = SCPlayer::sc_sc55;
            else if ([flavor isEqualToString:@"sc88"])
                mode = SCPlayer::sc_sc88;
            else if ([flavor isEqualToString:@"sc88pro"])
                mode = SCPlayer::sc_sc88pro;
            else if ([flavor isEqualToString:@"sc8850"])
                mode = SCPlayer::sc_sc8850;
            else if ([flavor isEqualToString:@"xg"])
                mode = SCPlayer::sc_xg;
            
            scplayer->set_sccore_path("/Library/Audio/Plug-Ins/Components/SOUND Canvas VA.component/Contents/Resources/SCCore00.dylib");
            scplayer->set_mode( mode );
            scplayer->setSampleRate( 44100 );
            
            player = scplayer;
        }
        else
        {
            auplayer = new AUPlayer;
        
            auplayer->setComponent(componentSubType, componentManufacturer);
            auplayer->setSampleRate( 44100 );
        
            if ( [soundFontPath length] )
            {
                auplayer->setSoundFont( [soundFontPath UTF8String] );
                soundFontsAssigned = YES;
            }

            player = auplayer;
        }
    }
    
    unsigned int loop_mode = framesFade ? MIDIPlayer::loop_mode_enable | MIDIPlayer::loop_mode_force : 0;
    unsigned int clean_flags = midi_container::clean_flag_emidi;
    
    if ( !player->Load( midi_file, track_num, loop_mode, clean_flags) )
        return NO;
    
    framesRead = 0;
    
	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];
	
	return YES;
}

- (NSDictionary *)properties
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		[NSNumber numberWithInt:0], @"bitrate",
		[NSNumber numberWithFloat:44100], @"sampleRate",
		[NSNumber numberWithLong:totalFrames], @"totalFrames",
		[NSNumber numberWithInt:32], @"bitsPerSample",
        [NSNumber numberWithBool:YES], @"floatingPoint",
		[NSNumber numberWithInt:2], @"channels", //output from gme_play is in stereo
		[NSNumber numberWithBool:YES], @"seekable",
		@"host", @"endian",
		nil];
}

- (int)readAudio:(void *)buf frames:(UInt32)frames
{
    BOOL repeatone = IsRepeatOneSet();
    long localFramesLength = framesLength;
    long localTotalFrames = totalFrames;
    
    player->SetLoopMode((repeatone || isLooped) ? (MIDIPlayer::loop_mode_enable | MIDIPlayer::loop_mode_force) : 0);
    
    if ( !repeatone && framesRead >= localTotalFrames )
        return 0;
    
    if ( (bmplayer||auplayer) && !soundFontsAssigned ) {
        NSString * soundFontPath = [[NSUserDefaults standardUserDefaults] stringForKey:@"soundFontPath"];
        if (soundFontPath == nil)
            return 0;
        
        if (bmplayer)
            bmplayer->setSoundFont( [soundFontPath UTF8String] );
        else if (auplayer)
            auplayer->setSoundFont( [soundFontPath UTF8String] );
        
        soundFontsAssigned = YES;
    }
    
    player->Play( (float *) buf, frames );
    
    if ( !repeatone && framesRead + frames > localFramesLength ) {
        if ( framesFade ) {
            long fadeStart = (localFramesLength > framesRead) ? localFramesLength : framesRead;
            long fadeEnd = (framesRead + frames > localTotalFrames) ? localTotalFrames : (framesRead + frames);
            long fadePos;
        
            float * buff = ( float * ) buf;
        
            float fadeScale = (float)(framesFade - (fadeStart - localFramesLength)) / framesFade;
            float fadeStep = 1.0 / (float)framesFade;
            for (fadePos = fadeStart; fadePos < fadeEnd; ++fadePos) {
                buff[ 0 ] *= fadeScale;
                buff[ 1 ] *= fadeScale;
                buff += 2;
                fadeScale -= fadeStep;
                if (fadeScale < 0) {
                    fadeScale = 0;
                    fadeStep = 0;
                }
            }
            
            frames = (int)(fadeEnd - framesRead);
        }
        else {
            frames = (int)(localTotalFrames - framesRead);
        }
    }
    
	framesRead += frames;
	return frames;
}

- (long)seek:(long)frame
{
    player->Seek( frame );
	
    framesRead = frame;
    
	return frame;
}

- (void)close
{
    delete player;
    player = NULL;
}

+ (NSArray *)fileTypes 
{	
	return [NSArray arrayWithObjects:@"mid", @"midi", @"kar", @"rmi", @"mids", @"mds", @"hmi", @"hmp", @"mus", @"xmi", @"lds", nil];
}

+ (NSArray *)mimeTypes 
{	
	return [NSArray arrayWithObjects:@"audio/midi", @"audio/x-midi", nil];
}

+ (float)priority
{
    return 1.0;
}

@end
