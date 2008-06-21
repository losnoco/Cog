//
//  SplitViewController.m
//  Cog
//
//  Created by Vincent Spader on 6/20/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SideBarController.h"

#import "SideView.h"

@implementation SideBarController

+ (void)initialize
{
	NSMutableDictionary *userDefaultsValuesDict = [NSMutableDictionary dictionary];
	
	[userDefaultsValuesDict setObject:[NSNumber numberWithBool:YES] forKey:@"sideBarVertical"];
	[userDefaultsValuesDict setObject:[NSNumber numberWithBool:NO] forKey:@"showSideBar"];
	[userDefaultsValuesDict setObject:[NSNumber numberWithFloat:100.0] forKey:@"sideBarDividerPosition"];
	[userDefaultsValuesDict setObject:@"File Tree" forKey:@"lastSideBar"];
	
	[[NSUserDefaults standardUserDefaults] registerDefaults:userDefaultsValuesDict];
}


- (id)init
{
	self = [super init];
	if (self)
	{
		[sideViewNibs setObject:@"FileTree" forKey:@"File Tree"];
		[sideViewNibs setObject:@"FileTree" forKey:@"Another File Tree!"];
		
		sideView = nil;
	}
	
	return self;
}

- (void)dealloc
{
	[sideView release];
	
	[super dealloc];
}

- (void)awakeFromNib
{
	[super awakeFromNib];
	
	if ([[NSUserDefaults standardUserDefaults] boolForKey:@"showSideBar"])
	{
		[self showSideBar];
	}
	
	if (![[NSUserDefaults standardUserDefaults] boolForKey:@"sideBarVertical"])
	{
		[self toggleVertical:self];
	}
}

- (void)selectSideViewWithTitle:(NSString *)title
{
	SideView *newSideView = [self sideViewForTitle:title];

	[[NSUserDefaults standardUserDefaults] setObject:title forKey:@"lastSideBar"];
	
	if (newSideView == [self sideView])
	{
		[self toggleSideBar:self];
	}
	else
	{
		[self setSideView: newSideView];
		[self showSideBar];
	}
	
	[splitView adjustSubviews];
}

- (IBAction)toggleSideBar:(id)sender
{
	//Show/hide current
	if ([self sideBarIsHidden])
	{
		[self showSideBar];
	}
	else
	{
		[self hideSideBar];
	}
	
	[splitView adjustSubviews];
}

- (IBAction)toggleVertical:(id)sender
{
	[splitView setVertical:![splitView isVertical]];
	
	if (![self sideBarIsHidden])
	{
		[self setSideView:[self sideView]];
		[self showSideBar];
	}
	
	[splitView adjustSubviews];
	
	[[NSUserDefaults standardUserDefaults] setBool:[splitView isVertical] forKey:@"sideBarVertical"];
}

- (SideView *)sideView
{
	if (sideView == nil)
	{
		[self setSideView:[self sideViewForTitle:[[NSUserDefaults standardUserDefaults] objectForKey:@"lastSideBar"]]];
	}
	
	return sideView;
}

- (void)setSideView:(SideView *)newSideView
{
	[newSideView retain];
	sideView = newSideView;
	[sideView release];
}

- (void)showSideBar
{
	if ([splitView isVertical]) {
		[splitView setSubviews:[NSArray arrayWithObjects:[[self sideView] view], playlistView, nil]];
	}
	else {
		[splitView setSubviews:[NSArray arrayWithObjects:playlistView, [[self sideView] view], nil]];
	}
	
	[self setDividerPosition: [[NSUserDefaults standardUserDefaults] floatForKey:@"sideBarDividerPosition"]];
	
	[[[sideView view] window] makeFirstResponder:[sideView firstResponder]];
	
	[[NSUserDefaults standardUserDefaults] setBool:YES forKey:@"showSideBar"];
}

- (void)hideSideBar
{
	[splitView setSubviews:[NSArray arrayWithObject:playlistView]];
	[[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"showSideBar"];
	
	[[playlistView window] makeFirstResponder:playlistView];
}

- (BOOL)sideBarIsHidden
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
	if (![self sideBarIsHidden])
	{
		[[NSUserDefaults standardUserDefaults] setFloat:[self dividerPosition] forKey:@"sideBarDividerPosition"];
		NSLog(@"DIVIDER POSITION: %f", [self dividerPosition]);
	}
}

- (void)splitView:(NSSplitView *)sender resizeSubviewsWithOldSize: (NSSize)oldSize
{
	if ([self sideBarIsHidden])
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
			sideRect.size.width = [[NSUserDefaults standardUserDefaults] floatForKey:@"sideBarDividerPosition"];
			sideRect.size.height = newFrame.size.height;
			sideRect.origin = NSMakePoint(0, 0);
			
			playlistRect.size.width = newFrame.size.width - sideRect.size.width - dividerThickness;
			playlistRect.size.height = newFrame.size.height;
			playlistRect.origin.x = sideRect.size.width + dividerThickness;
		}
		else
		{
			NSLog(@"NOT VERTICAL");
			sideRect.size.height = [[NSUserDefaults standardUserDefaults] floatForKey:@"sideBarDividerPosition"];
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
	
	[[NSUserDefaults standardUserDefaults] setFloat:position forKey:@"sideBarDividerPosition"];
	NSLog(@"SETTING POSITION: %f (%f) == %f?", actualPosition, position, [self dividerPosition]);
	
	NSRect sideRect = [[sideView view] frame];
	NSRect playlistRect = [playlistView frame];
	NSLog(@"SIDE: %f,%f %fx%f", sideRect.origin.x, sideRect.origin.y, sideRect.size.width, sideRect.size.height);
	NSLog(@"Playlist: %f,%f %fx%f", playlistRect.origin.x, playlistRect.origin.y, playlistRect.size.width, playlistRect.size.height);
}




@end
