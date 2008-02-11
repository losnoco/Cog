//
//  SpotlightWindowController.h
//  Cog
//
//  Created by Matthew Grinshpun on 10/02/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class PlaylistLoader;

@interface SpotlightWindowController : NSWindowController {
    IBOutlet PlaylistLoader *playlistLoader;
}

@property(retain) PlaylistLoader *playlistLoader;

@end
