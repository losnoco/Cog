//
//  AODecoder.h
//  AudioOverload
//
//  Created by Vincent Spader on 2/28/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "Plugin.h"

@interface AODecoder : NSObject<CogDecoder,CogMetadataReader> {
	uint8_t *buffer;
	int type;
	
	BOOL closed;
	long long totalFrames;
	long long framesRead;
}

@end
