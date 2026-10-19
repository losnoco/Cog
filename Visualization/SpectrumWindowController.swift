//
//  SpectrumWindowController.swift
//  Cog
//
//  Created by Christopher Snowhill on 5/22/22.
//  Rewritten in SwiftUI by Kevin López Brante on 2026-10-19.
//

import Cocoa
import SwiftUI

/// What the spectrum window shows: the 3D spectrum when it is chosen and
/// Metal can draw it, otherwise the flat one, moving only while the music
/// plays and the window is on screen.
@MainActor
final class SpectrumWindowModel: ObservableObject {
	/// While the window is open.
	@Published var isRunning = false
	@Published var status: CogStatus = .stopped
	@Published var isOccluded = false
	/// SceneKit's view follows playback and occlusion itself.
	@Published var sceneKitView: SpectrumViewSK?
}

/// Owns the spectrum window. Instantiated by MainMenu.xib, which connects
/// `playbackController` and targets showWindow:.
@objc(SpectrumWindowController)
final class SpectrumWindowController: NSWindowController, NSWindowDelegate {
	@IBOutlet var playbackController: PlaybackController?

	private let model = SpectrumWindowModel()
	private var sceneKitObservation: NSKeyValueObservation?

	@objc init() {
		super.init(window: nil)
	}

	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}

	override func loadWindow() {
		let window = NSWindow(
			contentRect: NSRect(x: 196, y: 240, width: 480, height: 270),
			styleMask: [.titled, .closable, .miniaturizable, .resizable],
			backing: .buffered,
			defer: true
		)
		window.title = NSLocalizedString("Spectrum", comment: "Spectrum window title")
		window.appearance = NSAppearance(named: .darkAqua)
		window.titlebarAppearsTransparent = true
		window.isReleasedWhenClosed = false
		window.contentView = NSHostingView(rootView: SpectrumWindowView(model: model))
		window.delegate = self
		self.window = window

		let center = NotificationCenter.default
		for name in ["CogPlaybackDidBeginNotificiation", "CogPlaybackDidPauseNotificiation",
		             "CogPlaybackDidResumeNotificiation", "CogPlaybackDidStopNotificiation"] {
			center.addObserver(self, selector: #selector(playbackStatusDidChange(_:)), name: Notification.Name(name), object: nil)
		}
		center.addObserver(self, selector: #selector(occlusionStateDidChange(_:)), name: NSWindow.didChangeOcclusionStateNotification, object: window)

		// Switching between 3D and flat in Settings swaps the view at once.
		sceneKitObservation = UserDefaults.standard.observe(\.spectrumSceneKit) { [weak self] _, _ in
			MainActor.assumeIsolated {
				guard let self, self.model.isRunning else { return }
				self.model.sceneKitView = nil
				self.startRunning()
			}
		}
	}

	private func startRunning() {
		model.status = playbackController?.playbackStatus ?? .stopped
		model.isRunning = true
		if model.sceneKitView == nil, let view = SpectrumViewSK.createGuard(withFrame: window?.contentLayoutRect ?? .zero) {
			view.enableCameraControl()
			if model.status == .playing {
				view.startPlayback()
			}
			model.sceneKitView = view
		}
	}

	private func stopRunning() {
		model.isRunning = false
		model.sceneKitView = nil
	}

	@objc private func playbackStatusDidChange(_ notification: Notification) {
		model.status = playbackController?.playbackStatus ?? .stopped
	}

	@objc private func occlusionStateDidChange(_ notification: Notification) {
		model.isOccluded = window.map { !$0.occlusionState.contains(.visible) } ?? true
	}

	func windowWillClose(_ notification: Notification) {
		stopRunning()
	}

	override func showWindow(_ sender: Any?) {
		// Without a nib, NSWindowController never calls loadWindow() itself.
		if window == nil {
			loadWindow()
		}
		startRunning()
		super.showWindow(sender)
	}

	@IBAction func toggleWindow(_ sender: Any?) {
		if window?.isVisible == true {
			window?.orderOut(self)
		} else {
			showWindow(self)
		}
	}
}

private extension UserDefaults {
	@objc dynamic var spectrumSceneKit: Bool { bool(forKey: "spectrumSceneKit") }
}

private struct SpectrumWindowView: View {
	@ObservedObject var model: SpectrumWindowModel
	@AppStorage("spectrumBarColor") private var barColorData: Data?
	@AppStorage("spectrumDotColor") private var dotColorData: Data?
	@AppStorage("spectrumFreqMode") private var frequencyMode = false

	var body: some View {
		if !model.isRunning {
			Color.clear
		} else if let view = model.sceneKitView {
			SceneKitSpectrum(view: view)
		} else if model.status != .stopped {
			SpectrumView(isPlaying: model.status == .playing && !model.isOccluded,
			             barColor: color(barColorData), peakColor: color(dotColorData),
			             isFullView: true, frequencyMode: frequencyMode)
				// A click switches between octave bands and frequencies.
				.contentShape(Rectangle())
				.onTapGesture { frequencyMode.toggle() }
		} else {
			Color.clear
		}
	}

	private func color(_ data: Data?) -> Color {
		Color(nsColor: ColorToValueTransformer().transformedValue(data) as? NSColor ?? .black)
	}
}

private struct SceneKitSpectrum: NSViewRepresentable {
	let view: SpectrumViewSK

	func makeNSView(context: Context) -> SpectrumViewSK {
		view
	}

	func updateNSView(_ nsView: SpectrumViewSK, context: Context) {}
}
