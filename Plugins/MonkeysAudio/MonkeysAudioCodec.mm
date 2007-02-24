//
//  MusepackCodec.m
//  MusepackCodec
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "MonkeysAudioCodec.h"
#import "MonkeysAudioDecoder.h"
#import "MonkeysAudioPropertiesReader.h"

@implementation MonkeysAudioCodec

- (PluginType)pluginType
{
	return kCogPluginCodec;
}

- (Class)decoder
{
	return [MonkeysAudioDecoder class];
}

- (Class)metadataReader
{
	return nil;
}

- (Class)propertiesReader
{
	return [MonkeysAudioPropertiesReader class];
}




@end
