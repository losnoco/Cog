//
//  MusepackCodec.m
//  MusepackCodec
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "FlacCodec.h"
#import "FlacDecoder.h"
#import "FlacPropertiesReader.h"

@implementation FlacCodec

- (int)pluginType
{
	return kCogPluginCodec;
}

- (Class)decoder
{
	return [FlacDecoder class];
}

- (Class)metadataReader
{
	return nil;
}

- (Class)propertiesReader
{
	return [FlacPropertiesReader class];
}




@end
