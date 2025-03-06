//
//  HotKeyPane.m
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "HotKeyPane.h"
#import "Shortcuts.h"

@implementation HotKeyPane {
	NSUserDefaultsController *defaultsController;
}

// Defaults have been moved to AppController.m
- (void)awakeFromNib {
	defaultsController = [NSUserDefaultsController sharedUserDefaultsController];

	_playShortcutView.associatedUserDefaultsKey = CogPlayShortcutKey;
	_nextShortcutView.associatedUserDefaultsKey = CogNextShortcutKey;
	_prevShortcutView.associatedUserDefaultsKey = CogPrevShortcutKey;
	_spamShortcutView.associatedUserDefaultsKey = CogSpamShortcutKey;
	_fadeShortcutView.associatedUserDefaultsKey = CogFadeShortcutKey;
	_seekBkwdShortcutView.associatedUserDefaultsKey = CogSeekBackwardShortcutKey;
	_seekFwdShortcutView.associatedUserDefaultsKey = CogSeekForwardShortcutKey;
}

- (NSString *)title {
	return NSLocalizedPrefString(@"Hot Keys");
}

- (NSImage *)icon {
	if(@available(macOS 11.0, *))
		return [NSImage imageWithSystemSymbolName:@"keyboard" accessibilityDescription:nil];
	return [[NSImage alloc] initWithContentsOfFile:[[NSBundle bundleForClass:[self class]] pathForImageResource:@"hot_keys"]];
}

- (IBAction)resetToDefaultShortcuts:(id)sender {
	[defaultsController revertToInitialValues:sender];
}

@end
