//
//  SpeedButton.swift
//  Cog
//
//  Created by Christopher Snowhill on 9/20/24.
//  Rewritten in SwiftUI by Kevin López Brante on 2026-10-19.
//

import Cocoa
import SwiftUI

/// The toolbar's speed button, which MainMenu.xib makes: it opens the speed
/// controls in a popover.
@objc(SpeedButton)
final class SpeedButton: NSButton, NSPopoverDelegate {
	private let popover = NSPopover()
	private let control = SpeedControl()
	private var scrollMonitor: Any?

	override func awakeFromNib() {
		super.awakeFromNib()
		// Nib loading happens on the main thread.
		MainActor.assumeIsolated {
			popover.behavior = .transient
			popover.delegate = self
		}
	}

	override func mouseDown(with event: NSEvent) {
		popover.close()

		let content = NSHostingController(rootView: SpeedPopoverView(control: control) { [weak self] in
			self?.popover.close()
			NSApp.sendAction(#selector(AppController.showRubberbandSettings(_:)), to: nil, from: self)
		})
		popover.contentViewController = content
		popover.contentSize = content.view.fittingSize
		popover.show(relativeTo: bounds, of: self, preferredEdge: .maxY)
		watchScrollWheel(over: content.view)

		super.mouseDown(with: event)
	}

	/// SwiftUI's sliders ignore the scroll wheel; the popover hands it to the
	/// one under the pointer.
	private func watchScrollWheel(over view: NSView) {
		stopWatchingScrollWheel()
		scrollMonitor = NSEvent.addLocalMonitorForEvents(matching: .scrollWheel) { [weak self, weak view] event in
			let window = event.window
			let location = event.locationInWindow
			let delta = event.deltaY
			let handled = MainActor.assumeIsolated {
				guard let self, let view, window === view.window else { return false }
				// The hosting view is flipped, as the popover's coordinate space is.
				return self.control.scroll(at: view.convert(location, from: nil), by: delta)
			}
			return handled ? nil : event
		}
	}

	private func stopWatchingScrollWheel() {
		if let scrollMonitor {
			NSEvent.removeMonitor(scrollMonitor)
		}
		scrollMonitor = nil
	}

	func popoverDidClose(_ notification: Notification) {
		stopWatchingScrollWheel()
	}
}
