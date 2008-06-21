//
//  SplitViewController.h
//  Cog
//
//  Created by Vincent Spader on 6/20/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class SideView;
@class PlaylistLoader;

@interface SideViewController : NSObject {
	IBOutlet NSSplitView *splitView;
	IBOutlet NSView *playlistView;
	IBOutlet NSMenu *sideViewMenu;
	IBOutlet PlaylistLoader *playlistLoader;
	
	NSMutableDictionary *sideViewNibs;
	NSMutableDictionary *sideViews;
	SideView *sideView;
}

- (IBAction)toggleVertical:(id)sender;
- (IBAction)selectSideView:(id)sender;
- (IBAction)toggleSideView:(id)sender;

- (void)showSideView;
- (void)hideSideView;
- (BOOL)sideViewIsHidden;


- (SideView *)sideViewForTitle:(NSString *)title;

- (SideView *)sideView;
- (void)setSideView:(SideView *)newSideView;

- (float)dividerPosition;
- (void)setDividerPosition:(float)position;

//Helper for adding files to the playlist
- (void) addToPlaylist:(NSArray *)urls;
@end
