//
//  FileTreeController.m
//  Cog
//
//  Created by Vincent Spader on 2/17/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "PlaylistController.h"
#import "FileTreeViewController.h"


@implementation FileTreeViewController

- (IBAction)addToPlaylist:(id)sender
{
	unsigned int index;
	NSIndexSet *selectedIndexes = [outlineView selectedRowIndexes];
	NSMutableArray *urls = [[NSMutableArray alloc] init];

	for (index = [selectedIndexes firstIndex];
		 index != NSNotFound; index = [selectedIndexes indexGreaterThanIndex: index])  
	{
		[urls addObject:[[outlineView itemAtRow:index] URL]];
	}
	
	[controller addToPlaylist:urls];
	[urls release];
}


@end
