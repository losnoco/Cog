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

- (void)awakeFromNib
{
	[outlineView setDoubleAction:@selector(addToPlaylist:)];
	[outlineView setTarget:self];
}
	
- (IBAction)addToPlaylist:(id)sender
{
	NSArray *urls = [NSArray arrayWithObject:[[outlineView itemAtRow:[outlineView clickedRow]] URL]];

	[playlistLoader addURLs:urls sort:NO];
}

@end
