//
//  SplitViewController.m
//  Cog
//
//  Created by Vincent Spader on 6/20/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SideViewController.h"

#import "PlaylistLoader.h"
#import "SideView.h"

@implementation SideViewController

- (id)init
{
	self = [super init];
	if (self)
	{
		sideViewNibs = [[NSMutableDictionary alloc] init];
		sideViews = [[NSMutableDictionary alloc] init];
	}
	
	return self;
}

- (void)dealloc
{
	[sideViews release];
	[sideViewNibs release];
	
	[super dealloc];
}

- (void)awakeFromNib
{
	for (NSString *title in sideViewNibs)
	{
		//Create menu item
		NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:title action:@selector(selectSideView:) keyEquivalent:@""];
		[item setTarget:self];
		[item setRepresentedObject:title];
		[sideViewMenu addItem:item];
	}
}

- (SideView *)sideViewForTitle:(NSString *)title
{
	SideView *newSideView = [sideViews objectForKey:title];
	if (newSideView == nil)
	{
		NSString *nibName = [sideViewNibs objectForKey:title];
		newSideView = [[SideView alloc] initWithNibNamed:nibName controller:self];
		
		[sideViews setObject:newSideView forKey:title];
		
		[newSideView release];
	}
	
	return newSideView;
}

- (IBAction)selectSideView:(id)sender
{
	NSString *title = [sender representedObject];
	
	[self selectSideViewWithTitle:title];
}

- (void)selectSideViewWithTitle:(NSString *)title
{
	//To be overridden
}

- (void) addToPlaylist:(NSArray *)urls
{
	[playlistLoader willInsertURLs:urls origin:URLOriginExternal];
	[playlistLoader didInsertURLs:[playlistLoader addURLs:urls sort:YES] origin:URLOriginExternal];
}


@end
