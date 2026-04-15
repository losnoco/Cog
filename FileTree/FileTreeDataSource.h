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

@interface FileTreeDataSource : NSObject <NSOutlineViewDataSource, NSSearchFieldDelegate>

@property(nonatomic, weak) IBOutlet NSOutlineView *outlineView;
@property(nonatomic, weak) IBOutlet NSPathControl *pathControl;
@property(nonatomic, weak) IBOutlet PathWatcher *watcher;
@property(nonatomic, weak) NSSearchField *searchField;

@property(nonatomic, copy) NSString *filterString;

- (void)changeURL:(NSURL *)rootURL;

- (void)reloadPathNode:(PathNode *)item;

@end
