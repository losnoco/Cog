//
//  BufferChain.m
//  CogNew
//
//  Created by Vincent Spader on 1/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "BufferChain.h"
#import "AudioSource.h"
#import "CoreAudioUtils.h"
#import "OutputNode.h"

#import "AudioPlayer.h"

#import "Logging.h"

@implementation BufferChain

- (id)initWithController:(id)c {
	self = [super init];
	if(self) {
		controller = c;
		streamURL = nil;
		userInfo = nil;
		rgInfo = nil;

		inputNode = nil;
		converterNode = nil;

		rubberbandNode = nil;
		fsurroundNode = nil;
		equalizerNode = nil;
		hrtfNode = nil;
		downmixNode = nil;

		visualizationNode = nil;
	}

	return self;
}

- (void)buildChain {
	inputNode = nil;
	converterNode = nil;

	inputNode = [[InputNode alloc] initWithController:self previous:nil];
	converterNode = [[ConverterNode alloc] initWithController:self previous:inputNode];
	rubberbandNode = [[DSPRubberbandNode alloc] initWithController:self previous:converterNode latency:0.1];
	fsurroundNode = [[DSPFSurroundNode alloc] initWithController:self previous:rubberbandNode latency:0.03];
	equalizerNode = [[DSPEqualizerNode alloc] initWithController:self previous:fsurroundNode latency:0.03];
	hrtfNode = [[DSPHRTFNode alloc] initWithController:self previous:equalizerNode latency:0.03];
	downmixNode = [[DSPDownmixNode alloc] initWithController:self previous:hrtfNode latency:0.03];

	// Approximately five frames
	visualizationNode = [[VisualizationNode alloc] initWithController:self previous:downmixNode latency:5.0 / 60.0];

	finalNode = visualizationNode;
}

- (BOOL)open:(NSURL *)url withOutputFormat:(AudioStreamBasicDescription)outputFormat withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi {
	[self setStreamURL:url];
	[self setUserInfo:userInfo];

	[self buildChain];

	id<CogSource> source = [AudioSource audioSourceForURL:url];
	DLog(@"Opening: %@", url);
	if(!source || ![source open:url]) {
		DLog(@"Couldn't open source...");
		url = [NSURL URLWithString:@"silence://10"];
		source = [AudioSource audioSourceForURL:url];
		if(![source open:url])
			return NO;
	}

	if(![inputNode openWithSource:source])
		return NO;

	NSDictionary *properties = [inputNode properties];

	AudioStreamBasicDescription inputFormat = [inputNode nodeFormat];
	uint32_t inputChannelConfig = 0;
	if([properties valueForKey:@"channelConfig"])
		inputChannelConfig = [[properties valueForKey:@"channelConfig"] unsignedIntValue];

	outputFormat.mChannelsPerFrame = inputFormat.mChannelsPerFrame;
	outputFormat.mBytesPerFrame = ((outputFormat.mBitsPerChannel + 7) / 8) * outputFormat.mChannelsPerFrame;
	outputFormat.mBytesPerPacket = outputFormat.mBytesPerFrame * outputFormat.mFramesPerPacket;

	if(![converterNode setupWithInputFormat:inputFormat withInputConfig:inputChannelConfig outputFormat:outputFormat isLossless:[[properties valueForKey:@"encoding"] isEqualToString:@"lossless"]])
		return NO;
	[self initDownmixer];

	[self setRGInfo:rgi];

	//		return NO;

	return YES;
}

- (BOOL)openWithInput:(InputNode *)i withOutputFormat:(AudioStreamBasicDescription)outputFormat withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi {
	DLog(@"New buffer chain!");
	[self setUserInfo:userInfo];
	[self buildChain];

	if(![inputNode openWithDecoder:[i decoder]])
		return NO;

	NSDictionary *properties = [inputNode properties];

	AudioStreamBasicDescription inputFormat = [inputNode nodeFormat];
	uint32_t inputChannelConfig = 0;
	if([properties valueForKey:@"channelConfig"])
		inputChannelConfig = [[properties valueForKey:@"channelConfig"] unsignedIntValue];

	outputFormat.mChannelsPerFrame = inputFormat.mChannelsPerFrame;
	outputFormat.mBytesPerFrame = ((outputFormat.mBitsPerChannel + 7) / 8) * outputFormat.mChannelsPerFrame;
	outputFormat.mBytesPerPacket = outputFormat.mBytesPerFrame * outputFormat.mFramesPerPacket;

	DLog(@"Input Properties: %@", properties);
	if(![converterNode setupWithInputFormat:inputFormat withInputConfig:inputChannelConfig outputFormat:outputFormat isLossless:[[properties objectForKey:@"encoding"] isEqualToString:@"lossless"]])
		return NO;
	[self initDownmixer];

	[self setRGInfo:rgi];

	return YES;
}

