//
//  SignalInspectorWindowController.swift
//  Cog
//
//  Created by Kevin López Brante on 2026-10-05.
//

import Cocoa
import SwiftUI

/// Owns the Signal Inspector HUD panel. Instantiated by MainMenu.xib, which
/// connects `playbackController` and targets toggleWindow:/showWindow:; the
/// main window's output status opens it too.
///
/// The output is metered only while the panel is on screen.
@objc(SignalInspectorWindowController)
final class SignalInspectorWindowController: NSWindowController, NSWindowDelegate {
	@IBOutlet var playbackController: PlaybackController?

	private let meters = SignalMetricsModel()
	private var timer: Timer?

	/// Meters refresh at about the engine's own tick.
	private static let refreshInterval = 1.0 / 20

	@objc init() {
		super.init(window: nil)
	}

	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}

	override func loadWindow() {
		let panel = NSPanel(
			contentRect: NSRect(x: 0, y: 0, width: 320, height: 640),
			styleMask: [.titled, .closable, .resizable, .utilityWindow, .nonactivatingPanel, .hudWindow],
			backing: .buffered,
			defer: true
		)
		panel.title = NSLocalizedString("Signal Inspector", comment: "Signal Inspector window title")
		panel.titlebarAppearsTransparent = true
		panel.hidesOnDeactivate = true
		panel.isReleasedWhenClosed = false
		panel.animationBehavior = .utilityWindow
		panel.contentMinSize = NSSize(width: 300, height: 360)
		panel.contentView = NSHostingView(rootView: SignalInspectorView(status: .shared, meters: meters))
		panel.setFrameAutosaveName("SignalInspector")
		panel.delegate = self
		window = panel

		// The panel hides with the app, so metering stops with it.
		let center = NotificationCenter.default
		for name in [NSApplication.didHideNotification, NSApplication.didResignActiveNotification,
		             NSApplication.didUnhideNotification, NSApplication.didBecomeActiveNotification] {
			center.addObserver(self, selector: #selector(applicationVisibilityDidChange(_:)), name: name, object: nil)
		}
		center.addObserver(self, selector: #selector(applicationVisibilityDidChange(_:)), name: NSWindow.didChangeOcclusionStateNotification, object: panel)
	}

	@objc private func applicationVisibilityDidChange(_ notification: Notification) {
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
		if window == nil {
			loadWindow()
		}
		if window?.isVisible == true {
			window?.orderOut(self)
			updateMetering()
		} else {
			showWindow(self)
		}
	}

	func windowWillClose(_ notification: Notification) {
		stopMetering()
	}

	// MARK: - Metering

	/// Meters while the panel can be seen.
	private func updateMetering() {
		if window?.isVisible == true && window?.occlusionState.contains(.visible) != false {
			startMetering()
		} else {
			stopMetering()
		}
	}

	private func startMetering() {
		guard timer == nil, let player = playbackController?.audioPlayer() else { return }
		player.meteringEnabled = true
		let timer = Timer(timeInterval: Self.refreshInterval, repeats: true) { [weak self] _ in
			MainActor.assumeIsolated { self?.refresh() }
		}
		RunLoop.main.add(timer, forMode: .common)
		self.timer = timer
		refresh()
	}

	private func stopMetering() {
		timer?.invalidate()
		timer = nil
		playbackController?.audioPlayer()?.meteringEnabled = false
		meters.update(nil)
	}

	private func refresh() {
		meters.update(playbackController?.audioPlayer()?.signalMetrics())
	}
}
