//
//  WaveformWindowController.swift
//  Cog
//
//  Created by Kevin López Brante on 2026-11-01.
//

import Cocoa
import SwiftUI

/// What the waveform pane shows: moving only while the music plays and the
/// pane can be seen.
@MainActor
final class WaveformWindowModel: ObservableObject {
	@Published var status: CogStatus = .stopped
	@Published var isVisible = false
}

/// Owns the waveform visualizer, in its window or docked. Instantiated by
/// MainMenu.xib, which connects `playbackController` and targets showWindow:.
@objc(WaveformWindowController)
final class WaveformWindowController: PaneWindowController {
	@IBOutlet var playbackController: PlaybackController?

	private let model = WaveformWindowModel()

	override var paneIdentifier: String {
		"waveform"
	}

	override var paneTitle: String {
		NSLocalizedString("Waveform", comment: "Waveform visualizer window title")
	}

	override var defaultPlacement: PanePlacement {
		PanePlacement(edge: .bottom, position: 2, size: 150)
	}

	override var paneAppearance: NSAppearance? {
		NSAppearance(named: .darkAqua)
	}

	override func awakeFromNib() {
		super.awakeFromNib()
		let center = NotificationCenter.default
		for name in ["CogPlaybackDidBeginNotificiation", "CogPlaybackDidPauseNotificiation",
		             "CogPlaybackDidResumeNotificiation", "CogPlaybackDidStopNotificiation"] {
			center.addObserver(self, selector: #selector(playbackStatusDidChange(_:)), name: Notification.Name(name), object: nil)
		}
	}

	override func makePaneView() -> NSView {
		hostingView(WaveformWindowView(model: model))
	}

	override func makeWindow() -> NSWindow {
		let window = NSWindow(
			contentRect: NSRect(x: 196, y: 240, width: 480, height: 150),
			styleMask: [.titled, .closable, .miniaturizable, .resizable],
			backing: .buffered,
			defer: true
		)
		window.title = paneTitle
		window.appearance = NSAppearance(named: .darkAqua)
		window.titlebarAppearsTransparent = true
		window.isReleasedWhenClosed = false
		window.contentMinSize = NSSize(width: 160, height: 60)
		window.setFrameAutosaveName("Waveform")
		return window
	}

	override func paneVisibilityDidChange() {
		if model.isVisible != isPaneVisible {
			model.isVisible = isPaneVisible
		}
		if isPaneVisible {
			model.status = playbackController?.playbackStatus ?? .stopped
		}
	}

	@objc private func playbackStatusDidChange(_ notification: Notification) {
		// The notifications come before PlaybackController's status changes,
		// so the status is the notification's.
		switch notification.name.rawValue {
		case "CogPlaybackDidBeginNotificiation", "CogPlaybackDidResumeNotificiation":
			model.status = .playing
		case "CogPlaybackDidPauseNotificiation":
			model.status = .paused
		default:
			model.status = .stopped
		}
	}
}

private struct WaveformWindowView: View {
	@ObservedObject var model: WaveformWindowModel
	@AppStorage("waveformVisualizerColor") private var colorData: Data?
	@AppStorage("waveformVisualizerMode") private var mode = WaveformView.Mode.oscilloscope

	var body: some View {
		WaveformView(isPlaying: model.status == .playing && model.isVisible,
		             lineColor: color, mode: mode, showsCenterLine: true)
			// Stopped, the last frame would linger: draw silence instead.
			.id(model.status == .stopped)
			// A click switches between the two, as does the menu.
			.contentShape(Rectangle())
			.onTapGesture { mode = mode == .oscilloscope ? .envelope : .oscilloscope }
			.contextMenu {
				Picker(selection: $mode) {
					Text("Oscilloscope", comment: "Waveform menu item: a trace of the last moment of sound").tag(WaveformView.Mode.oscilloscope)
					Text("Envelope", comment: "Waveform menu item: the last few seconds' peaks, scrolling").tag(WaveformView.Mode.envelope)
				} label: {
					EmptyView()
				}
				.pickerStyle(.inline)
			}
	}

