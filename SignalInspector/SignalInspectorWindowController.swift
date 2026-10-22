//
//  SignalInspectorWindowController.swift
//  Cog
//
//  Created by Kevin López Brante on 2026-10-05.
//

import Cocoa
import SwiftUI

/// Owns the Signal Inspector, in its HUD panel or docked. Instantiated by
/// MainMenu.xib, which connects `playbackController` and targets
/// toggleWindow:/showWindow:; the main window's output status opens it too.
///
/// The output is metered only while the inspector can be seen.
@objc(SignalInspectorWindowController)
final class SignalInspectorWindowController: PaneWindowController {
	@IBOutlet var playbackController: PlaybackController?

	private let meters = SignalMetricsModel()

	override var paneIdentifier: String {
		"signalInspector"
	}

	override var paneTitle: String {
		NSLocalizedString("Signal Inspector", comment: "Signal Inspector window title")
	}

	override var defaultPlacement: PanePlacement {
		PanePlacement(edge: .right, position: 1, size: 320)
	}

	override func awakeFromNib() {
		super.awakeFromNib()
		// The panel hides with the app, so metering stops with it.
		let center = NotificationCenter.default
		for name in [NSApplication.didHideNotification, NSApplication.didResignActiveNotification,
		             NSApplication.didUnhideNotification, NSApplication.didBecomeActiveNotification] {
			center.addObserver(self, selector: #selector(applicationVisibilityDidChange(_:)), name: name, object: nil)
		}
	}

	override func makePaneView() -> NSView {
		hostingView(SignalInspectorView(status: .shared, meters: meters))
	}

	override func makeWindow() -> NSWindow {
		let panel = NSPanel(
			contentRect: NSRect(x: 0, y: 0, width: 320, height: 640),
			styleMask: [.titled, .closable, .resizable, .utilityWindow, .nonactivatingPanel, .hudWindow],
			backing: .buffered,
			defer: true
		)
		panel.title = paneTitle
		panel.titlebarAppearsTransparent = true
		panel.hidesOnDeactivate = true
		panel.isReleasedWhenClosed = false
		panel.animationBehavior = .utilityWindow
		panel.contentMinSize = NSSize(width: 300, height: 360)
		panel.setFrameAutosaveName("SignalInspector")
		return panel
	}

	@objc private func applicationVisibilityDidChange(_ notification: Notification) {
		paneVisibilityDidChange()
	}

	override func paneVisibilityDidChange() {
		if isPaneVisible {
			OutputMeter.shared.subscribe(self, player: playbackController?.audioPlayer()) { [weak self] metrics in
				self?.meters.update(metrics)
			}
		} else {
			OutputMeter.shared.unsubscribe(self)
		}
	}
}
