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
	IBOutlet NSMenu *sideViewMenu;
	IBOutlet PlaylistLoader *playlistLoader;
	
	NSMutableDictionary *sideViewNibs;
	NSMutableDictionary *sideViews;
}

- (SideView *)sideViewForTitle:(NSString *)title;
- (void)selectSideViewWithTitle:(NSString *)title; //To be overridden by subclass

//Helper for adding files to the playlist
- (void) addToPlaylist:(NSArray *)urls;

@end
