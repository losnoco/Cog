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

#import "SourceIO.h"

@interface MonkeysAudioDecoder : NSObject <CogDecoder>
{
	IAPEDecompress *decompress;
	
	id<CogSource> source;
	SourceIO *sourceIO;
	
	int channels;
	int bitsPerSample;
	float frequency;
	double length;
}

- (void)setSource:(id<CogSource>)s;
- (id<CogSource>)source;

@end
