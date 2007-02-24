//
//  MusepackCodec.m
//  MusepackCodec
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "MusepackCodec.h"
#import "MusepackDecoder.h"
#import "MusepackPropertiesReader.h"

@implementation MusepackCodec

- (int)pluginType
{
	return kCogPluginCodec;
}

- (Class)decoder
{
	return [MusepackDecoder class];
}

- (Class)metadataReader
{
	return nil;
}

- (Class)propertiesReader
{
	return [MusepackPropertiesReader class];
}




@end
