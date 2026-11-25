//
//  NSView+Visibility.swift
//  Cog
//
//  Created by Christopher Snowhill on 6/8/22.
//  Copyright 2022 Christopher Snowhill. All rights reserved.
//

import AppKit

extension NSView {
	@objc var visibleInWindow: Bool {
		guard let window, window.isVisible else { return false }

		// Might have zero opacity.
		if alphaValue == 0 || isHiddenOrHasHiddenAncestor { return false }

		// Might be clipped by an ancestor.
		return !visibleRect.isEmpty
	}
}