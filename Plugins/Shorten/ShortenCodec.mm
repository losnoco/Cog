//
//  MusepackCodec.m
//  MusepackCodec
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "ShortenCodec.h"
#import "ShortenDecoder.h"
#import "ShortenPropertiesReader.h"

@implementation ShortenCodec

- (PluginType)pluginType
{
	return kCogPluginCodec;
}

- (Class)decoder
{
	return [ShortenDecoder class];
}

- (Class)metadataReader
{
	return nil;
}

- (Class)propertiesReader
{
	return [ShortenPropertiesReader class];
}




@end
