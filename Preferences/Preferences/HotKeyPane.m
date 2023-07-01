//
//  HotKeyPane.m
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "HotKeyPane.h"
#import "Shortcuts.h"

MASShortcut *shortcutWithMigration(NSString *oldKeyCodePrefName,
                                   NSString *oldKeyModifierPrefName,
                                   NSString *newShortcutPrefName,
                                   NSInteger newDefaultKeyCode) {
	NSEventModifierFlags defaultModifiers = NSEventModifierFlagControl | NSEventModifierFlagCommand;
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	if([defaults objectForKey:oldKeyCodePrefName]) {
		NSInteger oldKeyCode = [defaults integerForKey:oldKeyCodePrefName];
		NSEventModifierFlags oldKeyModifiers = [defaults integerForKey:oldKeyModifierPrefName];
		// Should we consider temporarily save these values for further migration?
		[defaults removeObjectForKey:oldKeyCodePrefName];
		[defaults removeObjectForKey:oldKeyModifierPrefName];
		return [MASShortcut shortcutWithKeyCode:oldKeyCode modifierFlags:oldKeyModifiers];
	} else {
		return [MASShortcut shortcutWithKeyCode:newDefaultKeyCode modifierFlags:defaultModifiers];
	}
}

@implementation HotKeyPane {
	NSUserDefaultsController *defaultsController;
}

- (void)awakeFromNib {
	MASShortcut *playShortcut = shortcutWithMigration(@"hotKeyPlayKeyCode",
	                                                  @"hotKeyPlayModifiers",
	                                                  CogPlayShortcutKey,
	                                                  kVK_ANSI_P);
	MASShortcut *nextShortcut = shortcutWithMigration(@"hotKeyNextKeyCode",
	                                                  @"hotKeyNextModifiers",
	                                                  CogNextShortcutKey,
	                                                  kVK_ANSI_N);
	MASShortcut *prevShortcut = shortcutWithMigration(@"hotKeyPreviousKeyCode",
	                                                  @"hotKeyPreviousModifiers",
	                                                  CogPrevShortcutKey,
	                                                  kVK_ANSI_R);
	MASShortcut *spamShortcut = shortcutWithMigration(@"hotKeySpamKeyCode",
	                                                  @"hotKeySpamModifiers",
	                                                  CogSpamShortcutKey,
	                                                  kVK_ANSI_C);
	MASShortcut *fadeShortcut = shortcutWithMigration(@"hotKeyFadeKeyCode",
													  @"hotKeyFadeModifiers",
													  CogFadeShortcutKey,
													  kVK_ANSI_O);

	NSData *playShortcutData = [NSKeyedArchiver archivedDataWithRootObject:playShortcut];
	NSData *nextShortcutData = [NSKeyedArchiver archivedDataWithRootObject:nextShortcut];
	NSData *prevShortcutData = [NSKeyedArchiver archivedDataWithRootObject:prevShortcut];
	NSData *spamShortcutData = [NSKeyedArchiver archivedDataWithRootObject:spamShortcut];
	NSData *fadeShortcutData = [NSKeyedArchiver archivedDataWithRootObject:fadeShortcut];

	// Register default values to be used for the first app start
	NSDictionary<NSString *, NSData *> *defaultShortcuts = @{
		CogPlayShortcutKey: playShortcutData,
		CogNextShortcutKey: nextShortcutData,
		CogPrevShortcutKey: prevShortcutData,
		CogSpamShortcutKey: spamShortcutData,
		CogFadeShortcutKey: fadeShortcutData
	};

	defaultsController =
	[[NSUserDefaultsController sharedUserDefaultsController] initWithDefaults:nil
	                                                            initialValues:defaultShortcuts];

	_playShortcutView.associatedUserDefaultsKey = CogPlayShortcutKey;
	_nextShortcutView.associatedUserDefaultsKey = CogNextShortcutKey;
	_prevShortcutView.associatedUserDefaultsKey = CogPrevShortcutKey;
	_spamShortcutView.associatedUserDefaultsKey = CogSpamShortcutKey;
	_fadeShortcutView.associatedUserDefaultsKey = CogFadeShortcutKey;
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
