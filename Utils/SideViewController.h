//
//  SideBarController.h
//  Cog
//
//  Created by Vincent Spader on 6/21/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PlaylistController.h"

@interface SideViewController : NSViewController {
	IBOutlet NSSplitView *splitView;
	IBOutlet NSView *mainView;
	IBOutlet NSView *firstResponder;
}

- (IBAction)toggleSideView:(id)sender;
- (IBAction)toggleVertical:(id)sender;

- (void)showSideView;
- (void)hideSideView;
- (BOOL)sideViewIsHidden;

- (void)setDividerPosition:(float)position;
- (float)dividerPosition;

- (void)doAddToPlaylist:(NSArray *)urls origin:(URLOrigin)origin;

- (void)clear:(id)sender;

- (void)playPauseResume:(id)sender;

@end
