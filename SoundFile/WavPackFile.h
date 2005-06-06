//
//  WavPackFile.h
//  Cog
//
//  Created by Zaphod Beeblebrox on 6/6/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SoundFile.h"

#import "Wavpack/wputils.h"

@interface WavPackFile : SoundFile {
	WavpackContext *wpc;
}

@end
