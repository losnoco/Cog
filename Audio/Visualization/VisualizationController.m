//
//  VisualizationController.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import "VisualizationController.h"
#import <Accelerate/Accelerate.h>

#import "fft.h"

@implementation VisualizationController

static VisualizationController *_sharedController = nil;

+ (VisualizationController *)sharedController {
	@synchronized(self) {
		if(!_sharedController) {
			_sharedController = [[VisualizationController alloc] init];
		}
	}
	return _sharedController;
}

- (id)init {
	self = [super init];
	if(self) {
		visAudio = NULL;
		latency = 0;
		visAudioTemp = (float *) calloc(sizeof(float), 4096);
		if(!visAudioTemp) {
			return nil;
		}
	}
	return self;
}

- (void)dealloc {
	fft_free();
}

- (void)postSampleRate:(double)sampleRate {
	@synchronized(self) {
		if(self->sampleRate != sampleRate) {
			self->sampleRate = sampleRate;
			int visAudioSize = (int)(sampleRate * 45.0);
			void *visAudio = realloc(self->visAudio, visAudioSize * sizeof(float));
			if(visAudio && visAudioSize) {
				if(visAudioSize > self->visAudioSize) {
					bzero(((float *)visAudio) + self->visAudioSize, sizeof(float) * (visAudioSize - self->visAudioSize));
				}
				self->visAudio = visAudio;
				self->visAudioSize = visAudioSize;
				visAudioCursor %= visAudioSize;
			}
		}
	}
}

- (void)postVisPCM:(const float *)inPCM amount:(int)amount {
	@synchronized(self) {
		int samplesRead = 0;
		while(amount > 0) {
			int amountToCopy = (int)(visAudioSize - visAudioCursor);
			if(amountToCopy > amount) amountToCopy = amount;
			cblas_scopy(amountToCopy, inPCM + samplesRead, 1, visAudio + visAudioCursor, 1);
			visAudioCursor = visAudioCursor + amountToCopy;
			if(visAudioCursor >= visAudioSize) visAudioCursor -= visAudioSize;
			amount -= amountToCopy;
			samplesRead += amountToCopy;
		}
	}
}

- (void)postLatency:(double)latency {
	self->latency = latency;
	assert(latency < 45.0);
}

- (double)readSampleRate {
	@synchronized(self) {
		return sampleRate;
	}
}

- (void)copyVisPCM:(float *)outPCM visFFT:(float *)outFFT latencyOffset:(double)latency {
	if(!outPCM && !outFFT) return;

	if(!visAudio || !visAudioSize) {
		if(outPCM) bzero(outPCM, sizeof(float) * 4096);
		if(outFFT) bzero(outFFT, sizeof(float) * 2048);
		return;
	}

	if(!outPCM) outPCM = &visAudioTemp[0];

	@synchronized(self) {
		if(!sampleRate) {
			bzero(outPCM, 4096 * sizeof(float));
			if(outFFT) {
				bzero(outFFT, 2048 * sizeof(float));
			}
			return;
		}
		int latencySamples = (int)(sampleRate * (self->latency + latency));
		if(latencySamples < 4096) latencySamples = 4096;
		int readCursor = visAudioCursor - latencySamples;
		int samples = 4096;
		int samplesRead = 0;
		while(readCursor < 0)
			readCursor += visAudioSize;
		while(readCursor >= visAudioSize)
			readCursor -= visAudioSize;
		while(samples > 0) {
			int samplesToRead = (int)(visAudioSize - readCursor);
			if(samplesToRead > samples) samplesToRead = samples;
			cblas_scopy(samplesToRead, visAudio + readCursor, 1, outPCM + samplesRead, 1);
			samplesRead += samplesToRead;
			readCursor += samplesToRead;
			samples -= samplesToRead;
			if(readCursor >= visAudioSize) readCursor -= visAudioSize;
		}
	}
	if(outFFT) {
		fft_calculate(outPCM, outFFT, 2048);
	}
}

@end
