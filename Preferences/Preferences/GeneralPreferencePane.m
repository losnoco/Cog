//
//  PreferencePane.m
//  Preferences
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

#import "GeneralPreferencePane.h"

@implementation GeneralPreferencePane

@synthesize title = _title;
@synthesize icon = _icon;
@synthesize view;

+ (instancetype)preferencePaneWithView:(NSView *)view
                                 title:(NSString *)title
                        systemIconName:(NSString *)systemIconName
                        orOldIconNamed:(NSString *)oldIconName {
	NSImage *icon;
	if(@available(macOS 11.0, *)) {
		if(systemIconName)
			icon = [NSImage imageWithSystemSymbolName:systemIconName accessibilityDescription:nil];
	}
	if(icon == nil) {
		NSString *file = [[NSBundle bundleForClass:[self class]] pathForImageResource:oldIconName];
		icon = [[NSImage alloc] initWithContentsOfFile:file];
	}

	return [[self alloc] initWithView:view title:title icon:icon];
}

- (instancetype)initWithView:(NSView *)contentView title:(NSString *)title icon:(NSImage *)image {
	self = [super init];
	if(self) {
		view = contentView;
		_title = title;
		_icon = image;
	}
	return self;
}

@end
