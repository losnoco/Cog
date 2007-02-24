//
//  MonkeysFile.h
//  zyVorbis
//
//  Created by Vincent Spader on 1/30/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "MAC/All.h"
#import "MAC/MACLib.h"

#import "Plugin.h"

@interface MonkeysAudioDecoder : NSObject <CogDecoder>
{
	IAPEDecompress * decompress;
	
	int channels;
	int bitsPerSample;
	float frequency;
	double length;
}

@end
