//
//  AudioMetadataWriter.m
//  CogAudio
//
//  Created by Safari on 08/11/18.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "AudioMetadataWriter.h"
#import "PluginController.h"

@implementation AudioMetadataWriter
+ (int)putMetadataInURL:(NSURL *)url
{
	return [[PluginController sharedPluginController] putMetadataInURL:url];
}
@end
