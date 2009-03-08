//
//  PreferencePane.h
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "PreferencePanePlugin.h"

@interface GeneralPreferencePane : NSObject <PreferencePane> {
	IBOutlet NSView *view;
	
	NSString *title;
	NSImage *icon;
}

+ (GeneralPreferencePane *)preferencePaneWithView:(NSView *)v title:(NSString *)t iconNamed:(NSString *)i;

- (void)setView:(NSView *)v;
- (void)setTitle:(NSString *)t;
- (void)setIcon:(NSImage *)i;

@end
