//
//  FileTreeDataSource.h
//  Cog
//
//  Created by Vincent Spader on 10/14/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class PathNode;
@class PathWatcher;
@class FileTreeWindowController;

@interface FileTreeDataSource : NSObject {
	PathNode *rootNode;

	IBOutlet FileTreeWindowController *fileTreeController;
	IBOutlet PathWatcher *watcher;
	IBOutlet NSOutlineView *outlineView;
}

- (NSURL *)rootURL;
- (void)setRootURL:(NSURL *)rootURL;

- (IBAction)doubleClickSelector:(id)sender;

- (void)reloadPathNode:(PathNode *)item;

@end
