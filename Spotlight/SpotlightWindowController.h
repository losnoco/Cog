//
//  SpotlightWindowController.h
//  Cog
//
//  Created by Matthew Grinshpun on 10/02/08.
//  Copyright 2008 Matthew Leon Grinshpun. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class PlaylistLoader;
@protocol NSMetadataQueryDelegate;

@interface SpotlightWindowController : NSWindowController<NSMetadataQueryDelegate> {
    IBOutlet PlaylistLoader *playlistLoader;
    IBOutlet NSArrayController *playlistController;
    IBOutlet NSPathControl *pathControl;
    IBOutlet NSSearchField *searchField;
    NSMetadataQuery *query;
    NSString *searchString;
}

- (IBAction)addToPlaylist:(id)sender;
- (IBAction)toggleWindow:(id)sender;

- (IBAction)pathComponentClicked:(id)sender;

- (void)performSearch;
- (NSPredicate *)processSearchString;

- (void)searchForArtist:(NSString *)artist;
- (void)searchForAlbum:(NSString *)album;

@property(retain) NSMetadataQuery *query;
@property(copy, nonatomic) NSString *searchString;

@end
