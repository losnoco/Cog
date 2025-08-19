//
//  WavPackFile.m
//  Cog
//
//  Created by Vincent Spader on 6/6/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "WavPackDecoder.h"

#import "Logging.h"

#import <sys/sysctl.h>

static int worker_threads;

int get_default_worker_threads (void)
{
	int num_processors = 1;

	size_t len = sizeof (num_processors);
	int mib[2] = { CTL_HW, HW_NCPU };
	sysctl (mib, 2, &num_processors, &len, NULL, 0);

	if (num_processors <= 1)
		return 0;
	else if (num_processors > 4)
		return 4;
	else
		return num_processors;
}

@implementation WavPackReader
+ (void)initialize {
	worker_threads = get_default_worker_threads ();
}

- (id)initWithSource:(id<CogSource>)s {
	self = [super init];
	if(self) {
		source = s;
	}
	return self;
}

- (void)setSource:(id<CogSource>)s {
	source = s;
}

- (id<CogSource>)source {
	return source;
}
@end

@implementation WavPackDecoder

int32_t ReadBytesProc(void *ds, void *data, int32_t bcount) {
	WavPackReader *wv = (__bridge WavPackReader *)ds;

	return (int32_t)[[wv source] read:data amount:bcount];
}

uint32_t GetPosProc(void *ds) {
	WavPackReader *wv = (__bridge WavPackReader *)ds;

	return (uint32_t)[[wv source] tell];
}

int SetPosAbsProc(void *ds, uint32_t pos) {
	WavPackReader *wv = (__bridge WavPackReader *)ds;

	return ([[wv source] seek:pos whence:SEEK_SET] ? 0 : -1);
}

int SetPosRelProc(void *ds, int32_t delta, int mode) {
	WavPackReader *wv = (__bridge WavPackReader *)ds;

	return ([[wv source] seek:delta whence:mode] ? 0 : -1);
}

int PushBackByteProc(void *ds, int c) {
	WavPackReader *wv = (__bridge WavPackReader *)ds;

	if([[wv source] seekable]) {
		[[wv source] seek:-1 whence:SEEK_CUR];

		return c;
	} else {
		return -1;
	}
}

uint32_t GetLengthProc(void *ds) {
	WavPackReader *wv = (__bridge WavPackReader *)ds;

	if([[wv source] seekable]) {
		long currentPos = [[wv source] tell];

		[[wv source] seek:0 whence:SEEK_END];
		long size = [[wv source] tell];

		[[wv source] seek:currentPos whence:SEEK_SET];

		return (uint32_t)size;
	} else {
		return 0;
	}
}

int CanSeekProc(void *ds) {
	WavPackReader *wv = (__bridge WavPackReader *)ds;

	return [[wv source] seekable];
}

int32_t WriteBytesProc(void *ds, void *data, int32_t bcount) {
	return -1;
}

- (id)init {
	self = [super init];
	if(self) {
		wpc = NULL;
		inputBuffer = NULL;
		inputBufferSize = 0;
	}
	return self;
}

