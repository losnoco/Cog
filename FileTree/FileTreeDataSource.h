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

@interface FileTreeDataSource : NSObject <NSOutlineViewDataSource>

@property(nonatomic, weak) IBOutlet NSOutlineView *outlineView;
@property(nonatomic, weak) IBOutlet NSPathControl *pathControl;
@property(nonatomic, weak) IBOutlet PathWatcher *watcher;

- (void)changeURL:(NSURL *)rootURL;

- (void)reloadPathNode:(PathNode *)item;

@end
