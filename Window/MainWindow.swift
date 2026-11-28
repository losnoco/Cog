//
//  MainWindow.swift
//  Cog
//
//  Created by Vincent Spader on 2/22/09.
//  Copyright 2009 Vincent Spader. All rights reserved.
//

import AppKit

// NOTICE! We bury first time defaults that should depend on whether the install is fresh or not here
// so that they get created correctly depending on the situation.

// For instance, for the first option to get this treatment, we want time stretching to stay enabled
// for existing installations, but disable itself by default on new installs, to spare processing.

@MainActor
func showSentryConsent(_ window: NSWindow) {
	let defaults = UserDefaults.standard
	guard !defaults.bool(forKey: "sentryAskedConsent") else { return }

	window.orderFront(window)

	let alert = NSAlert()
	alert.messageText = NSLocalizedString("SentryConsentTitle", comment: "")
	alert.informativeText = NSLocalizedString("SentryConsentText", comment: "")
	alert.addButton(withTitle: NSLocalizedString("ConsentNo", comment: ""))
	alert.addButton(withTitle: NSLocalizedString("ConsentYes", comment: ""))

	alert.beginSheetModal(for: window) { returnCode in
		if returnCode == .alertSecondButtonReturn {
			UserDefaults.standard.set(true, forKey: "sentryConsented")
		}
	}

	defaults.set(true, forKey: "sentryAskedConsent")
}

@objc(MainWindow)
class MainWindow: NSWindow {
	@IBOutlet var playlistView: NSView?
	@IBOutlet var mainToolbar: NSToolbar?
	@IBOutlet var outputFormatField: NSTextField?
	@IBOutlet var searchField: NSSearchField?
	// The SignalInspectorWindowController, opened by clicking the output status.
	@IBOutlet var signalInspector: AnyObject?
	// The bar under the playlist, with the total duration and the output status.
	@IBOutlet var statusBar: NSView?

	override init(contentRect: NSRect, styleMask windowStyle: NSWindow.StyleMask, backing bufferingType: NSWindow.BackingStoreType, defer deferCreation: Bool) {
		super.init(contentRect: contentRect, styleMask: windowStyle, backing: bufferingType, defer: deferCreation)
		isExcludedFromWindowsMenu = true
		collectionBehavior = .fullScreenPrimary
	}

	override func awakeFromNib() {
		super.awakeFromNib()

		outputFormatField?.setAccessibilityLabel(NSLocalizedString("Audio output status", comment: "Audio output status accessibility label"))
		showAudioOutputStatus()
		NotificationCenter.default.addObserver(self, selector: #selector(audioOutputStatusDidChange(_:)), name: SignalStatusModel.didChangeNotification, object: nil)
		// Clicking the status opens the Signal Inspector.
		let click = NSClickGestureRecognizer(target: signalInspector, action: Selector(("toggleWindow:")))
		outputFormatField?.addGestureRecognizer(click)

		statusBar?.isHidden = UserDefaults.standard.bool(forKey: "hideStatusBar")

		playlistView?.nextResponder = self

		if !UserDefaults.standard.bool(forKey: "miniMode") {
			showSentryConsent(self)
		}
	}

	deinit {
		NotificationCenter.default.removeObserver(self, name: SignalStatusModel.didChangeNotification, object: nil)
	}

	@objc private func audioOutputStatusDidChange(_ notification: Notification) {
		showAudioOutputStatus()
	}

	// Whether Cog passes the decoded samples on unchanged, and in which formats
	// they go from the decoder through Core Audio to the device.
	private func showAudioOutputStatus() {
		let model = SignalStatusModel.shared
		outputFormatField?.stringValue = model.summary
		outputFormatField?.toolTip = model.toolTip
	}

	// The playlist's stack view detaches a hidden view, so the playlist takes
	// the bar's room.
	@IBAction func toggleStatusBar(_ sender: Any?) {
		let hide = !(statusBar?.isHidden ?? false)
		statusBar?.isHidden = hide
		UserDefaults.standard.set(hide, forKey: "hideStatusBar")
	}

	override func validateMenuItem(_ menuItem: NSMenuItem) -> Bool {
		if menuItem.action == #selector(toggleStatusBar(_:)) {
			menuItem.state = (statusBar?.isHidden ?? false) ? .off : .on
			return true
		}
		return super.validateMenuItem(menuItem)
	}

	@objc func focusSearch(_ sender: Any?) {
		makeFirstResponder(searchField)
		let range = NSRange(location: 0, length: (searchField?.stringValue as NSString?)?.length ?? 0)
		if let editor = searchField?.currentEditor() {
			editor.selectedRange = range
		}
	}

	@IBAction func openSearch(_ sender: Any?) {
		focusSearch(sender)
		// hack
		let timer = Timer.scheduledTimer(timeInterval: 0.125, target: self, selector: #selector(focusSearch(_:)), userInfo: nil, repeats: false)
		RunLoop.main.add(timer, forMode: .common)
	}
}
