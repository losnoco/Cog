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
#import "DSPDownmixNode.h"
#import "DSPFaderNode.h"
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
	}

	return self;
}

- (BOOL)buildChain:(BOOL)resetBuffers {
	// Cut off output source
	finalNode = nil;

	// Tear them down in reverse
	converterNode = nil;
	inputNode = nil;

	inputNode = [[InputNode alloc] initWithController:self previous:nil];
	if(!inputNode) return NO;
	[inputNode setResetBuffers:resetBuffers];
	converterNode = [[ConverterNode alloc] initWithController:self previous:inputNode];
	if(!converterNode) return NO;

	finalNode = converterNode;

	return YES;
}

- (BOOL)open:(NSURL *)url withOutputFormat:(AudioStreamBasicDescription)outputFormat withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi {
	return [self open:url withOutputFormat:outputFormat withUserInfo:userInfo withRGInfo:rgi resetBuffers:NO];
}

- (BOOL)open:(NSURL *)url withOutputFormat:(AudioStreamBasicDescription)outputFormat withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi resetBuffers:(BOOL)resetBuffers {
	if(!url) {
		DLog(@"Player attempted to play invalid file...");
		return NO;
	}
	[self setStreamURL:url];
	[self setUserInfo:userInfo];

	if(![self buildChain:resetBuffers]) {
		DLog(@"Couldn't build processing chain...");
		return NO;
	}

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

	if(![self initConverter:outputFormat])
		return NO;
	[self initDownmixer];

	[self setRGInfo:rgi];

	//		return NO;

	return YES;
}

- (BOOL)openWithInput:(InputNode *)i withOutputFormat:(AudioStreamBasicDescription)outputFormat withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi {
	return [self openWithInput:i withOutputFormat:outputFormat withUserInfo:userInfo withRGInfo:rgi resetBuffers:NO];
}

- (BOOL)openWithInput:(InputNode *)i withOutputFormat:(AudioStreamBasicDescription)outputFormat withUserInfo:(id)userInfo withRGInfo:(NSDictionary *)rgi resetBuffers:(BOOL)resetBuffers {
	DLog(@"New buffer chain!");
	[self setUserInfo:userInfo];
	if(![self buildChain:resetBuffers]) {
		DLog(@"Couldn't build processing chain...");
		return NO;
	}

	if(![inputNode openWithDecoder:[i decoder]])
		return NO;

	if(![self initConverter:outputFormat])
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
	if(![self buildChain:NO]) {
		DLog(@"Couldn't build processing chain...");
		return NO;
	}

	if(![inputNode openWithDecoder:decoder])
		return NO;

	if(![self initConverter:outputFormat])
		return NO;
	[self initDownmixer];

	[self setRGInfo:rgi];

	return YES;
}

- (BOOL)initConverter:(AudioStreamBasicDescription)outputFormat {
	NSDictionary *properties = [inputNode properties];

	DLog(@"Input Properties: %@", properties);

	AudioStreamBasicDescription inputFormat = [inputNode nodeFormat];
	uint32_t inputChannelConfig = 0;
	if([properties valueForKey:@"channelConfig"])
		inputChannelConfig = [[properties valueForKey:@"channelConfig"] unsignedIntValue];

	outputFormat.mChannelsPerFrame = inputFormat.mChannelsPerFrame;
	outputFormat.mBytesPerFrame = ((outputFormat.mBitsPerChannel + 7) / 8) * outputFormat.mChannelsPerFrame;
	outputFormat.mBytesPerPacket = outputFormat.mBytesPerFrame * outputFormat.mFramesPerPacket;

	if(![converterNode setupWithInputFormat:inputFormat withInputConfig:inputChannelConfig outputFormat:outputFormat isLossless:[[properties valueForKey:@"encoding"] isEqualToString:@"lossless"]])
		return NO;

	return YES;
}

- (void)initDownmixer {
	AudioPlayer * audioPlayer = controller;
	OutputNode *outputNode = [audioPlayer output];
	DSPDownmixNode *downmixNode = [outputNode downmix];
	[downmixNode setOutputFormat:[outputNode deviceFormat] withChannelConfig:[outputNode deviceChannelConfig]];
}

- (void)launchThreads {
	DLog(@"Properties: %@", [inputNode properties]);

	[inputNode launchThread];
	[converterNode launchThread];
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

@end
