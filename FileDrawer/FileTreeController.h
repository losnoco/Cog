//
//  FileTreeController.h
//  Cog
//
//  Created by Vincent Spader on 8/20/2006.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@class FileTreeWatcher;
@class PlaylistLoader;

@interface FileTreeController : NSTreeController
{
	IBOutlet PlaylistLoader *playlistLoader;

	NSString *rootPath;
	
	FileTreeWatcher *watcher;
}

- (FileTreeWatcher *)watcher;

- (id)rootPath;
- (void)setRootPath:(id)r;
- (void)refreshRoot;
- (NSArray *)acceptableFileTypes;

@end
