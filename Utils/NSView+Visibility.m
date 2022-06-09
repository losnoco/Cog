//
//  NSView+Visibility.m
//  Cog
//
//  Created by Christopher Snowhill on 6/8/22.
//

#import "NSView+Visibility.h"

@implementation NSView (Visibility)

- (BOOL)visibleInWindow {
	if(self.window == nil) {
		return NO;
	}

	if(![self.window isVisible]) {
		return NO;
	}

	// Might have zero opacity.
	if(self.alphaValue == 0 || self.hiddenOrHasHiddenAncestor) {
		return NO;
	}

	// Might be clipped by an ancestor.
	return !NSIsEmptyRect(self.visibleRect);
}

@end
