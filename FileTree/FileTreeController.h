//
//  FileTreeController.h
//  Cog
//
//  Created by Vincent Spader on 2/17/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class SideViewController;
@interface FileTreeController : NSObject {
	IBOutlet SideViewController *controller;
	IBOutlet NSOutlineView *outlineView;
}

- (IBAction)addToPlaylist:(id)sender;

@end
