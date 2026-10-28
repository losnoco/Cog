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

/// Owns the spectrum, in its window or docked. Instantiated by MainMenu.xib,
/// which connects `playbackController` and targets showWindow:.
@objc(SpectrumWindowController)
final class SpectrumWindowController: PaneWindowController {
	@IBOutlet var playbackController: PlaybackController?

	private let model = SpectrumWindowModel()
	private var sceneKitObservation: NSKeyValueObservation?

	override var paneIdentifier: String {
		"spectrum"
	}

	override var paneTitle: String {
		NSLocalizedString("Spectrum", comment: "Spectrum window title")
	}

	override var defaultPlacement: PanePlacement {
		PanePlacement(edge: .bottom, size: 180)
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

		// Switching between 3D and flat in Settings swaps the view at once.
		sceneKitObservation = UserDefaults.standard.observe(\.spectrumSceneKit) { [weak self] _, _ in
			MainActor.assumeIsolated {
				guard let self, self.model.isRunning else { return }
				self.model.sceneKitView = nil
				self.startRunning()
			}
		}
	}

	override func makePaneView() -> NSView {
		hostingView(SpectrumWindowView(model: model))
	}

	override func makeWindow() -> NSWindow {
		let window = NSWindow(
			contentRect: NSRect(x: 196, y: 240, width: 480, height: 270),
			styleMask: [.titled, .closable, .miniaturizable, .resizable],
			backing: .buffered,
			defer: true
		)
		window.title = paneTitle
		window.appearance = NSAppearance(named: .darkAqua)
		window.titlebarAppearsTransparent = true
		window.isReleasedWhenClosed = false
		return window
	}

	/// Runs while the spectrum shows; covered, it stops drawing.
	override func paneVisibilityDidChange() {
		if isPaneOpen {
			if !model.isRunning {
				startRunning()
			}
		} else if model.isRunning {
			stopRunning()
		}
		let isOccluded = !isPaneVisible
		if model.isOccluded != isOccluded {
			model.isOccluded = isOccluded
		}
	}

	private func startRunning() {
		model.status = playbackController?.playbackStatus ?? .stopped
		model.isRunning = true
		if model.sceneKitView == nil, let view = SpectrumViewSK.createGuard(withFrame: paneView.bounds) {
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
				// A click switches between octave bands and frequencies, as
				// does the menu.
				.contentShape(Rectangle())
				.onTapGesture { frequencyMode.toggle() }
				.contextMenu {
					Picker(selection: $frequencyMode) {
						Text("Octave Bands", comment: "Spectrum menu item: bars for octave note bands").tag(false)
						Text("Frequencies", comment: "Spectrum menu item: a line for each frequency").tag(true)
					} label: {
						EmptyView()
					}
					.pickerStyle(.inline)
				}
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
