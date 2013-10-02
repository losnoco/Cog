//
//  InputNode.m
//  Cog
//
//  Created by Vincent Spader on 8/2/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

#import "InputNode.h"
#import "BufferChain.h"
#import "Plugin.h"
#import "CoreAudioUtils.h"


static BOOL hostIsBigEndian()
{
#ifdef __BIG_ENDIAN__
	return YES;
#else
	return NO;
#endif
}

@implementation InputNode

- (BOOL)openWithSource:(id<CogSource>)source
{
	decoder = [AudioDecoder audioDecoderForSource:source];
	[decoder retain];

	if (decoder == nil)
		return NO;

	[self registerObservers];

	if (![decoder open:source])
	{
		NSLog(@"Couldn't open decoder...");
		return NO;
	}
	
	NSDictionary *properties = [decoder properties];
	int bitsPerSample = [[properties objectForKey:@"bitsPerSample"] intValue];
	int channels = [[properties objectForKey:@"channels"] intValue];
	
    bytesPerSample = bitsPerSample / 8;
	bytesPerFrame = bytesPerSample * channels;
    
	if (([[properties objectForKey:@"endian"] isEqualToString:@"big"] && !hostIsBigEndian()) ||
        ([[properties objectForKey:@"endian"] isEqualToString:@"little"] && hostIsBigEndian())) {
        swapEndian = YES;
    }
    else {
        swapEndian = NO;
    }
    
    [self refreshVolumeScaling];
	
	shouldContinue = YES;
	shouldSeek = NO;

	return YES;
}

- (BOOL)openWithDecoder:(id<CogDecoder>) d
{
	NSLog(@"Opening with old decoder: %@", d);
	decoder = d;
	[decoder retain];

	NSDictionary *properties = [decoder properties];
	int bitsPerSample = [[properties objectForKey:@"bitsPerSample"] intValue];
	int channels = [[properties objectForKey:@"channels"] intValue];
	
    bytesPerSample = bitsPerSample / 8;
	bytesPerFrame = bytesPerSample * channels;
    
    [self refreshVolumeScaling];
	
	[self registerObservers];

	shouldContinue = YES;
	shouldSeek = NO;
	
	NSLog(@"DONES: %@", decoder);
	return YES;
}


