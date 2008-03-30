//
//  ShnFile.h
//  Cog
//
//  Created by Vincent Spader on 6/6/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <Shorten/shn_reader.h>

#import "Plugin.h"

@interface ShortenDecoder : NSObject <CogDecoder>
{
	//shn_file *handle;
	shn_reader *decoder;
	
	long bufferSize; //total size
	
	int channels;
	int bitsPerSample;
	float frequency;
	long totalFrames;
	BOOL seekable;
}

@end
