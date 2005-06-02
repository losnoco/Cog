//
//  MPEGFile.h
//  zyVorbis
//
//  Created by Vincent Spader on 1/30/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SoundFile.h"

#include "DecMPA/decmpa.h"

@interface MPEGFile : SoundFile {
	void *decoder;
}

@end