- (void)registerObservers
{
	NSLog(@"REGISTERING OBSERVERS");
	[decoder addObserver:self
			  forKeyPath:@"properties" 
				 options:(NSKeyValueObservingOptionNew)
				 context:NULL];

	[decoder addObserver:self
			  forKeyPath:@"metadata" 
				 options:(NSKeyValueObservingOptionNew)
				 context:NULL];

	[[NSUserDefaultsController sharedUserDefaultsController] addObserver:self forKeyPath:@"values.volumeScaling"		options:0 context:nil];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
					  ofObject:(id)object 
                        change:(NSDictionary *)change
                       context:(void *)context
{
	NSLog(@"SOMETHING CHANGED!");
	if ([keyPath isEqual:@"properties"]) {
		//Setup converter!
		//Inform something of properties change
		//Disable support until it is properly implimented.
		//[controller inputFormatDidChange: propertiesToASBD([decoder properties])];
        [self refreshVolumeScaling];
	}
	else if ([keyPath isEqual:@"metadata"]) {
		//Inform something of metadata change
	}
    else if ([keyPath isEqual:@"values.volumeScaling"]) {
        //User reset the volume scaling option
        [self refreshVolumeScaling];
    }
}

static float db_to_scale(float db)
{
    return pow(10.0, db / 20);
}

- (void)refreshVolumeScaling
{
	NSDictionary *properties = [decoder properties];
    if (rgInfo != nil)
        properties = rgInfo;
    NSString * scaling = [[NSUserDefaults standardUserDefaults] stringForKey:@"volumeScaling"];
    BOOL useAlbum = [scaling hasPrefix:@"albumGain"];
    BOOL useTrack = useAlbum || [scaling hasPrefix:@"trackGain"];
    BOOL useVolume = useAlbum || useTrack || [scaling isEqualToString:@"volumeScale"];
    BOOL usePeak = [scaling hasSuffix:@"WithPeak"];
    float scale = 1.0;
    float peak = 0.0;
    if (useVolume) {
        id pVolumeScale = [properties objectForKey:@"volume"];
        if (pVolumeScale != nil)
            scale = [pVolumeScale floatValue];
    }
    if (useTrack) {
        id trackGain = [properties objectForKey:@"replayGainTrackGain"];
        id trackPeak = [properties objectForKey:@"replayGainTrackPeak"];
        if (trackGain != nil)
            scale = db_to_scale([trackGain floatValue]);
        if (trackPeak != nil)
            peak = [trackPeak floatValue];
    }
    if (useAlbum) {
        id albumGain = [properties objectForKey:@"replayGainAlbumGain"];
        id albumPeak = [properties objectForKey:@"replayGainAlbumPeak"];
        if (albumGain != nil)
            scale = db_to_scale([albumGain floatValue]);
        if (albumPeak != nil)
            peak = [albumPeak floatValue];
    }
    if (usePeak) {
        if (scale * peak > 1.0)
            scale = 1.0 / peak;
    }
    volumeScale = scale * 4096;
}

static int16_t swap_16(uint16_t input)
{
    return (input >> 8) | (input << 8);
}

static int32_t swap_24(uint32_t input)
{
    int32_t temp = (input << 24) >> 8;
    return temp | ((input >> 16) & 0xff) | (input & 0xff00);
}

static int32_t swap_32(uint32_t input)
{
    return (input >> 24) | ((input >> 8) & 0xff00) | ((input << 8) & 0xff0000) | (input << 24);
}

- (void)process
{
	int amountInBuffer = 0;
	void *inputBuffer = malloc(CHUNK_SIZE);
	
	BOOL shouldClose = YES;
	
	while ([self shouldContinue] == YES && [self endOfStream] == NO)
	{
		if (shouldSeek == YES)
		{
			NSLog(@"SEEKING!");
			[decoder seek:seekFrame];
			shouldSeek = NO;
			NSLog(@"Seeked! Resetting Buffer");
			
			[self resetBuffer];
			
			NSLog(@"Reset buffer!");
			initialBufferFilled = NO;
		}

		if (amountInBuffer < CHUNK_SIZE) {
			int framesToRead = (CHUNK_SIZE - amountInBuffer)/bytesPerFrame;
			int framesRead = [decoder readAudio:((char *)inputBuffer) + amountInBuffer frames:framesToRead];
			amountInBuffer += (framesRead * bytesPerFrame);

			if (framesRead <= 0)
			{
				if (initialBufferFilled == NO) {
					[controller initialBufferFilled:self];
				}
				
				NSLog(@"End of stream? %@", [self properties]);
				endOfStream = YES;
				shouldClose = [controller endOfInputReached]; //Lets us know if we should keep going or not (occassionally, for track changes within a file)
				NSLog(@"closing? is %i", shouldClose);
				break; 
			}
            
            if (volumeScale != 4096) {
                int totalFrames = amountInBuffer / bytesPerSample;
                switch (bytesPerSample) {
                    case 1:
                    {
                        uint8_t * samples = (uint8_t *)inputBuffer;
                        for (int i = 0; i < totalFrames; i++)
                        {
                            int32_t sample = (int8_t)samples[i] - 128;
                            sample = (sample * volumeScale) >> 12;
                            if ((unsigned)(sample + 0x80) & 0xffffff00) sample = (sample >> 31) ^ 0x7f;
                            samples[i] = sample + 128;
                        }
                    }
                    break;
                        
                    case 2:
                    {
                        int16_t * samples = (int16_t *)inputBuffer;
                        for (int i = 0; i < totalFrames; i++)
                        {
                            int32_t sample = samples[i];
                            if (swapEndian) sample = swap_16(sample);
                            sample = (sample * volumeScale) >> 12;
                            if ((unsigned)(sample + 0x8000) & 0xffff0000) sample = (sample >> 31) ^ 0x7fff;
                            if (swapEndian) sample = swap_16(sample);
                            samples[i] = sample;
                        }
                    }
                    break;
                    
                    case 3:
                    {
                        uint8_t * samples = (uint8_t *)inputBuffer;
                        for (int i = 0; i < totalFrames; i++)
                        {
                            int32_t sample = (samples[i * 3] << 8) | (samples[i * 3 + 1] << 16) | (samples[i * 3 + 2] << 24);
                            sample >>= 8;
                            if (swapEndian) sample = swap_24(sample);
                            sample = (sample * volumeScale) >> 12;
                            if ((unsigned)(sample + 0x800000) & 0xff000000) sample = (sample >> 31) ^ 0x7fffff;
                            if (swapEndian) sample = swap_24(sample);
                            samples[i * 3] = sample;
                            samples[i * 3 + 1] = sample >> 8;
                            samples[i * 3 + 2] = sample >> 16;
                        }
                    }
                    break;
                        
                    case 4:
                    {
                        int32_t * samples = (int32_t *)inputBuffer;
                        for (int i = 0; i < totalFrames; i++)
                        {
                            int64_t sample = samples[i];
                            if (swapEndian) sample = swap_32(sample);
                            sample = (sample * volumeScale) >> 12;
                            if ((unsigned)(sample + 0x80000000) & 0xffffffff00000000) sample = (sample >> 63) ^ 0x7fffffff;
                            if (swapEndian) sample = swap_32(sample);
                            samples[i] = sample;
                        }
                    }
                    break;
                }
            }
		
			[self writeData:inputBuffer amount:amountInBuffer];
			amountInBuffer = 0;
		}
	}
	if (shouldClose)
		[decoder close];
	
	free(inputBuffer);
}

- (void)seek:(long)frame
{
	seekFrame = frame;
	shouldSeek = YES;
	NSLog(@"Should seek!");
	[semaphore signal];
}

- (BOOL)setTrack:(NSURL *)track
{
	if ([decoder respondsToSelector:@selector(setTrack:)] && [decoder setTrack:track]) {
		NSLog(@"SET TRACK!");
		
		return YES;
	}
	
	return NO;
}

- (void)setRGInfo:(NSDictionary *)i
{
    [i retain];
    [rgInfo release];
    rgInfo = i;
    [self refreshVolumeScaling];
}

- (void)dealloc
{
	NSLog(@"Input Node dealloc");
    
    [rgInfo release];

	[decoder removeObserver:self forKeyPath:@"properties"];
	[decoder removeObserver:self forKeyPath:@"metadata"];

	[decoder release];
	
	[super dealloc];
}

- (NSDictionary *) properties
{
	return [decoder properties];
}

- (id<CogDecoder>) decoder
{
	return decoder;
}

@end
