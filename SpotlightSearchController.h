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
    NSMetadataQuery *query;
    NSString *searchString;
}

- (IBAction)addToPlaylist:(id)sender;

- (void)performSearch;

@property(retain) NSMetadataQuery *query;
@property(copy) NSString *searchString;

@end
