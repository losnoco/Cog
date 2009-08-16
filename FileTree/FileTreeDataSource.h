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

@interface FileTreeDataSource : NSObject {
	PathNode *rootNode;
	
	IBOutlet NSPathControl *pathControl;
	IBOutlet PathWatcher *watcher;
	IBOutlet NSOutlineView *outlineView;
}

- (NSURL *)rootURL;
- (void)setRootURL:(NSURL *)rootURL;
- (void)changeURL:(NSURL *)rootURL;

- (void)reloadPathNode:(PathNode *)item;

@end
