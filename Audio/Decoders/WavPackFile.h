//
//  WavPackFile.h
//  Cog
//
//  Created by Vincent Spader on 6/6/05.
//  Copyright 2005 Vincent Spader All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SoundFile.h"

#import "Wavpack/wputils.h"

@interface WavPackFile : SoundFile {
	WavpackContext *wpc;
}

@end
