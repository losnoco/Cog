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

/// The Spotlight panel: a SwiftUI window (SpotlightPanelView) around the
/// results table, which SpotlightPanel.xib keeps in AppKit with its array
/// controller. This runs the query that fills it.
@interface SpotlightWindowController : NSWindowController <NSMetadataQueryDelegate> {
	IBOutlet PlaylistLoader *playlistLoader;
	IBOutlet NSArrayController *playlistController;
	NSMetadataQuery *query;
	NSString *searchString;
}

- (IBAction)addToPlaylist:(id _Nullable)sender;
- (IBAction)toggleWindow:(id _Nullable)sender;

- (void)performSearch;
- (NSPredicate *_Nullable)processSearchString;

- (void)searchForArtist:(NSString *_Nonnull)artist;
- (void)searchForAlbum:(NSString *_Nonnull)album;

@property(retain) NSMetadataQuery *_Nonnull query;
@property(copy, nonatomic) NSString *_Nullable searchString;
/// The folder the search looks in, kept in the "spotlightSearchPath" setting.
@property(copy, nonatomic) NSURL *_Nonnull searchFolder;
/// The results table and its scroll view, from SpotlightPanel.xib.
@property(strong, nonatomic) IBOutlet NSScrollView *_Nullable resultsView;
/// The results, as the table shows them, for the panel to count.
@property(readonly, nonatomic) NSArrayController *_Nonnull resultsController;

@end
