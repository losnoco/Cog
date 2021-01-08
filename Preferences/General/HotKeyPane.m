//
//  HotKeyPane.m
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "HotKeyPane.h"
#import "NDHotKey/NDHotKeyEvent.h"
#import "NDHotKey/NDKeyboardLayout.h"
#import "HotKeyControl.h"

@implementation HotKeyPane

static void setControlText(HotKeyControl* control, NSString* kcprop, NSString* mprop)
{
    UInt16 keyCode = [[NSUserDefaults standardUserDefaults] integerForKey:kcprop];
    NSUInteger modifiers = [[NSUserDefaults standardUserDefaults] integerForKey:mprop];
    NSString *str = [[NDKeyboardLayout keyboardLayout] stringForKeyCode:keyCode modifierFlags:(UInt32)modifiers];
    [control setStringValue:str];
}

- (void)awakeFromNib
{
    setControlText(prevHotKeyControl, @"hotKeyPreviousKeyCode", @"hotKeyPreviousModifiers");
    setControlText(nextHotKeyControl, @"hotKeyNextKeyCode", @"hotKeyNextModifiers");
    setControlText(playHotKeyControl, @"hotKeyPlayKeyCode", @"hotKeyPlayModifiers");
    setControlText(spamHotKeyControl, @"hotKeySpamKeyCode", @"hotKeySpamModifiers");
}

- (NSString *)title
{
    return NSLocalizedPrefString(@"Hot Keys");
}

- (NSImage *)icon
{
    if (@available(macOS 11.0, *))
        return [NSImage imageWithSystemSymbolName:@"keyboard" accessibilityDescription:nil];
    return [[NSImage alloc] initWithContentsOfFile:[[NSBundle bundleForClass:[self class]] pathForImageResource:@"hot_keys"]];
}

- (IBAction) hotKeyChanged:(id)sender
{
    if (sender == playHotKeyControl) {
        [[NSUserDefaults standardUserDefaults] setInteger:[playHotKeyControl modifierFlags] forKey:@"hotKeyPlayModifiers"];
        [[NSUserDefaults standardUserDefaults] setInteger:[playHotKeyControl keyCode] forKey:@"hotKeyPlayKeyCode"];
    }
    else if (sender == prevHotKeyControl) {
        [[NSUserDefaults standardUserDefaults] setInteger:[prevHotKeyControl modifierFlags] forKey:@"hotKeyPreviousModifiers"];
        [[NSUserDefaults standardUserDefaults] setInteger:[prevHotKeyControl keyCode] forKey:@"hotKeyPreviousKeyCode"];
    }
    else if (sender == nextHotKeyControl) {
        [[NSUserDefaults standardUserDefaults] setInteger:[nextHotKeyControl modifierFlags] forKey:@"hotKeyNextModifiers"];
        [[NSUserDefaults standardUserDefaults] setInteger:[nextHotKeyControl keyCode] forKey:@"hotKeyNextKeyCode"];
    }
    else if (sender == spamHotKeyControl) {
        [[NSUserDefaults standardUserDefaults] setInteger:[spamHotKeyControl modifierFlags] forKey:@"hotKeySpamModifiers"];
        [[NSUserDefaults standardUserDefaults] setInteger:[spamHotKeyControl keyCode] forKey:@"hotKeySpamKeyCode"];
    }
}

@end
