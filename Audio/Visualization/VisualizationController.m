//
//  VisualizationController.m
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 2/12/22.
//

#import <CogAudio/VisualizationController.h>

#import <Accelerate/Accelerate.h>

#import "fft.h"

@implementation VisualizationController {
	MIDIVisualizationController *midiController;
	NSTimer *sineTimer;

	double sampleRate;
	double latency;
	double fullLatency;
	float *visAudio;
	int visAudioCursor, visAudioSize;
	uint64_t visSamplesPosted;
	BOOL ignoreLatency;
	double sinePhase;
}

+ (VisualizationController *)sharedController {
	static VisualizationController *_sharedController = nil;
	static dispatch_once_t onceToken;
	dispatch_once(&onceToken, ^{
		_sharedController = [[VisualizationController alloc] init];
	});
	return _sharedController;
}

+ (MIDIVisualizationController *)sharedMIDIController {
	return [[VisualizationController sharedController] midiController];
}

- (MIDIVisualizationController *)midiController {
	return midiController;
}

- (id)init {
	self = [super init];
	if(self) {
		midiController = [[MIDIVisualizationController alloc] initWithController:self];

		visAudio = NULL;
		visAudioSize = 0;
		latency = 0;
		ignoreLatency = YES;
		sinePhase = 0.0;

		sineTimer = [NSTimer timerWithTimeInterval:1.0 / 60.0 repeats:YES block:^(NSTimer * _Nonnull timer) {
			double sinePhase = self->sinePhase;
			self->sinePhase = fmod(sinePhase + (M_PI / 90.0), M_PI * 2.0);
		}];
		[[NSRunLoop mainRunLoop] addTimer:sineTimer forMode:NSRunLoopCommonModes];
	}
	return self;
}

- (void)dealloc {
	fft_free();
	[sineTimer invalidate];
	sineTimer = nil;
}

- (void)reset {
	@synchronized (self) {
		latency = 0;
		visAudioCursor = 0;
		visSamplesPosted = 0;
		ignoreLatency = YES;
		sinePhase = 0.0;
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
	if((latency >= 45.0) || (latency < 0.0)) [[clang::unlikely]] {
		ignoreLatency = YES;
		self->latency = latency;
	} else {
		self->latency = latency;
		ignoreLatency = NO;
	}
}

- (double)getLatency {
	if(!ignoreLatency) return latency;
	else return 0.0;
}

- (void)postFullLatency:(double)latency {
	self->fullLatency = latency;
}

- (double)getFullLatency {
	if(!ignoreLatency) return fullLatency;
	else return 0.0;
}

- (double)readSampleRate {
	@synchronized(self) {
		return sampleRate ?: 44100.0;
	}
}

- (UInt64)samplesPosted {
	return visSamplesPosted;
}

- (void)generateSineWave:(float *_Nullable)outPCM visFFT:(float *_Nullable)outFFT {
	double sinePhase = self->sinePhase;
	if(outPCM || outFFT) {
		const double stepSize = M_PI * 2.0 * 5.0 / 4096.0;
		double sineStep = sinePhase;
		for(int i = 0; i < 2048; ++i) {
			double sinePoint = sin(sineStep);
			if(outPCM) {
				outPCM[i * 2] = sinePoint;
				outPCM[i * 2 + 1] = sin(sineStep + stepSize);
			}
			if(outFFT) {
				outFFT[i] = sinePoint * -40.0 - 40.0;
			}
			sineStep = fmod(sineStep + stepSize * 2, M_PI * 2.0);
		}
	}
}

- (void)copyVisPCM:(float *_Nullable)outPCM visFFT:(float *_Nullable)outFFT latencyOffset:(double)latency {
	if(!outPCM && !outFFT) return;

	if(ignoreLatency || !visAudio || !visAudioSize) {
		[self generateSineWave:outPCM visFFT:outFFT];
		return;
	}

	void *visAudioTemp = calloc(sizeof(float), 4096);
	if(!visAudioTemp) {
		[self generateSineWave:outPCM visFFT:outFFT];
		return;
	}

	@synchronized(self) {
		if(!sampleRate) {
			free(visAudioTemp);
			[self generateSineWave:outPCM visFFT:outFFT];
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
