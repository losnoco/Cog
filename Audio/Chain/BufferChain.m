//
//  BufferChain.m
//  CogNew
//
//  Created by Vincent Spader on 1/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "BufferChain.h"
#import "OutputNode.h"
#import "AudioSource.h"
#import "CoreAudioUtils.h"

#import "Logging.h"

@implementation BufferChain

- (id)initWithController:(id)c
{
	self = [super init];
	if (self)
	{
		controller = c;
		streamURL = nil;
		userInfo = nil;
        rgInfo = nil;

		inputNode = nil;
		converterNode = nil;
	}
	
	return self;
}

- (void)buildChain
{
    inputNode = nil;
    converterNode = nil;
    
	inputNode = [[InputNode alloc] initWithController:self previous:nil];
	converterNode = [[ConverterNode alloc] initWithController:self previous:inputNode];
	
	finalNode = converterNode;
}

- (BOOL)open:(NSURL *)url withOutputFormat:(AudioStreamBasicDescription)outputFormat withRGInfo:(NSDictionary *)rgi
{	
	[self setStreamURL:url];

	[self buildChain];
	
	id<CogSource> source = [AudioSource audioSourceForURL:url];
	DLog(@"Opening: %@", url);
	if (!source || ![source open:url])
	{
		DLog(@"Couldn't open source...");
        url = [NSURL URLWithString:@"silence://10"];
        source = [AudioSource audioSourceForURL:url];
        if (![source open:url])
            return NO;
	}

	if (![inputNode openWithSource:source])
		return NO;

	if (![converterNode setupWithInputFormat:(inputFormat = propertiesToASBD([inputNode properties])) outputFormat:outputFormat])
		return NO;

    [self setRGInfo:rgi];

//		return NO;

	return YES;
}

- (BOOL)openWithInput:(InputNode *)i withOutputFormat:(AudioStreamBasicDescription)outputFormat withRGInfo:(NSDictionary *)rgi
{
	DLog(@"New buffer chain!");
	[self buildChain];

	if (![inputNode openWithDecoder:[i decoder]])
		return NO;
	
	DLog(@"Input Properties: %@", [inputNode properties]);
	if (![converterNode setupWithInputFormat:(inputFormat = propertiesToASBD([inputNode properties])) outputFormat:outputFormat])
		return NO;

    [self setRGInfo:rgi];
    
	return YES;
}

- (BOOL)openWithDecoder:(id<CogDecoder>)decoder
    withOutputFormat:(AudioStreamBasicDescription)outputFormat
    withRGInfo:(NSDictionary*)rgi;
{
    DLog(@"New buffer chain!");
    [self buildChain];
    
    if (![inputNode openWithDecoder:decoder])
        return NO;
    
    DLog(@"Input Properties: %@", [inputNode properties]);
    if (![converterNode setupWithInputFormat:(inputFormat = propertiesToASBD([inputNode properties])) outputFormat:outputFormat])
        return NO;
    
    [self setRGInfo:rgi];
    
    return YES;
}

- (void)launchThreads
{
	DLog(@"Properties: %@", [inputNode properties]);

	[inputNode launchThread];
	[converterNode launchThread];
}

- (void)setUserInfo:(id)i
{
	userInfo = i;
}

- (id)userInfo
{
	return userInfo;
}

- (void)setRGInfo:(NSDictionary *)rgi
{
    rgInfo = rgi;
    [converterNode setRGInfo:rgi];
}

- (NSDictionary *)rgInfo
{
    return rgInfo;
}

- (void)dealloc
{
    [inputNode setShouldContinue:NO];
    [[inputNode exitAtTheEndOfTheStream] signal];
    [[inputNode semaphore] signal];
    [[inputNode exitAtTheEndOfTheStream] wait]; // wait for decoder to be closed (see InputNode's -(void)process )

	DLog(@"Bufferchain dealloc");
}

- (void)seek:(double)time
{
	long frame = (long) round(time * [[[inputNode properties] objectForKey:@"sampleRate"] floatValue]);

	[inputNode seek:frame];
}

- (BOOL)endOfInputReached
{
	return [controller endOfInputReached:self];
}

- (BOOL)setTrack: (NSURL *)track
{
	return [inputNode setTrack:track];
}

- (void)initialBufferFilled:(id)sender
{
	DLog(@"INITIAL BUFFER FILLED");
	[controller launchOutputThread];
}

- (void)inputFormatDidChange:(AudioStreamBasicDescription)format
{
	DLog(@"FORMAT DID CHANGE!");
}


- (InputNode *)inputNode
{
	return inputNode;
}

- (id)finalNode
{
	return finalNode;
}

- (NSURL *)streamURL
{
	return streamURL;
}

- (void)setStreamURL:(NSURL *)url
{
	streamURL = url;
}

- (void)setShouldContinue:(BOOL)s
{
	[inputNode setShouldContinue:s];
	[converterNode setShouldContinue:s];
}

- (BOOL)isRunning
{
    InputNode *node = [self inputNode];
    if (nil != node && [node shouldContinue] && ![node endOfStream])
    {
        return YES;
    }
    return NO;
}

- (id)controller
{
    return controller;
}

- (ConverterNode *)converter
{
    return converterNode;
}

- (AudioStreamBasicDescription)inputFormat
{
    return inputFormat;
}

@end
