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

+ (PreferencePane *)preferencePaneWithView:(NSView *)v name:(NSString *)n icon:(NSString *)i;

- (NSView *)paneView;
- (NSString *)paneName;
- (NSImage *)paneIcon;
- (NSString *)paneToolTip;

- (BOOL)allowsHorizontalResizing;
- (BOOL)allowsVerticalResizing;

- (void)setView:(NSView *)v;
- (void)setName:(NSString *)s;
- (void)setIcon:(NSString *)i;

@end
