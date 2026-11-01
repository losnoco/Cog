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
