//
//  PreferencesWindowController.h
//  Cog
//
//  Created by Vincent Spader on 3/7/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

/**
 * TODO: Use NSTabViewController.
 * This will allow to manage Toolbar automatically, lazy load
 * preference panes and and crossfade between them.
 */
@interface PreferencesWindow : NSWindow<NSToolbarDelegate>

- (instancetype)initWithPreferencePanes:(NSArray *)preferencePanes NS_DESIGNATED_INITIALIZER;
- (instancetype)initWithContentRect:(NSRect)contentRect
                          styleMask:(NSWindowStyleMask)style
                            backing:(NSBackingStoreType)backingStoreType
                              defer:(BOOL)flag NS_UNAVAILABLE;

- (void)show;

@end
