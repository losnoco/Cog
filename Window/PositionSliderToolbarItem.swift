//
//  PositionSliderToolbarItem.swift
//  Cog
//
//  Created by Dzmitry Neviadomski on 7.01.21.
//

import Cocoa

class PositionSliderToolbarItem: NSToolbarItem {
    override var minSize: NSSize{
        get {
            return NSSize(width: 100, height: 28)
        }
        set {}
    }
	override var maxSize: NSSize{
		get {
			return NSSize(width: 1024, height: 28)
		}
		set {}
	}

	override func awakeFromNib() {
		super.awakeFromNib()
		guard let slider = view, !(slider is PositionSliderContainer) else { return }
		let container = PositionSliderContainer(frame: slider.frame)
		slider.frame = container.bounds
		slider.autoresizingMask = [.width, .height]
		slider.translatesAutoresizingMaskIntoConstraints = true
		container.addSubview(slider)
		view = container
	}
}

/// Holds the seek bar so that none of it drags the window. In the toolbar
/// before macOS 26, the slider kept only its top 20 points out of the
/// window's drag area, while the waveform fills all 28, so a click on the
/// waveform's lower part moved the window. AppKit leaves out of that area a
/// view that takes first responder and handles mouseDown:, and looks no
/// further into it; a view inside the slider was not seen. The clicks still
/// go to the slider.
final class PositionSliderContainer: NSView {
	override var acceptsFirstResponder: Bool {
		true
	}

	override var canBecomeKeyView: Bool {
		false
	}

	override var mouseDownCanMoveWindow: Bool {
		false
	}

	private var forwarding = false

	/// A disabled slider passes the click up the responder chain, which
	/// leads back here; drop it then rather than forward it again.
	override func mouseDown(with event: NSEvent) {
		guard !forwarding, let slider = subviews.first else { return }
		forwarding = true
		defer { forwarding = false }
		slider.mouseDown(with: event)
	}
}
