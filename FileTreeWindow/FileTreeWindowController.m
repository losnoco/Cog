//
//  FileTreeController.m
//  Cog
//
//  Created by Vincent Spader on 2/17/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "FileTreeWindowController.h"


@implementation FileTreeWindowController

- (id)init
{
	return [super initWithWindowNibName:@"FileTreePanel"];
}

- (void)addToPlaylist:(NSArray *)urls
{
	[playlistLoader addURLs:urls sort:NO];
}

@end
