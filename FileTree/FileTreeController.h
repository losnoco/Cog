//
//  FileTreeController.h
//  Cog
//
//  Created by Vincent Spader on 2/17/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "FileTreeDataSource.h"

@class SideViewController;
@interface FileTreeController : NSObject {
	IBOutlet SideViewController *controller;
	IBOutlet NSOutlineView *outlineView;
	IBOutlet FileTreeDataSource *dataSource;
}

- (IBAction)addToPlaylist:(id)sender;
- (IBAction)setAsPlaylist:(id)sender;
- (IBAction)playPauseResume:(NSObject *)id;
- (IBAction)showEntryInFinder:(id)sender;
- (IBAction)setAsRoot:(id)sender;

@end
