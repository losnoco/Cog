//
//  PreferencesWindowController.m
//  Cog
//
//  Created by Vincent Spader on 3/7/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import "PreferencesWindow.h"
#import "PreferencePanePlugin.h"

#import "Logging.h"

@interface PreferencesWindow()

- (NSString *)lastPaneDefaultsKey;
- (BOOL)loadPaneNamed:(NSString *)name display:(BOOL)display;
- (void)createToolbar;

@end


@implementation PreferencesWindow

- (id)initWithPreferencePanes:(NSArray *)panes
{
    self = [super initWithContentRect:NSMakeRect(0, 0, 350, 200)
                                              styleMask:(NSClosableWindowMask | NSResizableWindowMask | NSTitledWindowMask)
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
	if (self)
	{
		preferencePaneOrder = [[NSMutableArray alloc] init];
		preferencePanes = [[NSMutableDictionary alloc] init];
		
		for (id<PreferencePane> pane in panes) {
			[preferencePaneOrder addObject:[pane title]];
			[preferencePanes setObject:pane forKey:[pane title]];
		}
		
		[self setReleasedWhenClosed:NO];
		[self setTitle:@"Preferences"]; // initial default title
		[self setShowsToolbarButton: NO];
		[self setShowsResizeIndicator:NO];
		[self center];
        
        [[self standardWindowButton:NSWindowZoomButton] setEnabled:FALSE];
        
        
        if (@available(macOS 11, *)) {
            [self setToolbarStyle:NSWindowToolbarStylePreference];
        }
		
		[self createToolbar];
	}
	
	return self;
}

- (NSString *)lastPaneDefaultsKey
{
	return @"LastPreferencePane";
}

-(NSRect)newFrameForNewContentView:(NSView *)view {
	NSRect newFrame = [self frame];
    newFrame.size.height = [view frame].size.height + ([self frame].size.height - [[self contentView] frame].size.height);
    newFrame.size.width = [view frame].size.width;
    newFrame.origin.y += ([[self contentView] frame].size.height - [view frame].size.height);

	return newFrame;
}

- (void)setContentView:(NSView *)view animate:(BOOL)animate
{
    if (animate) {
        NSView *tempView = [[NSView alloc] initWithFrame:[[self contentView] frame]];
        [self setContentView:tempView];
    }
	
	NSRect newFrame = [self newFrameForNewContentView:view];
	[self setFrame:newFrame display:animate animate:animate];
	
	[self setContentView:view];
}

- (BOOL)loadPaneNamed:(NSString *)name display:(BOOL)display
{
    id<PreferencePane> paneController = [preferencePanes objectForKey:name];
    if (!paneController) {
        return NO;
    }
    
    NSView *paneView = [paneController view];
    if (!paneView) {
        return NO;
    }
	
	[self setContentView:paneView animate:display];
    
	
	[self setTitle:name];
    
    // Update defaults
    [[NSUserDefaults standardUserDefaults] setObject:name forKey:[self lastPaneDefaultsKey]];
	
    [[self toolbar] setSelectedItemIdentifier:name];
    
    return YES;
}

- (void)createToolbar
{
	toolbarItems = [[NSMutableDictionary alloc] init];
	for (NSString *name in preferencePaneOrder) {
		id<PreferencePane> pane = [preferencePanes objectForKey:name];
		
		NSToolbarItem *item = [[NSToolbarItem alloc] initWithItemIdentifier:name];
		[item setPaletteLabel:name]; // item's label in the "Customize Toolbar" sheet (not relevant here, but we set it anyway)
		[item setLabel:name]; // item's label in the toolbar
		
		[item setToolTip:name];
		[item setImage:[pane icon]];
		
		[item setTarget:self];
		[item setAction:@selector(toolbarItemClicked:)]; // action called when item is clicked
		
		[toolbarItems setObject:item forKey:name];
	}
	
	NSString *bundleIdentifier = [[NSBundle mainBundle] bundleIdentifier];
	NSToolbar *toolbar = [[NSToolbar alloc] initWithIdentifier:[bundleIdentifier stringByAppendingString:@" Preferences Toolbar"]];
	[toolbar setDelegate:self];
	[toolbar setAllowsUserCustomization:NO];
	[toolbar setAutosavesConfiguration:NO];
	[toolbar setDisplayMode:NSToolbarDisplayModeIconAndLabel];
	
	[toolbar setSizeMode:NSToolbarSizeModeDefault];
	
	[self setToolbar:toolbar];
}



- (void)show
{
	NSString *lastPane = [[NSUserDefaults standardUserDefaults] objectForKey:[self lastPaneDefaultsKey]];
	if (nil == lastPane) {
		if (0 >= [preferencePaneOrder count]) {
			ALog(@"Error: Preference panes not found!");
		}
		
		lastPane = [preferencePaneOrder objectAtIndex:0];
	}
	
	[self loadPaneNamed:lastPane display:NO];
	
	[self makeKeyAndOrderFront:self];
}


#pragma mark Delegate methods

- (NSArray *)toolbarAllowedItemIdentifiers:(NSToolbar*)toolbar
{
    return preferencePaneOrder;
}


- (NSArray *)toolbarSelectableItemIdentifiers:(NSToolbar *)toolbar
{
    return preferencePaneOrder;
}


- (NSArray *)toolbarDefaultItemIdentifiers:(NSToolbar*)toolbar
{
    return preferencePaneOrder;
}

- (NSToolbarItem *)toolbar:(NSToolbar *)toolbar itemForItemIdentifier:(NSString *)itemIdentifier willBeInsertedIntoToolbar:(BOOL)flag
{
    return [toolbarItems objectForKey:itemIdentifier];
}

- (void)toolbarItemClicked:(NSToolbarItem *)item
{
    if (![[item itemIdentifier] isEqualToString:[self title]]) {
        [self loadPaneNamed:[item itemIdentifier] display:YES];
    }
}

@end
