//
//  SpotlightSearchController.h
//  Cog
//
//  Created by Matthew Grinshpun on 10/02/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class SpotlightWindowController;

@interface SpotlightSearchController : NSObject {
    IBOutlet NSArrayController *playlistController;
    IBOutlet SpotlightWindowController *spotlightWindowController;
    IBOutlet NSPathControl *pathControl;
    NSMetadataQuery *query;
    NSString *searchString;
}

- (IBAction)addToPlaylist:(id)sender;
- (IBAction)changeSearchPath:(id)sender;

- (void)performSearch;
- (NSPredicate *)processSearchString;

@property(retain) NSMetadataQuery *query;
@property(copy) NSString *searchString;

@end
