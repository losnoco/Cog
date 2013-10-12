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

- (void)addToPlaylist:(NSArray *)urls
{
	[playlistLoader willInsertURLs:urls origin:URLOriginInternal];
	[playlistLoader didInsertURLs:[playlistLoader addURLs:urls sort:YES] origin:URLOriginInternal];
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
