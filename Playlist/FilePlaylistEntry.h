//
//  FilePlaylistEntry.h
//  Cog
//
//  Created by Vincent Spader on 3/12/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "PlaylistEntry.h"


@interface FilePlaylistEntry : PlaylistEntry {
	FSRef fileRef;
}

@end
