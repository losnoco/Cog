//
//  MusepackCodec.m
//  MusepackCodec
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "WavPackCodec.h"
#import "WavPackDecoder.h"
#import "WavPackPropertiesReader.h"

@implementation WavPackCodec

- (int)pluginType
{
	return kCogPluginCodec;
}

- (Class)decoder
{
	return [WavPackDecoder class];
}

- (Class)metadataReader
{
	return nil;
}

- (Class)propertiesReader
{
	return [WavPackPropertiesReader class];
}




@end
