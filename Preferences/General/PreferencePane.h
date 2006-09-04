//
//  PreferencePane.h
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SS_PreferencePaneProtocol.h"

@interface PreferencePane : NSObject <SS_PreferencePaneProtocol> {
	IBOutlet NSView *view;
	
	NSString *name;
	NSImage *icon;
}

- (NSView *)paneView;
- (NSString *)paneName;
- (NSImage *)paneIcon;
- (NSString *)paneToolTip;

- (BOOL)allowsHorizontalResizing;
- (BOOL)allowsVerticalResizing;

- (void)setName:(NSString *)s;
- (void)setIcon:(NSString *)i;
- (void)setToolTip:(NSString *)t;

@end
