//
//  MusepackCodec.m
//  MusepackCodec
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "MADCodec.h"
#import "MADDecoder.h"
#import "MADPropertiesReader.h"

@implementation MADCodec

- (int)pluginType
{
	return kCogPluginCodec;
}

- (Class)decoder
{
	return [MADDecoder class];
}

- (Class)metadataReader
{
	return nil;
}

- (Class)propertiesReader
{
	return [MADPropertiesReader class];
}




@end
