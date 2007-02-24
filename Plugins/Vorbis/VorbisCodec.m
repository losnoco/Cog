//
//  MusepackCodec.m
//  MusepackCodec
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "VorbisCodec.h"
#import "VorbisDecoder.h"
#import "VorbisPropertiesReader.h"

@implementation VorbisCodec

- (int)pluginType
{
	return kCogPluginCodec;
}

- (Class)decoder
{
	return [VorbisDecoder class];
}

- (Class)metadataReader
{
	return nil;
}

- (Class)propertiesReader
{
	return [VorbisPropertiesReader class];
}




@end
