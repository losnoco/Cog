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

@interface PreferencesWindow ()

- (NSString *)lastPaneDefaultsKey;
- (BOOL)loadPaneNamed:(NSString *)name display:(BOOL)display;
- (void)createToolbar;

@end

@implementation PreferencesWindow {
	NSMutableArray<NSToolbarItemIdentifier> *preferencePaneOrder;
	NSMutableDictionary<NSToolbarItemIdentifier, id<PreferencePane>> *preferencePanes;
	NSMutableDictionary<NSToolbarItemIdentifier, NSToolbarItem *> *toolbarItems;
}

- (instancetype)initWithPreferencePanes:(NSArray<id<PreferencePane>> *)panes {
	NSWindowStyleMask windowStyleMask =
	(NSWindowStyleMaskClosable | NSWindowStyleMaskTitled);
	self = [super initWithContentRect:NSMakeRect(0, 0, 640, 300)
	                        styleMask:windowStyleMask
	                          backing:NSBackingStoreBuffered
	                            defer:NO];
	if(self) {
		preferencePaneOrder = [NSMutableArray new];
		preferencePanes = [NSMutableDictionary new];

		for(id<PreferencePane> pane in panes) {
			[preferencePaneOrder addObject:[pane title]];
			[preferencePanes setObject:pane forKey:[pane title]];
		}

		[self setReleasedWhenClosed:NO];
		[self setTitle:NSLocalizedString(@"PreferencesTitle", @"")];
		[self center];

		if(@available(macOS 11.0, *)) {
			[self setToolbarStyle:NSWindowToolbarStylePreference];
		}

		[self createToolbar];
	}

	return self;
}

- (NSString *)lastPaneDefaultsKey {
	return @"LastPreferencePane";
}

- (void)setContentView:(NSView *)view animate:(BOOL)animate {
	NSSize newSize = view.bounds.size;
	NSSize oldSize = [self contentView].bounds.size;

	CGFloat diff = newSize.height - oldSize.height;
	NSRect newFrame = [self frame];
	newFrame.size.height += diff;
	newFrame.origin.y -= diff;

	if(animate) {
		[self setContentView:nil];
	}
	[self setFrame:newFrame display:animate animate:animate];

	[self setContentView:view];
	[self setContentSize:newSize];
}

- (BOOL)loadPaneNamed:(NSString *)name display:(BOOL)display {
	id<PreferencePane> paneController = preferencePanes[name];
	if(!paneController) {
		return NO;
	}

	NSView *paneView = [paneController view];
	if(!paneView || [self contentView] == paneView) {
		return NO;
	}

	[self setContentView:paneView animate:display];
	if([paneController respondsToSelector:@selector(refreshPathList:)]) {
		[paneController refreshPathList:self];
	}

	// Update defaults
	[[NSUserDefaults standardUserDefaults] setObject:name forKey:[self lastPaneDefaultsKey]];

	[[self toolbar] setSelectedItemIdentifier:name];

	return YES;
}

- (void)createToolbar {
	toolbarItems = [NSMutableDictionary new];
	for(NSString *name in preferencePaneOrder) {
		id<PreferencePane> pane = preferencePanes[name];

		NSToolbarItem *item = [[NSToolbarItem alloc] initWithItemIdentifier:name];
		[item setPaletteLabel:name]; // item's label in the "Customize Toolbar" sheet (not relevant here, but we set it anyway)
		[item setLabel:name]; // item's label in the toolbar

		[item setToolTip:name];
		[item setImage:[pane icon]];

		[item setTarget:self];
		[item setAction:@selector(toolbarItemClicked:)]; // action called when item is clicked

		toolbarItems[name] = item;
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

- (void)show {
	NSString *lastPane = [[NSUserDefaults standardUserDefaults] objectForKey:[self lastPaneDefaultsKey]];
	// Previous pane names migrations.
	if([lastPane isEqualToString:NSLocalizedPrefString(@"Growl")]) {
		lastPane = NSLocalizedPrefString(@"Notifications");
	}
	if([lastPane isEqualToString:NSLocalizedPrefString(@"Last.fm")] ||
	   [lastPane isEqualToString:NSLocalizedPrefString(@"Scrobble")]) {
		lastPane = NSLocalizedPrefString(@"General");
	}
	if(nil == lastPane) {
		if(0 >= [preferencePaneOrder count]) {
			ALog(@"Error: Preference panes not found!");
		}

		lastPane = preferencePaneOrder[0];
	}

	[self loadPaneNamed:lastPane display:NO];

	[self makeKeyAndOrderFront:self];
}

- (void)showPathSuggester {
	NSString *name = NSLocalizedPrefString(@"General");

	[self loadPaneNamed:name display:NO];

	[self makeKeyAndOrderFront:self];

	id<PreferencePane> pane = preferencePanes[name];
	if(pane && [pane respondsToSelector:@selector(showPathSuggester:)]) {
		[pane showPathSuggester:self];
	}
}

- (void)showRubberbandSettings {
	NSString *name = NSLocalizedPrefString(@"Rubber Band");

	[self loadPaneNamed:name display:NO];

	[self makeKeyAndOrderFront:self];
}

// Close on Esc pressed.
- (void)cancelOperation:(id)sender {
	[self close];
}

#pragma mark Delegate methods

- (NSArray *)toolbarAllowedItemIdentifiers:(NSToolbar *)toolbar {
	return preferencePaneOrder;
}

- (NSArray *)toolbarSelectableItemIdentifiers:(NSToolbar *)toolbar {
	return preferencePaneOrder;
}

- (NSArray *)toolbarDefaultItemIdentifiers:(NSToolbar *)toolbar {
	return preferencePaneOrder;
}

- (NSToolbarItem *)toolbar:(NSToolbar *)toolbar itemForItemIdentifier:(NSString *)itemIdentifier willBeInsertedIntoToolbar:(BOOL)flag {
	return toolbarItems[itemIdentifier];
}

- (void)toolbarItemClicked:(NSToolbarItem *)item {
	[self loadPaneNamed:[item itemIdentifier] display:YES];
}

@end
