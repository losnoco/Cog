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
		guard let slider = view, !(slider is ToolbarSliderContainer) else { return }
		let container = ToolbarSliderContainer(frame: slider.frame)
		slider.frame = container.bounds
		slider.autoresizingMask = [.width, .height]
		slider.translatesAutoresizingMaskIntoConstraints = true
		container.addSubview(slider)
		view = container
	}
}

/// The volume slider, in a container as tall as the toolbar's controls.
/// The slider itself is 14 points tall, and before macOS 26 a click a
/// point or two below its knob moved the window instead.
class VolumeSliderToolbarItem: NSToolbarItem {
	override func awakeFromNib() {
		super.awakeFromNib()
		guard let slider = view, !(slider is ToolbarSliderContainer) else { return }
		// Its width comes from the slider's own constraints.
		let container = ToolbarSliderContainer(frame: NSRect(x: 0, y: 0, width: slider.frame.width, height: 28))
		slider.translatesAutoresizingMaskIntoConstraints = false
		container.addSubview(slider)
		NSLayoutConstraint.activate([
			slider.leadingAnchor.constraint(equalTo: container.leadingAnchor),
			slider.trailingAnchor.constraint(equalTo: container.trailingAnchor),
			slider.centerYAnchor.constraint(equalTo: container.centerYAnchor),
			container.heightAnchor.constraint(equalToConstant: 28),
		])
		view = container
	}
}

/// Holds a toolbar slider so that none of it drags the window. In the
/// toolbar before macOS 26, a slider kept only what AppKit takes as the
/// control out of the window's drag area: the seek bar's top 20 points,
/// while the waveform fills all 28, so a click on the waveform's lower part
/// moved the window. AppKit leaves out of that area a view that takes first
/// responder and handles mouseDown:, and looks no further into it; a view
/// inside the slider was not seen. The clicks still go to the slider, which
/// takes its value from where the pointer is across it.
final class ToolbarSliderContainer: NSView {
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

	/// The volume slider scrolls; the seek bar passes scrolling up the
	/// responder chain, back here, and it goes on from here instead.
	override func scrollWheel(with event: NSEvent) {
		guard !forwarding, let slider = subviews.first else { return super.scrollWheel(with: event) }
		forwarding = true
		defer { forwarding = false }
		slider.scrollWheel(with: event)
	}
}
