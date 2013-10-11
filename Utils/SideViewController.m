//
//  SplitViewController.m
//  Cog
//
//  Created by Vincent Spader on 6/20/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SideViewController.h"

#import "Logging.h"

@implementation SideViewController

- (NSString *)showSideViewDefaultsKey
{
	return [NSString stringWithFormat:@"%@ShowSideView", [self nibName]];
}

- (NSString *)sideViewVerticalDefaultsKey
{
	return [NSString stringWithFormat:@"%@SideViewVertical", [self nibName]];
}

- (NSString *)sideViewDividerPositionDefaultsKey
{
	return [NSString stringWithFormat:@"%@SideViewDividerPosition", [self nibName]];
}

- (void)registerDefaults
{
	NSMutableDictionary *userDefaultsValuesDict = [NSMutableDictionary dictionary];
	
	[userDefaultsValuesDict setObject:[NSNumber numberWithBool:YES] forKey:[self sideViewVerticalDefaultsKey]];
	[userDefaultsValuesDict setObject:[NSNumber numberWithBool:NO] forKey:[self showSideViewDefaultsKey]];
	[userDefaultsValuesDict setObject:[NSNumber numberWithFloat:100.0] forKey:[self sideViewDividerPositionDefaultsKey]];
	
	[[NSUserDefaults standardUserDefaults] registerDefaults:userDefaultsValuesDict];
}

- (id)initWithNibName:(NSString *)nib bundle:(NSBundle *)bundle
{
	self = [super initWithNibName:nib bundle:bundle];
	if (self)
	{
		[self registerDefaults];
	}
	
	return self;
}


- (void)awakeFromNib
{
	[splitView setVertical:[[NSUserDefaults standardUserDefaults] boolForKey:[self sideViewVerticalDefaultsKey]]];

	if ([[NSUserDefaults standardUserDefaults] boolForKey:[self showSideViewDefaultsKey]])
	{
		[self showSideView];
	}
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
		[self showSideView];
	}
	
	[splitView adjustSubviews];
	
	[[NSUserDefaults standardUserDefaults] setBool:[splitView isVertical] forKey:[self sideViewVerticalDefaultsKey]];
}

- (void)showSideView
{
	if ([splitView isVertical]) {
		[splitView setSubviews:[NSArray arrayWithObjects:[self view], mainView, nil]];
	}
	else {
		[splitView setSubviews:[NSArray arrayWithObjects:mainView, [self view], nil]];
	}
	
	[self setDividerPosition: [[NSUserDefaults standardUserDefaults] floatForKey:[self sideViewDividerPositionDefaultsKey]]];
	
	[[[self view] window] makeFirstResponder:firstResponder];
	
	[[NSUserDefaults standardUserDefaults] setBool:YES forKey:[self showSideViewDefaultsKey]];
}

- (void)hideSideView
{
	[splitView setSubviews:[NSArray arrayWithObject:mainView]];
	[[NSUserDefaults standardUserDefaults] setBool:NO forKey:[self showSideViewDefaultsKey]];
	
	[[mainView window] makeFirstResponder:mainView];
}

- (BOOL)sideViewIsHidden
{
	return ([[splitView subviews] count] == 1);
}

- (BOOL)splitView:(NSSplitView *)aSplitView canCollapseSubview:(NSView *)subview
{
	return (subview != mainView);
}


- (BOOL)splitView:(NSSplitView *)aSplitView shouldCollapseSubview:(NSView *)subview forDoubleClickOnDividerAtIndex:(NSInteger)dividerIndex
{
	return (subview != mainView);
}

- (void)splitViewDidResizeSubviews:(NSNotification *)aNotification
{
	//Update default
	if (![self sideViewIsHidden])
	{
		[[NSUserDefaults standardUserDefaults] setFloat:[self dividerPosition] forKey:[self sideViewDividerPositionDefaultsKey]];
		DLog(@"DIVIDER POSITION: %f", [self dividerPosition]);
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
		
		NSRect sideRect = [[self view] frame];
		NSRect mainRect = [mainView frame];
		
		NSRect newFrame = [splitView frame];
		
		if ([splitView isVertical])
		{
			sideRect.size.width = [[NSUserDefaults standardUserDefaults] floatForKey:[self sideViewDividerPositionDefaultsKey]];
			sideRect.size.height = newFrame.size.height;
			sideRect.origin = NSMakePoint(0, 0);
			
			mainRect.size.width = newFrame.size.width - sideRect.size.width - dividerThickness;
			mainRect.size.height = newFrame.size.height;
			mainRect.origin.x = sideRect.size.width + dividerThickness;
		}
		else
		{
			sideRect.size.height = [[NSUserDefaults standardUserDefaults] floatForKey:[self sideViewDividerPositionDefaultsKey]];
			sideRect.size.width = newFrame.size.width;
			
			mainRect.origin = NSMakePoint(0, 0);
			mainRect.size.width = newFrame.size.width;
			mainRect.size.height = newFrame.size.height - sideRect.size.height - dividerThickness;
			
			sideRect.origin.y = mainRect.size.height + dividerThickness;
		}
		
		
		[[self view] setFrame:sideRect];
		[mainView setFrame:mainRect];
	}
}

- (float)dividerPosition
{
	if ([splitView isVertical])
	{
		return [[self view] frame].size.width;
	}
	
	return [[self view] frame].size.height;
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
	
	[[NSUserDefaults standardUserDefaults] setFloat:position forKey:[self sideViewDividerPositionDefaultsKey]];
}


@end
