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
}

+ (instancetype)preferencePaneWithView:(NSView *)view
                                 title:(NSString *)title
                        systemIconName:(NSString *)systemIconName
                        orOldIconNamed:(NSString *)oldIconName;

- (instancetype) init NS_UNAVAILABLE;
- (instancetype) initWithView:(NSView *)contentView
                        title:(NSString *)title
                         icon:(NSImage *)image NS_DESIGNATED_INITIALIZER;

@end
