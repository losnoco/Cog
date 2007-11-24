//
//  MonkeysFile.h
//  zyVorbis
//
//  Created by Vincent Spader on 1/30/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import "Plugin.h"

#import <Cocoa/Cocoa.h>
#import "MAC/All.h"
#import "MAC/MACLib.h"

#import "SourceIO.h"

@interface MonkeysAudioDecoder : NSObject <CogDecoder>
{
	IAPEDecompress *decompress;
	
	id<CogSource> source;
	SourceIO *sourceIO;
	
	int channels;
	int bitsPerSample;
	float frequency;
	long totalFrames;
}

- (void)setSource:(id<CogSource>)s;
- (id<CogSource>)source;

@end