- (BOOL)openWithDecoder:(id<CogDecoder>)decoder
	   withOutputFormat:(AudioStreamBasicDescription)outputFormat
           withUserInfo:(id)userInfo
             withRGInfo:(NSDictionary *)rgi;
{
	DLog(@"New buffer chain!");
	[self setUserInfo:userInfo];
	[self buildChain];

	if(![inputNode openWithDecoder:decoder])
		return NO;

	NSDictionary *properties = [inputNode properties];

	DLog(@"Input Properties: %@", properties);

	AudioStreamBasicDescription inputFormat = [inputNode nodeFormat];
	uint32_t inputChannelConfig = 0;
	if([properties valueForKey:@"channelConfig"])
		inputChannelConfig = [[properties valueForKey:@"channelConfig"] unsignedIntValue];

	outputFormat.mChannelsPerFrame = inputFormat.mChannelsPerFrame;
	outputFormat.mBytesPerFrame = ((outputFormat.mBitsPerChannel + 7) / 8) * outputFormat.mChannelsPerFrame;
	outputFormat.mBytesPerPacket = outputFormat.mBytesPerFrame * outputFormat.mFramesPerPacket;

	if(![converterNode setupWithInputFormat:inputFormat withInputConfig:inputChannelConfig outputFormat:outputFormat isLossless:[[properties objectForKey:@"encoding"] isEqualToString:@"lossless"]])
		return NO;
	[self initDownmixer];

	[self setRGInfo:rgi];

	return YES;
}

- (void)initDownmixer {
	AudioPlayer * audioPlayer = controller;
	OutputNode *outputNode = [audioPlayer output];
	[downmixNode setOutputFormat:[outputNode deviceFormat] withChannelConfig:[outputNode deviceChannelConfig]];
}

- (void)launchThreads {
	DLog(@"Properties: %@", [inputNode properties]);

	[inputNode launchThread];
	[converterNode launchThread];
	[rubberbandNode launchThread];
	[fsurroundNode launchThread];
	[equalizerNode launchThread];
	[hrtfNode launchThread];
	[downmixNode launchThread];
	[visualizationNode launchThread];
}

- (void)setUserInfo:(id)i {
	userInfo = i;
}

- (id)userInfo {
	return userInfo;
}

- (void)setRGInfo:(NSDictionary *)rgi {
	rgInfo = rgi;
	[converterNode setRGInfo:rgi];
}

- (NSDictionary *)rgInfo {
	return rgInfo;
}

- (void)dealloc {
	[inputNode setShouldContinue:NO];
	[[inputNode exitAtTheEndOfTheStream] signal];
	[[inputNode writeSemaphore] signal];
	if(![inputNode threadExited])
		[[inputNode exitAtTheEndOfTheStream] wait]; // wait for decoder to be closed (see InputNode's -(void)process )

	// Must do this here, or else the VisualizationContainer will carry a reference forever
	[visualizationNode pop];

	DLog(@"Bufferchain dealloc");
}

- (void)seek:(double)time {
	long frame = (long)round(time * [[[inputNode properties] objectForKey:@"sampleRate"] floatValue]);

	[inputNode seek:frame];
}

- (BOOL)endOfInputReached {
	return [controller endOfInputReached:self];
}

- (BOOL)setTrack:(NSURL *)track {
	return [inputNode setTrack:track];
}

- (void)initialBufferFilled:(id)sender {
	DLog(@"INITIAL BUFFER FILLED");
	[controller launchOutputThread];
}

- (InputNode *)inputNode {
	return inputNode;
}

- (id)finalNode {
	return finalNode;
}

- (NSURL *)streamURL {
	return streamURL;
}

- (void)setStreamURL:(NSURL *)url {
	streamURL = url;
}

- (void)setShouldContinue:(BOOL)s {
	[inputNode setShouldContinue:s];
	[converterNode setShouldContinue:s];
	[rubberbandNode setShouldContinue:s];
	[fsurroundNode setShouldContinue:s];
	[equalizerNode setShouldContinue:s];
	[hrtfNode setShouldContinue:s];
	[downmixNode setShouldContinue:s];
	[visualizationNode setShouldContinue:s];
}

- (BOOL)isRunning {
	InputNode *node = [self inputNode];
	if(nil != node && [node shouldContinue] && ![node endOfStream]) {
		return YES;
	}
	return NO;
}

- (id)controller {
	return controller;
}

- (ConverterNode *)converter {
	return converterNode;
}

- (DSPRubberbandNode *)rubberband {
	return rubberbandNode;
}

- (DSPFSurroundNode *)fsurround {
	return fsurroundNode;
}

- (DSPHRTFNode *)hrtf {
	return hrtfNode;
}

- (DSPEqualizerNode *)equalizer {
	return equalizerNode;
}

- (DSPDownmixNode *)downmix {
	return downmixNode;
}

- (VisualizationNode *)visualization {
	return visualizationNode;
}

- (AudioStreamBasicDescription)inputFormat {
	return [inputNode nodeFormat];
}

- (uint32_t)inputConfig {
	return [inputNode nodeChannelConfig];
}

- (double)secondsBuffered {
	double duration = 0.0;
	Node *node = [self finalNode];
	while(node) {
		duration += [node secondsBuffered];
		node = [node previousNode];
	}
	return duration;
}

- (void)sustainHDCD {
	OutputNode *outputNode = (OutputNode *)[controller output];
	[outputNode sustainHDCD];
	[controller sustainHDCD];
}

- (void)restartPlaybackAtCurrentPosition {
	[controller restartPlaybackAtCurrentPosition];
}

- (void)pushInfo:(NSDictionary *)info {
	[controller pushInfo:info toTrack:userInfo];
}

- (void)setError:(BOOL)status {
	[controller setError:status];
}

- (double)getPostVisLatency {
	double latency = 0.0;
	Node *node = finalNode;
	while(node) {
		latency += [node secondsBuffered];
		if(node == visualizationNode) {
			break;
		}
		node = [node previousNode];
	}
	return latency;
}

@end
