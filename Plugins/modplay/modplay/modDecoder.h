//
//  modDecoder.h
//  modplay
//
//  Created by Christopher Snowhill on 03/17/14.
//  Copyright 2014 __NoWork, Inc__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <modplay/st3play.h>

#import <modplay/ft2play.h>

#import "unmo3.h"

#import "umx.h"

#import "Plugin.h"

enum {
	TYPE_UNKNOWN = 0,
	TYPE_S3M = 1,
	TYPE_XM = 2
};

@interface modDecoder : NSObject <CogDecoder> {
	uint32_t type;
	void *player;
	void *data;
	long size;
	int dataWasMo3;
	int track_num;

	long framesLength;
	long totalFrames;
	long framesRead;
}
@end
