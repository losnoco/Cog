//
//  PreferencePanePlugin.h
//  Cog
//
//  Created by Vincent Spader on 3/7/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@protocol PreferencePanePlugin <NSObject> 
@required

// An array of PrefPaneController instances that the plugin has available
+ (NSArray *)preferencePanes;

@end

@protocol PreferencePane <NSObject>
@required

- (NSView *)view;
- (NSString *)title;
- (NSImage *)icon;

@end
