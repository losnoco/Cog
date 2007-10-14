//
//  CueSheetPlugin.m
//  CueSheet
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "CueSheetPlugin.h"

#import "CueSheetContainer.h"
#import "CueSheetDecoder.h"
#import "CueSheetMetadataReader.h"

@implementation CueSheetPlugin

+ (NSDictionary *)pluginInfo
{
	return [NSDictionary dictionaryWithObjectsAndKeys:
		kCogContainer, [CueSheetContainer className],
		kCogDecoder, [CueSheetDecoder className],
		kCogMetadataReader, [CueSheetMetadataReader className],
		nil];
}

@end
