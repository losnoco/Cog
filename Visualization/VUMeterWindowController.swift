//
//  VUMeterWindowController.swift
//  Cog
//
//  Created by Kevin López Brante on 2026-10-22.
//

import Cocoa
import SwiftUI

/// Owns the VU meter window. Instantiated by MainMenu.xib, which connects
/// `playbackController` and targets toggleWindow:/showWindow:; the toolbar's
/// VU meter opens it too.
///
/// The output is metered only while the window is on screen.
@objc(VUMeterWindowController)
final class VUMeterWindowController: NSWindowController, NSWindowDelegate {
	@IBOutlet var playbackController: PlaybackController?

	private let model = VUMeterModel()

	@objc init() {
		super.init(window: nil)
	}

	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}

	override func loadWindow() {
		let window = NSWindow(
			contentRect: NSRect(x: 196, y: 240, width: 380, height: 150),
			styleMask: [.titled, .closable, .miniaturizable, .resizable],
			backing: .buffered,
			defer: true
		)
		window.title = NSLocalizedString("VU Meter", comment: "VU meter window title")
		window.appearance = NSAppearance(named: .darkAqua)
		window.titlebarAppearsTransparent = true
		window.isReleasedWhenClosed = false
		window.contentMinSize = NSSize(width: 240, height: 90)
		window.contentView = NSHostingView(rootView: VUMeterView(model: model))
		window.setFrameAutosaveName("VUMeter")
		window.delegate = self
		self.window = window

		NotificationCenter.default.addObserver(self, selector: #selector(occlusionStateDidChange(_:)), name: NSWindow.didChangeOcclusionStateNotification, object: window)
	}

	@objc private func occlusionStateDidChange(_ notification: Notification) {
		updateMetering()
	}

	override func showWindow(_ sender: Any?) {
		// Without a nib, NSWindowController never calls loadWindow() itself.
		if window == nil {
			loadWindow()
		}
		super.showWindow(sender)
		updateMetering()
	}

	@IBAction func toggleWindow(_ sender: Any?) {
		if window?.isVisible == true {
			window?.orderOut(self)
			updateMetering()
		} else {
			showWindow(self)
		}
	}

	func windowWillClose(_ notification: Notification) {
		OutputMeter.shared.unsubscribe(self)
	}

	/// Meters while the window can be seen.
	private func updateMetering() {
		if window?.isVisible == true && window?.occlusionState.contains(.visible) != false {
			OutputMeter.shared.subscribe(self, player: playbackController?.audioPlayer()) { [weak self] metrics in
				self?.model.update(metrics)
			}
		} else {
			OutputMeter.shared.unsubscribe(self)
		}
	}
}

/// The VU meter's toolbar item: the output's levels as two small bars, which
/// open the VU meter window when clicked.
@objc(VUMeterItem)
final class VUMeterItem: NSToolbarItem {
	@IBOutlet var playbackController: PlaybackController?
	@IBOutlet var windowController: VUMeterWindowController?

	override func awakeFromNib() {
		super.awakeFromNib()
		let view = VUMeterItemView(frame: NSRect(x: 0, y: 0, width: 64, height: 26))
		view.playbackController = playbackController
		view.onClick = { [weak self] in
			self?.windowController?.showWindow(nil)
		}
		self.view = view
	}
}

/// Meters while its window is on screen.
final class VUMeterItemView: NSView {
	weak var playbackController: PlaybackController?
	var onClick: (() -> Void)?

	private let model = SignalMetricsModel()
	private var occlusionObserver: NSObjectProtocol?

	override init(frame: NSRect) {
		super.init(frame: frame)
		setUp()
	}

	/// Customize Toolbar's palette copies the item's view through a coder.
	required init?(coder: NSCoder) {
		super.init(coder: coder)
		subviews.forEach { $0.removeFromSuperview() }
		setUp()
	}

	private func setUp() {
		let host = NSHostingView(rootView: CompactVUMeter(model: model))
		host.frame = bounds
		host.autoresizingMask = [.width, .height]
		addSubview(host)
		toolTip = NSLocalizedString("Output levels. Click to open the VU meter.", comment: "VU meter toolbar item tooltip")
	}

	override func viewDidMoveToWindow() {
		super.viewDidMoveToWindow()
		if let occlusionObserver {
			NotificationCenter.default.removeObserver(occlusionObserver)
		}
		occlusionObserver = window.map { window in
			NotificationCenter.default.addObserver(forName: NSWindow.didChangeOcclusionStateNotification, object: window, queue: .main) { [weak self] _ in
				MainActor.assumeIsolated { self?.updateMetering() }
			}
		}
		updateMetering()
	}

	private func updateMetering() {
		if let window, window.isVisible, window.occlusionState.contains(.visible) {
			OutputMeter.shared.subscribe(self, player: playbackController?.audioPlayer()) { [weak self] metrics in
				self?.model.update(metrics)
			}
		} else {
			OutputMeter.shared.unsubscribe(self)
		}
	}

	// The bars take no clicks of their own; the item opens the window.
	override func hitTest(_ point: NSPoint) -> NSView? {
		frame.contains(point) ? self : nil
	}

	override func mouseDown(with event: NSEvent) {
		onClick?()
	}
}
