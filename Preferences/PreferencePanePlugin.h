//
//  PreferencePanePlugin.h
//  Cog
//
//  Created by Vincent Spader on 3/7/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#define NSLocalizedPrefString(key) \
	[[NSBundle bundleWithIdentifier:@"org.cogx.cog.preferences"] localizedStringForKey:(key) value:@"" table:nil]

@protocol PreferencePane <NSObject>
@required
@property(readonly) NSView *view;
@property(readonly, copy) NSString *title;
@property(readonly) NSImage *icon;

@optional
- (IBAction)showPathSuggester:(id)sender;

@end

@protocol PreferencePanePlugin <NSObject>
@required

// An array of PrefPaneController instances that the plugin has available
+ (NSArray<id<PreferencePane>> *)preferencePanes;

@end
