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
	[playlistLoader willInsertURLs:urls origin:URLOriginExternal];
	[playlistLoader didInsertURLs:[playlistLoader addURLs:urls sort:YES] origin:URLOriginExternal];
}


@end
