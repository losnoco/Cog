//
//  GameFile.h
//  Cog
//
//  Created by Zaphod Beeblebrox on 5/29/06.
//  Copyright 2006 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SoundFile.h"

#undef HAVE_CONFIG_H

#import "GME/Music_Emu.h"
@interface GameFile : SoundFile {
	Music_Emu* emu;
}

@end
