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

+ (void)initialize
{
	NSMutableDictionary *userDefaultsValuesDict = [NSMutableDictionary dictionary];
	
	[userDefaultsValuesDict setObject:[NSNumber numberWithBool:YES] forKey:@"sideViewVertical"];
	[userDefaultsValuesDict setObject:[NSNumber numberWithBool:NO] forKey:@"showSideView"];
	[userDefaultsValuesDict setObject:[NSNumber numberWithFloat:100.0] forKey:@"sideViewDividerPosition"];
	[userDefaultsValuesDict setObject:@"File Tree" forKey:@"lastSideViewTitle"];
	
	[[NSUserDefaults standardUserDefaults] registerDefaults:userDefaultsValuesDict];
}


- (id)init
{
	self = [super init];
	if (self)
	{
		sideViewNibs = [[NSMutableDictionary alloc] init];
		sideViews = [[NSMutableDictionary alloc] init];
		
		sideView = nil;
	}
	
	return self;
}

- (void)dealloc
{
	[sideViews release];
	[sideViewNibs release];
	
	[sideView release];
	
	[super dealloc];
}

- (void)awakeFromNib
{
	[sideViewNibs setObject:@"FileTree" forKey:@"File Tree"];
	
	for (NSString *title in sideViewNibs)
	{
		//Create menu item
		NSMenuItem *item = [[NSMenuItem alloc] initWithTitle:title action:@selector(selectSideView:) keyEquivalent:@""];
		[item setTarget:self];
		[item setRepresentedObject:title];
		[sideViewMenu addItem:item];
	}

	
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"showSideView"])
	{
		[self showSideView];
	}
	
	if (![[NSUserDefaults standardUserDefaults] boolForKey:@"sideViewVertical"])
	{
		[self toggleVertical:self];
	}
	
	NSLog(@"TWICE?!");
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

	[[NSUserDefaults standardUserDefaults] setObject:title forKey:@"lastSideViewTitle"];
	
	return newSideView;
}

- (IBAction)selectSideView:(id)sender
{
	NSString *title = [sender representedObject];
	SideView *newSideView = [self sideViewForTitle:title];
	
	if (newSideView == [self sideView])
	{
		[self toggleSideView:self];
	}
	else
	{
		[self setSideView: newSideView];
		[self showSideView];
	}
	
	[splitView adjustSubviews];
}

- (IBAction)toggleSideView:(id)sender
{
	//Show/hide current
	if ([self sideViewIsHidden])
	{
		[self showSideView];
	}
	else
	{
		[self hideSideView];
	}

	[splitView adjustSubviews];
}

- (IBAction)toggleVertical:(id)sender
{
	[splitView setVertical:![splitView isVertical]];
	
	if (![self sideViewIsHidden])
	{
		[self setSideView:[self sideView]];
		[self showSideView];
	}
	
	[splitView adjustSubviews];

	[[NSUserDefaults standardUserDefaults] setBool:[splitView isVertical] forKey:@"sideViewVertical"];
}

- (SideView *)sideView
{
	if (sideView == nil)
	{
		[self setSideView:[self sideViewForTitle:[[NSUserDefaults standardUserDefaults] objectForKey:@"lastSideViewTitle"]]];
	}
	
	return sideView;
}

- (void)setSideView:(SideView *)newSideView
{
	[newSideView retain];
	sideView = newSideView;
	[sideView release];
}

- (void)showSideView
{
	if ([splitView isVertical]) {
		[splitView setSubviews:[NSArray arrayWithObjects:[[self sideView] view], playlistView, nil]];
	}
	else {
		[splitView setSubviews:[NSArray arrayWithObjects:playlistView, [[self sideView] view], nil]];
	}
	
	[self setDividerPosition: [[NSUserDefaults standardUserDefaults] floatForKey:@"sideViewDividerPosition"]];

	[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"showSideView"];
}

- (void)hideSideView
{
	[splitView setSubviews:[NSArray arrayWithObject:playlistView]];
	[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"showSideView"];
}

- (BOOL)sideViewIsHidden
{
	return ([[splitView subviews] count] == 1);
}

