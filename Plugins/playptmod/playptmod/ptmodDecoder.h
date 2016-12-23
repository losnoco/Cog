//
//  ptmodDecoder.h
//  playptmod
//
//  Created by Christopher Snowhill on 10/22/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import <playptmod/playptmod.h>

#import "Plugin.h"

@interface ptmodDecoder : NSObject <CogDecoder> {
	void *ptmod;
    void *data;
    long size;
    int track_num;
    
    int isMo3;
    int isVblank;
    
	long framesLength;
    long totalFrames;
    long framesRead;
}
@end
