//
//  SideBarController.h
//  Cog
//
//  Created by Vincent Spader on 6/21/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "SideViewController.h"

@class SideView;

@interface SideBarController : SideViewController {
	IBOutlet NSSplitView *splitView;
	IBOutlet NSView *playlistView;
	SideView *sideView;
}

- (IBAction)toggleSideBar:(id)sender;
- (IBAction)toggleVertical:(id)sender;

- (void)showSideBar;
- (void)hideSideBar;
- (BOOL)sideBarIsHidden;

- (void)setDividerPosition:(float)position;
- (float)dividerPosition;

- (SideView *)sideView;
- (void)setSideView:(SideView *)sideView;

@end
