//
//  GameFile.h
//  Cog
//
//  Created by Vincent Spader on 5/29/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SoundFile.h"

#undef HAVE_CONFIG_H

#import "GME/Music_Emu.h"
@interface GameFile : SoundFile {
	Music_Emu* emu;
}

@end