- (BOOL)open:(id<CogSource>)s {
	int open_flags = 0;
	char error[80];

	wv = [[WavPackReader alloc] initWithSource:s];

	if([s seekable]) {
		id audioSourceClass = NSClassFromString(@"AudioSource");
		NSURL *wvcurl = [[s url] URLByDeletingPathExtension];
		wvcurl = [wvcurl URLByAppendingPathExtension:@"wvc"];
		id<CogSource> wvcsrc = [audioSourceClass audioSourceForURL:wvcurl];
		if([wvcsrc open:wvcurl]) {
			wvc = [[WavPackReader alloc] initWithSource:wvcsrc];
		} else {
			wvc = nil;
		}
	} else {
		wvc = nil;
	}

	reader.read_bytes = ReadBytesProc;
	reader.get_pos = GetPosProc;
	reader.set_pos_abs = SetPosAbsProc;
	reader.set_pos_rel = SetPosRelProc;
	reader.push_back_byte = PushBackByteProc;
	reader.get_length = GetLengthProc;
	reader.can_seek = CanSeekProc;
	reader.write_bytes = WriteBytesProc;

	open_flags |= OPEN_DSD_NATIVE | OPEN_ALT_TYPES;

	if(![s seekable])
		open_flags |= OPEN_STREAMING;

	if (worker_threads)
		open_flags |= worker_threads << OPEN_THREADS_SHFT;

	wpc = WavpackOpenFileInputEx(&reader, (__bridge void *)(wv), (__bridge void *)(wvc), error, open_flags, 0);
	if(!wpc) {
		DLog(@"Unable to open file..");
		return NO;
	}

	channels = WavpackGetNumChannels(wpc);
	channelConfig = WavpackGetChannelMask(wpc);
	bitsPerSample = WavpackGetBitsPerSample(wpc);

	frequency = WavpackGetSampleRate(wpc);

	totalFrames = WavpackGetNumSamples(wpc);
	frame = 0;

	isDSD = NO;

	float nativeFrequency = WavpackGetNativeSampleRate(wpc);

	if(nativeFrequency != frequency && bitsPerSample == 8) {
		isDSD = YES;
		frequency = nativeFrequency;
		bitsPerSample = 1;
		totalFrames *= 8;
	}

	bitrate = (int)(WavpackGetAverageBitrate(wpc, TRUE) / 1000.0);

	floatingPoint = MODE_FLOAT & WavpackGetMode(wpc) && 127 == WavpackGetFloatNormExp(wpc);

	isLossy = !(WavpackGetMode(wpc) & MODE_LOSSLESS);

	[self willChangeValueForKey:@"properties"];
	[self didChangeValueForKey:@"properties"];

	return YES;
}
/*
- (int)fillBuffer:(void *)buf ofSize:(UInt32)size
{
    int numsamples;
    int n;
    void *sampleBuf = malloc(size*2);

    numsamples = size/(bitsPerSample/8)/channels;
    n = WavpackUnpackSamples(wpc, sampleBuf, numsamples);

    int i;
    for (i = 0; i < n*channels; i++)
    {
        ((UInt16 *)buf)[i] = ((UInt32 *)sampleBuf)[i];
    }

    n *= (bitsPerSample/8)*channels;

    free(sampleBuf);

    return n;
}
*/
- (AudioChunk *)readAudio {
	int32_t frames = 1024;
	if(worker_threads) {
		if(channels <= 2)
			frames = (worker_threads + 1) * 48000;
		else
			frames = 48000;

		while(frames * channels > 8388608 / sizeof(int32_t))
			frames >>= 1;
	}

	id audioChunkClass = NSClassFromString(@"AudioChunk");
	AudioChunk *chunk = [[audioChunkClass alloc] initWithProperties:[self properties]];

	uint32_t sample;
	int32_t audioSample;
	uint32_t samplesRead;
	int8_t *alias8;
	int16_t *alias16;
	int32_t *alias32;

	const size_t bufferSize = frames * [chunk format].mBytesPerFrame;
	void *buf = (void *)outputBuffer;
	if(!buf || outputBufferSize < bufferSize) {
		buf = realloc(outputBuffer, bufferSize);
		if(!buf) {
			return nil;
		}
		outputBuffer = (uint8_t *)buf;
	}

	size_t newSize = frames * sizeof(int32_t) * channels;
	if(!inputBuffer || newSize > inputBufferSize) {
		void *inp = realloc(inputBuffer, inputBufferSize = newSize);
		if(!inp) {
			return nil;
		}
		inputBuffer = (int32_t *)inp;
	}

	samplesRead = WavpackUnpackSamples(wpc, inputBuffer, frames);

	switch(bitsPerSample) {
		case 1:
		case 8:
			// No need for byte swapping
			alias8 = buf;
			for(sample = 0; sample < samplesRead * channels; ++sample) {
				*alias8++ = (int8_t)inputBuffer[sample];
			}
			break;
		case 16:
			// Convert to little endian byte order
			alias16 = buf;
			for(sample = 0; sample < samplesRead * channels; ++sample) {
				*alias16++ = OSSwapHostToLittleInt16((int16_t)inputBuffer[sample]);
			}
			break;
		case 24:
			// Convert to little endian byte order
			alias8 = buf;
			for(sample = 0; sample < samplesRead * channels; ++sample) {
				audioSample = inputBuffer[sample];
				*alias8++ = (int8_t)audioSample;
				*alias8++ = (int8_t)(audioSample >> 8);
				*alias8++ = (int8_t)(audioSample >> 16);
			}
			break;
		case 32:
			// Convert to little endian byte order
			alias32 = buf;
			for(sample = 0; sample < samplesRead * channels; ++sample) {
				*alias32++ = OSSwapHostToLittleInt32(inputBuffer[sample]);
			}
			break;
		default:
			ALog(@"Unsupported sample size: %d", bitsPerSample);
	}

	double DSDrate = (bitsPerSample == 1) ? 8.0 : 1.0;
	double streamTimestamp = (double)(frame) / frequency * DSDrate;
	frame += samplesRead;

	[chunk setStreamTimestamp:streamTimestamp];
	[chunk assignSamples:outputBuffer frameCount:samplesRead];

	return chunk;
}

- (long)seek:(long)frame {
	uint64_t trueFrame = (uint64_t)frame;
	if(isDSD) {
		trueFrame /= 8;
		frame = trueFrame * 8;
	}
	WavpackSeekSample64(wpc, trueFrame);

	self->frame = frame;

	return frame;
}

- (void)close {
	if(wpc) {
		WavpackCloseFile(wpc);
		wpc = NULL;
	}
	if(inputBuffer) {
		free(inputBuffer);
		inputBuffer = NULL;
		inputBufferSize = 0;
	}
	if(outputBuffer) {
		free(outputBuffer);
		outputBuffer = NULL;
		outputBufferSize = 0;
	}
	wvc = nil;
	wv = nil;
}

- (void)dealloc {
	[self close];
}

- (NSDictionary *)properties {
	return @{ @"channels": @(channels),
		      @"channelConfig": @(channelConfig),
		      @"bitsPerSample": @(bitsPerSample),
		      @"bitrate": @(bitrate),
		      @"sampleRate": @(frequency),
		      @"floatingPoint": @(floatingPoint),
		      @"totalFrames": @(totalFrames),
		      @"seekable": @([[wv source] seekable]),
		      @"codec": @"WavPack",
		      @"endian": @"little",
		      @"encoding": isLossy ? @"lossy" : @"lossless" };
}

- (NSDictionary *)metadata {
	return @{};
}

+ (NSArray *)fileTypes {
	return @[@"wv", @"wvp"];
}

+ (NSArray *)mimeTypes {
	return @[@"audio/x-wavpack"];
}

+ (float)priority {
	return 1.0;
}

+ (NSArray *)fileTypeAssociations {
	return @[
		@[@"Wavpack File", @"wv.icns", @"wv", @"wvp"]
	];
}

@end
