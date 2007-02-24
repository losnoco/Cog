//
//  MusepackCodec.m
//  MusepackCodec
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "TagLibPlugin.h"
#import "TagLibMetadataReader.h"

@implementation TagLibPlugin

- (int)pluginType
{
	return kCogPluginCodec;
}

- (Class)decoder
{
	return nil;
}

- (Class)metadataReader
{
	return [TagLibMetadataReader class];
}

- (Class)propertiesReader
{
	return nil;
}




@end
