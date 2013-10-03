//
//  PreferencesWindowController.h
//  Cog
//
//  Created by Vincent Spader on 3/7/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface PreferencesWindow : NSWindow<NSToolbarDelegate> {
	NSMutableArray *preferencePaneOrder;
	NSMutableDictionary *preferencePanes;
	NSMutableDictionary *toolbarItems;
}

- (id)initWithPreferencePanes:(NSArray *)preferencePanes;

- (void)show;

@end
