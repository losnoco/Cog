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
    IBOutlet NSArrayController *playlistController;
    IBOutlet NSPathControl *pathControl;
    NSMetadataQuery *query;
    NSString *searchString;
    NSString *spotlightSearchPath;
}

- (IBAction)addToPlaylist:(id)sender;

- (void)performSearch;
- (NSPredicate *)processSearchString;

@property(retain) NSMetadataQuery *query;
@property(copy) NSString *searchString;
@property(copy) NSString *spotlightSearchPath;

@end
