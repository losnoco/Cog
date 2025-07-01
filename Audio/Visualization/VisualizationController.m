//
//  VisualizationController.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import "VisualizationController.h"
#import <Accelerate/Accelerate.h>

#import "fft.h"

@implementation VisualizationController {
	double sampleRate;
	double latency;
	float *visAudio;
	int visAudioCursor, visAudioSize;
	uint64_t visSamplesPosted;
	BOOL ignoreLatency;
}

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
		visAudioSize = 0;
		latency = 0;
		ignoreLatency = YES;
	}
	return self;
}

- (void)dealloc {
	fft_free();
}

- (void)reset {
	@synchronized (self) {
		latency = 0;
		visAudioCursor = 0;
		visSamplesPosted = 0;
		ignoreLatency = YES;
		if(visAudio && visAudioSize) {
			bzero(visAudio, sizeof(float) * visAudioSize);
		}
	}
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
			} else {
				if(self->visAudio) {
					free(self->visAudio);
					self->visAudio = NULL;
				}
				self->visAudioSize = 0;
			}
		}
	}
}

- (void)postVisPCM:(const float *)inPCM amount:(int)amount {
	@synchronized(self) {
		if(!visAudioSize) {
			return;
		}
		int samplesRead = 0;
		while(amount > 0) {
			int amountToCopy = (int)(visAudioSize - visAudioCursor);
			if(amountToCopy > amount) amountToCopy = amount;
			cblas_scopy(amountToCopy, inPCM + samplesRead, 1, visAudio + visAudioCursor, 1);
			visAudioCursor = visAudioCursor + amountToCopy;
			if(visAudioCursor >= visAudioSize) visAudioCursor -= visAudioSize;
			amount -= amountToCopy;
			samplesRead += amountToCopy;
			visSamplesPosted += amountToCopy;
		}
	}
}

- (void)postLatency:(double)latency {
	ignoreLatency = (latency >= 45.0) || (latency < 0.0);
	self->latency = latency;
}

- (double)readSampleRate {
	@synchronized(self) {
		return sampleRate;
	}
}

- (UInt64)samplesPosted {
	return visSamplesPosted;
}

- (void)copyVisPCM:(float *_Nullable)outPCM visFFT:(float *_Nullable)outFFT latencyOffset:(double)latency {
	if(!outPCM && !outFFT) return;

	if(ignoreLatency || !visAudio || !visAudioSize) {
		if(outPCM) bzero(outPCM, sizeof(float) * 4096);
		if(outFFT) bzero(outFFT, sizeof(float) * 2048);
		return;
	}

	void *visAudioTemp = calloc(sizeof(float), 4096);
	if(!visAudioTemp) {
		if(outPCM) bzero(outPCM, sizeof(float) * 4096);
		if(outFFT) bzero(outFFT, sizeof(float) * 2048);
		return;
	}

	@synchronized(self) {
		if(!sampleRate) {
			free(visAudioTemp);
			if(outPCM) bzero(outPCM, 4096 * sizeof(float));
			if(outFFT) bzero(outFFT, 2048 * sizeof(float));
			return;
		}
		int latencySamples = (int)(sampleRate * (self->latency + latency)) + 2048;
		if(latencySamples < 4096) latencySamples = 4096;
		int readCursor = visAudioCursor - latencySamples;
		int samples = 4096;
		int samplesRead = 0;
		if(latencySamples + samples > visAudioSize) {
			samples = (int)(visAudioSize - latencySamples);
		}
		while(readCursor < 0)
			readCursor += visAudioSize;
		while(readCursor >= visAudioSize)
			readCursor -= visAudioSize;
		while(samples > 0) {
			int samplesToRead = (int)(visAudioSize - readCursor);
			if(samplesToRead > samples) samplesToRead = samples;
			cblas_scopy(samplesToRead, visAudio + readCursor, 1, visAudioTemp + samplesRead, 1);
			samplesRead += samplesToRead;
			readCursor += samplesToRead;
			samples -= samplesToRead;
			if(readCursor >= visAudioSize) readCursor -= visAudioSize;
		}
	}
	if(outPCM) {
		cblas_scopy(4096, visAudioTemp, 1, outPCM, 1);
	}
	if(outFFT) {
		fft_calculate(visAudioTemp, outFFT, 2048);
	}

	free(visAudioTemp);
}

@end