- (BOOL)splitView:(NSSplitView *)aSplitView canCollapseSubview:(NSView *)subview
{
	return (subview != playlistView);
}


- (BOOL)splitView:(NSSplitView *)aSplitView shouldCollapseSubview:(NSView *)subview forDoubleClickOnDividerAtIndex:(NSInteger)dividerIndex
{
	return (subview != playlistView);
}

- (void)splitViewDidResizeSubviews:(NSNotification *)aNotification
{
	//Update default
	if (![self sideViewIsHidden])
	{
		[[NSUserDefaults standardUserDefaults] setFloat:[self dividerPosition] forKey:@"sideViewDividerPosition"];
		NSLog(@"DIVIDER POSITION: %f", [self dividerPosition]);
	}
}

- (void)splitView:(NSSplitView *)sender resizeSubviewsWithOldSize: (NSSize)oldSize
{
	if ([self sideViewIsHidden])
	{
		[splitView adjustSubviews];
	}
	else
	{
		CGFloat dividerThickness = [splitView dividerThickness];
		
		NSRect sideRect = [[sideView view] frame];
		NSRect playlistRect = [playlistView frame];
		
		NSRect newFrame = [splitView frame];
		
		if ([splitView isVertical])
		{
			NSLog(@"VERTICAL");
			sideRect.size.width = [[NSUserDefaults standardUserDefaults] floatForKey:@"sideViewDividerPosition"];
			sideRect.size.height = newFrame.size.height;
			sideRect.origin = NSMakePoint(0, 0);
			
			playlistRect.size.width = newFrame.size.width - sideRect.size.width - dividerThickness;
			playlistRect.size.height = newFrame.size.height;
			playlistRect.origin.x = sideRect.size.width + dividerThickness;
		}
		else
		{
			NSLog(@"NOT VERTICAL");
			sideRect.size.height = [[NSUserDefaults standardUserDefaults] floatForKey:@"sideViewDividerPosition"];
			sideRect.size.width = newFrame.size.width;
			
			playlistRect.origin = NSMakePoint(0, 0);
			playlistRect.size.width = newFrame.size.width;
			playlistRect.size.height = newFrame.size.height - sideRect.size.height - dividerThickness;

			sideRect.origin.y = playlistRect.size.height + dividerThickness;
		}
		

		NSLog(@"SIDE: %f,%f %fx%f", sideRect.origin.x, sideRect.origin.y, sideRect.size.width, sideRect.size.height);
		NSLog(@"Playlist: %f,%f %fx%f", playlistRect.origin.x, playlistRect.origin.y, playlistRect.size.width, playlistRect.size.height);
		
		[[sideView view] setFrame:sideRect];
		[playlistView setFrame:playlistRect];
	}
}

- (float)dividerPosition
{
	if ([splitView isVertical])
	{
		return [[sideView view] frame].size.width;
	}
	
	return [[sideView view] frame].size.height;
}

- (void)setDividerPosition:(float)position
{
	float actualPosition = position;
	if (![splitView isVertical])
	{
		actualPosition = ([splitView frame].size.height - position);
	}
	
	[splitView adjustSubviews];
	[splitView setPosition:actualPosition ofDividerAtIndex:0];

	[[NSUserDefaults standardUserDefaults] setFloat:position forKey:@"sideViewDividerPosition"];
	NSLog(@"SETTING POSITION: %f (%f) == %f?", actualPosition, position, [self dividerPosition]);
	
	NSRect sideRect = [[sideView view] frame];
	NSRect playlistRect = [playlistView frame];
	NSLog(@"SIDE: %f,%f %fx%f", sideRect.origin.x, sideRect.origin.y, sideRect.size.width, sideRect.size.height);
	NSLog(@"Playlist: %f,%f %fx%f", playlistRect.origin.x, playlistRect.origin.y, playlistRect.size.width, playlistRect.size.height);
}

- (void) addToPlaylist:(NSArray *)urls
{
	[playlistLoader willInsertFiles:urls origin:OpenFromFileTree];
	[playlistLoader didInsertFiles:[playlistLoader addURLs:urls sort:YES] origin:OpenFromFileTree];
}


@end
