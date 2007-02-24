//
//  CoreAudio.m
//  CoreAudio
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "CoreAudioCodec.h"
#import "CoreAudioDecoder.h"
#import "CoreAudioPropertiesReader.h"

@implementation CoreAudioCodec

- (int)pluginType
{
	return kCogPluginCodec;
}

- (Class)decoder
{
	return [CoreAudioDecoder class];
}

- (Class)metadataReader
{
	return nil;
}

- (Class)propertiesReader
{
	return [CoreAudioPropertiesReader class];
}





@end