	private var color: Color {
		Color(nsColor: ColorToValueTransformer().transformedValue(colorData) as? NSColor ?? .orange)
	}
}

/// The waveform's toolbar item: a small trace in the waveform pane's color
/// and mode, which opens the waveform window when clicked.
@objc(WaveformItem)
final class WaveformItem: NSToolbarItem {
	@IBOutlet var playbackController: PlaybackController?
	@IBOutlet var windowController: WaveformWindowController?

	override func awakeFromNib() {
		super.awakeFromNib()
		// Nibs load on the main thread.
		MainActor.assumeIsolated {
			let view = WaveformItemView(frame: NSRect(x: 0, y: 0, width: 64, height: 26))
			view.setStatus(playbackController?.playbackStatus ?? .stopped)
			view.onClick = { [weak self] in
				self?.windowController?.showWindow(nil)
			}
			self.view = view
		}
	}
}

/// Draws while playing and while its window is on screen.
final class WaveformItemView: NSView {
	var onClick: (() -> Void)?

	private let model = WaveformWindowModel()
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
		let host = NSHostingView(rootView: WaveformItemContent(model: model))
		host.frame = bounds
		host.autoresizingMask = [.width, .height]
		addSubview(host)
		toolTip = NSLocalizedString("Waveform. Click to open the waveform.", comment: "Waveform toolbar item tooltip")

		let center = NotificationCenter.default
		for name in ["CogPlaybackDidBeginNotificiation", "CogPlaybackDidPauseNotificiation",
		             "CogPlaybackDidResumeNotificiation", "CogPlaybackDidStopNotificiation"] {
			center.addObserver(self, selector: #selector(playbackStatusDidChange(_:)), name: Notification.Name(name), object: nil)
		}
	}

	func setStatus(_ status: CogStatus) {
		if model.status != status {
			model.status = status
		}
	}

	@objc private func playbackStatusDidChange(_ notification: Notification) {
		// As in the pane: the notification, not PlaybackController, has the
		// new status.
		switch notification.name.rawValue {
		case "CogPlaybackDidBeginNotificiation", "CogPlaybackDidResumeNotificiation":
			setStatus(.playing)
		case "CogPlaybackDidPauseNotificiation":
			setStatus(.paused)
		default:
			setStatus(.stopped)
		}
	}

	override func viewDidMoveToWindow() {
		super.viewDidMoveToWindow()
		if let occlusionObserver {
			NotificationCenter.default.removeObserver(occlusionObserver)
		}
		occlusionObserver = window.map { window in
			NotificationCenter.default.addObserver(forName: NSWindow.didChangeOcclusionStateNotification, object: window, queue: .main) { [weak self] _ in
				MainActor.assumeIsolated { self?.updateVisibility() }
			}
		}
		updateVisibility()
	}

	private func updateVisibility() {
		let visible = window.map { $0.isVisible && $0.occlusionState.contains(.visible) } ?? false
		if model.isVisible != visible {
			model.isVisible = visible
		}
	}

	// The trace takes no clicks of its own; the item opens the window.
	override func hitTest(_ point: NSPoint) -> NSView? {
		frame.contains(point) ? self : nil
	}

	override func mouseDown(with event: NSEvent) {
		onClick?()
	}
}

private struct WaveformItemContent: View {
	@ObservedObject var model: WaveformWindowModel
	@AppStorage("waveformVisualizerColor") private var colorData: Data?
	@AppStorage("waveformVisualizerMode") private var mode = WaveformView.Mode.oscilloscope

	var body: some View {
		WaveformView(isPlaying: model.status == .playing && model.isVisible,
		             lineColor: color, mode: mode)
			.id(model.status == .stopped)
	}

	private var color: Color {
		Color(nsColor: ColorToValueTransformer().transformedValue(colorData) as? NSColor ?? .orange)
	}
}
