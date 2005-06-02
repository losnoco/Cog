//
//  MonkeysFile.h
//  zyVorbis
//
//  Created by Vincent Spader on 1/30/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <MAC/All.h>
#import <MAC/MACLib.h>

#import "SoundFile.h"

@interface MonkeysFile : SoundFile {
	IAPEDecompress * decompress;
}

@end
