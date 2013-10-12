//
//  SplitViewController.m
//  Cog
//
//  Created by Vincent Spader on 6/20/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "FileTreeViewController.h"
#import "PlaylistLoader.h"

@implementation FileTreeViewController

- (id)init
{
	return [super initWithNibName:@"FileTree" bundle:[NSBundle mainBundle]];
}

- (void)addToPlaylistInternal:(NSArray *)urls
{
	[self doAddToPlaylist:urls origin:URLOriginInternal];
}

- (void)addToPlaylistExternal:(NSArray *)urls
{
    [self doAddToPlaylist:urls origin:URLOriginExternal];
}

- (void)doAddToPlaylist:(NSArray *)urls origin:(URLOrigin)origin
{
    [playlistLoader willInsertURLs:urls origin:origin];
    [playlistLoader didInsertURLs:[playlistLoader addURLs:urls sort:YES] origin:origin];
}

- (void)clear:(id)sender
{
	[playlistLoader clear:sender];
}

- (void)playPauseResume:(NSObject *)id
{
	[playbackController playPauseResume:id];
}

- (FileTreeOutlineView*)outlineView
{
    return fileTreeOutlineView;
}

@end
