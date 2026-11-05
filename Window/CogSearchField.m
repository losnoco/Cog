//
//  CogSearchField.m
//  Cog
//
//  Created by Christopher Snowhill on 9/30/25.
//

#import "CogSearchField.h"

NS_ASSUME_NONNULL_BEGIN

@implementation CogSearchField {
	NSLayoutConstraint *borrowedWidth;
}

/// Customize Toolbar's palette takes this very field out of the toolbar to
/// draw it, in windows of AppKit's own. If its width changes there, the search
/// item lays out the toolbar while the item's view is away, and the toolbar
/// raises an exception as it constrains that view to its place. That happens
/// only with the field expanded, in a window wide enough for it. So the field
/// keeps its width until it is back in the main window.
- (void)viewWillMoveToWindow:(nullable NSWindow *)newWindow {
	if(newWindow == mainWindow) {
		borrowedWidth.active = NO;
		borrowedWidth = nil;
	} else if(self.window == mainWindow && !borrowedWidth) {
		borrowedWidth = [self.widthAnchor constraintEqualToConstant:NSWidth(self.frame)];
		borrowedWidth.active = YES;
	}
	[super viewWillMoveToWindow:newWindow];
}

- (void)cancelOperation:(nullable id)sender {
	[mainWindow makeFirstResponder:nil];
	[mainWindow resignFirstResponder];
}

@end

NS_ASSUME_NONNULL_END
