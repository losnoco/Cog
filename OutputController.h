//
//  OutputController.h
//  Cog
//
//  Created by Zaphod Beeblebrox on 8/2/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface OutputController : NSObject {
	VirtualRingBuffer *buffer;
	AudioStreamBasicDescription format;
}

@end
