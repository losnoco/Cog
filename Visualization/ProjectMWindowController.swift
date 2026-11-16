//
//  ProjectMWindowController.swift
//  Cog
//
//  Created by Kevin López Brante on 2026-11-15.
//

import Cocoa
import SwiftUI

/// What the projectM pane shows: the visuals filling it, with the track and
/// the playback controls floating over them.
@MainActor
final class ProjectMModel: ObservableObject {
	/// While the pane is open and projectM could start.
	@Published var view: ProjectMView?
	@Published var status: CogStatus = .stopped
	@Published var entry: PlaylistEntry?
	@Published var position: Double = 0
	@Published var isSeekable = false
	@Published var volume: Double = 0
	/// The preset showing, by path, and the folder the presets come from.
	@Published var presetPath = ""
	@Published var presetFolder: URL?
	var presetName: String {
		URL(fileURLWithPath: presetPath).deletingPathExtension().lastPathComponent
	}
	/// Kept across launches, as is the preset showing.
	@Published var isPresetLocked = UserDefaults.standard.bool(forKey: "projectMPresetLocked") {
		didSet {
			view?.presetLocked = isPresetLocked
			UserDefaults.standard.set(isPresetLocked, forKey: "projectMPresetLocked")
		}
	}

	/// The controls show after the mouse moves and fade when it rests.
	@Published var showsControls = true
	/// The controls exist only in full screen; docked or in a window, the
	/// pane shows the visuals and the caption alone.
	@Published var isFullScreen = false

	var showsHUD: Bool {
		isFullScreen && showsControls
	}

	/// How long the caption stays at each end of a track.
	static let captionSeconds = 8.0

	/// As a music video's caption: the track for its first and last seconds,
	/// unless the controls already show it.
	var showsCaption: Bool {
		guard let entry, status != .stopped, !showsHUD else { return false }
		if position < Self.captionSeconds { return true }
		let length = entry.length.doubleValue
		return length > Self.captionSeconds * 3 && length - position < Self.captionSeconds
	}

	weak var playbackController: PlaybackController?

	func playPause() {
		playbackController?.playPauseResume(nil)
	}

	func previous() {
		playbackController?.prev(nil)
	}

	func next() {
		playbackController?.next(nil)
	}

	func seek(to time: Double) {
		position = time
		playbackController?.seek(nil, toTime: time)
	}

	func setVolume(_ volume: Double) {
		self.volume = volume
		playbackController?.setVolume(volume)
	}
}

/// Owns projectM, in its window or docked. Instantiated by MainMenu.xib,
/// which connects the controllers and targets showWindow:.
@objc(ProjectMWindowController)
final class ProjectMWindowController: PaneWindowController {
	@IBOutlet var playbackController: PlaybackController?
	@IBOutlet var currentEntryController: NSObjectController?

	private let model = ProjectMModel()
	private var observations: [NSKeyValueObservation] = []
	private var positionTimer: Timer?
	private var idleTimer: Timer?
	private var mouseMonitor: Any?
	private var userFolderAccess: UnsafeRawPointer?
	/// The seizure warning was accepted since launch.
	private var warningAccepted = false
	private var isWarning = false

	override var paneIdentifier: String {
		"projectM"
	}

	override var paneTitle: String {
		NSLocalizedString("projectM", comment: "projectM visualizer window title")
	}

	override var defaultPlacement: PanePlacement {
		PanePlacement(edge: .bottom, size: 240)
	}

	override var paneAppearance: NSAppearance? {
		NSAppearance(named: .darkAqua)
	}

	override func awakeFromNib() {
		super.awakeFromNib()
		// Nib loading happens on the main thread.
		MainActor.assumeIsolated {
			model.playbackController = playbackController
			let center = NotificationCenter.default
			for name in ["CogPlaybackDidBeginNotificiation", "CogPlaybackDidPauseNotificiation",
			             "CogPlaybackDidResumeNotificiation", "CogPlaybackDidStopNotificiation"] {
				center.addObserver(self, selector: #selector(playbackStatusDidChange(_:)), name: Notification.Name(name), object: nil)
			}
			if let currentEntryController {
				observations.append(currentEntryController.observe(\.content, options: [.initial]) { [weak self] _, _ in
					MainActor.assumeIsolated { self?.updateEntry() }
				})
			}
			let defaults = UserDefaults.standard
			observations.append(defaults.observe(\.projectMShuffle) { [weak self] _, _ in
				MainActor.assumeIsolated { self?.model.view?.shuffle = UserDefaults.standard.projectMShuffle }
			})
			observations.append(defaults.observe(\.projectMPresetDuration) { [weak self] _, _ in
				MainActor.assumeIsolated { self?.model.view?.presetDuration = UserDefaults.standard.projectMPresetDuration }
			})
			for keyPath in [\UserDefaults.projectMCrossfade, \UserDefaults.projectMHardCutThreshold] {
				observations.append(defaults.observe(keyPath) { [weak self] _, _ in
					MainActor.assumeIsolated { self?.applyTransitionSettings() }
				})
			}
			observations.append(defaults.observe(\.projectMHardCuts) { [weak self] _, _ in
				MainActor.assumeIsolated { self?.applyTransitionSettings() }
			})
			for keyPath in [\UserDefaults.projectMDetail, \UserDefaults.projectMResolution, \UserDefaults.projectMFrameRate] {
				observations.append(defaults.observe(keyPath) { [weak self] _, _ in
					MainActor.assumeIsolated { self?.applyTransitionSettings() }
				})
			}
			observations.append(defaults.observe(\.projectMExcludedThemes) { [weak self] _, _ in
				MainActor.assumeIsolated { self?.model.view?.excludedThemes = UserDefaults.standard.projectMExcludedThemes }
			})
			observations.append(defaults.observe(\.projectMPresetFolder) { [weak self] _, _ in
				MainActor.assumeIsolated { self?.updatePresetFolders() }
			})
			for name in [NSWindow.didEnterFullScreenNotification, NSWindow.didExitFullScreenNotification] {
				center.addObserver(self, selector: #selector(fullScreenDidChange(_:)), name: name, object: nil)
			}
		}
	}

	override func makePaneView() -> NSView {
		hostingView(ProjectMPaneView(model: model))
	}

	override func makeWindow() -> NSWindow {
		let window = NSWindow(
			contentRect: NSRect(x: 196, y: 240, width: 640, height: 400),
			styleMask: [.titled, .closable, .miniaturizable, .resizable, .fullSizeContentView],
			backing: .buffered,
			defer: true
		)
		window.title = paneTitle
		window.appearance = NSAppearance(named: .darkAqua)
		window.titlebarAppearsTransparent = true
		window.titleVisibility = .hidden
		window.collectionBehavior.insert(.fullScreenPrimary)
		window.isReleasedWhenClosed = false
		return window
	}

	// MARK: - The seizure warning

	override func showWindow(_ sender: Any?) {
		guard isPaneOpen || confirmSeizureWarning() else { return }
		super.showWindow(sender)
	}

	override func toggleWindow(_ sender: Any?) {
		guard isPaneOpen || confirmSeizureWarning() else { return }
		super.toggleWindow(sender)
	}

	private var mayStart: Bool {
		warningAccepted || UserDefaults.standard.bool(forKey: "projectMSeizureWarningSuppressed")
	}

	/// Warns of the flashing before projectM first shows in a session, unless
	/// told not to. True to go on.
	private func confirmSeizureWarning() -> Bool {
		if mayStart {
			return true
		}
		let alert = NSAlert()
		alert.alertStyle = .warning
		alert.messageText = NSLocalizedString("Photosensitive Seizure Warning", comment: "projectM warning title")
		alert.informativeText = NSLocalizedString("projectM shows flashing lights, strobing colors and fast-moving patterns. A small number of people may have seizures when they see such images, even with no history of epilepsy or seizures.\n\nIf you or anyone watching has had seizures or epilepsy, ask a doctor before using it. Stop watching at once if you feel dizzy or disoriented, or have altered vision, eye or muscle twitching, involuntary movements or convulsions.",
		                                          comment: "projectM photosensitive seizure warning")
		alert.addButton(withTitle: NSLocalizedString("Open projectM", comment: "projectM warning button"))
		alert.addButton(withTitle: NSLocalizedString("Cancel", comment: ""))
		alert.showsSuppressionButton = true
		alert.suppressionButton?.title = NSLocalizedString("Don’t warn me again", comment: "projectM warning checkbox")
		guard alert.runModal() == .alertFirstButtonReturn else { return false }
		warningAccepted = true
		if alert.suppressionButton?.state == .on {
			UserDefaults.standard.set(true, forKey: "projectMSeizureWarningSuppressed")
		}
		return true
	}

	/// A pane restored docked at launch opened without showWindow:; it is
	/// warned about once the app is up, and closed again if declined.
	private func warnBeforeStarting() {
		guard !isWarning else { return }
		isWarning = true
		DispatchQueue.main.async { [weak self] in
			guard let self else { return }
			MainActor.assumeIsolated {
				let accepted = self.confirmSeizureWarning()
				self.isWarning = false
				if accepted {
					self.paneVisibilityDidChange()
				} else if PaneManager.shared.isDocked(self) {
					PaneManager.shared.hide(self)
				} else {
					self.hideFloating()
				}
			}
		}
	}

	// MARK: - Showing

	/// Draws while the pane can be seen and the music plays; frees projectM
	/// once the pane closes.
	override func paneVisibilityDidChange() {
		if isPaneOpen {
			if model.view == nil {
				if mayStart {
					start()
				} else {
					warnBeforeStarting()
				}
			}
		} else if model.view != nil {
			stop()
		}
		if model.view != nil {
			// Docking moves the pane out of a full screen window.
			updateFullScreen()
		}
		updateRunning()
	}

	private func start() {
		guard let view = ProjectMView.create(withFrame: paneView.bounds) else { return }
		let defaults = UserDefaults.standard
		view.shuffle = defaults.projectMShuffle
		view.presetDuration = defaults.projectMPresetDuration
		view.presetLocked = model.isPresetLocked
		view.excludedThemes = defaults.projectMExcludedThemes
		view.startingPresetPath = defaults.string(forKey: "projectMLastPreset")
		view.presetDidChange = { [weak self] path in
			MainActor.assumeIsolated {
				self?.model.presetPath = path
				UserDefaults.standard.set(path, forKey: "projectMLastPreset")
			}
		}
		model.view = view
		applyTransitionSettings()
		updatePresetFolders()

		model.status = playbackController?.playbackStatus ?? .stopped
		updatePosition()
		positionTimer = Timer.scheduledTimer(withTimeInterval: 0.25, repeats: true) { [weak self] _ in
			MainActor.assumeIsolated { self?.updatePosition() }
		}
		mouseMonitor = NSEvent.addLocalMonitorForEvents(matching: [.mouseMoved, .leftMouseDown, .scrollWheel]) { [weak self] event in
			MainActor.assumeIsolated {
				if let self, self.model.isFullScreen, event.window != nil, event.window === self.paneView.window {
					self.showControls()
				}
			}
			return event
		}
		paneView.window?.acceptsMouseMovedEvents = true
		updateFullScreen()
	}

	private func updateFullScreen() {
		let isFullScreen = paneView.window?.styleMask.contains(.fullScreen) == true
		guard model.isFullScreen != isFullScreen else { return }
		model.isFullScreen = isFullScreen
		if isFullScreen {
			showControls()
		} else {
			idleTimer?.invalidate()
			idleTimer = nil
		}
	}

	@objc private func fullScreenDidChange(_ notification: Notification) {
		guard let window = notification.object as? NSWindow, window === paneView.window else { return }
		updateFullScreen()
	}

	private func stop() {
		model.view?.shutdown()
		model.view = nil
		positionTimer?.invalidate()
		positionTimer = nil
		idleTimer?.invalidate()
		idleTimer = nil
		if let mouseMonitor {
			NSEvent.removeMonitor(mouseMonitor)
		}
		mouseMonitor = nil
		endUserFolderAccess()
	}

	private func updateEntry() {
		model.entry = currentEntryController?.content as? PlaylistEntry
	}

	private func updateRunning() {
		model.view?.isRunning = isPaneVisible && model.status == .playing
	}

	private func updatePosition() {
		guard let playbackController else { return }
		model.position = playbackController.position()
		model.isSeekable = playbackController.seekable()
		model.volume = playbackController.volume()
	}

	/// The bundled presets, and the user's folder when one is chosen.
	private func updatePresetFolders() {
		guard let view = model.view else { return }
		var presets: [URL] = []
		var textures: [URL] = []
		if let bundled = Bundle.main.url(forResource: "projectM", withExtension: nil) {
			presets.append(bundled.appendingPathComponent("presets", isDirectory: true))
			textures.append(bundled.appendingPathComponent("textures", isDirectory: true))
		}
		endUserFolderAccess()
		let path = UserDefaults.standard.projectMPresetFolder
		if !path.isEmpty {
			let folder = URL(fileURLWithPath: path, isDirectory: true)
			// Held while the pane is open: projectM reads each preset as it
			// switches to it.
			userFolderAccess = SandboxBroker.shared().beginFolderAccess(folder)
			presets = [folder]
			textures.insert(folder, at: 0)
		}
		view.textureFolders = textures
		model.presetFolder = presets.first
		view.presetFolders = presets
	}

	/// How presets change, and how finely and how often they draw.
	private func applyTransitionSettings() {
		guard let view = model.view else { return }
		let defaults = UserDefaults.standard
		view.crossfadeDuration = defaults.projectMCrossfade
		view.hardCutsEnabled = defaults.projectMHardCuts
		view.hardCutThreshold = defaults.projectMHardCutThreshold
		view.meshSize = ProjectMDetail(rawValue: defaults.projectMDetail)?.meshSize ?? ProjectMDetail.standard.meshSize
		view.renderScale = [1, 2, 4].contains(defaults.projectMResolution) ? defaults.projectMResolution : 1
		view.frameRateLimit = Double(max(defaults.projectMFrameRate, 0))
	}

	private func endUserFolderAccess() {
		if let userFolderAccess {
			SandboxBroker.shared().endFolderAccess(userFolderAccess)
		}
		userFolderAccess = nil
	}

	private func showControls() {
		guard model.isFullScreen else { return }
		if !model.showsControls {
			withAnimation(.easeOut(duration: 0.2)) { model.showsControls = true }
		}
		idleTimer?.invalidate()
		idleTimer = Timer.scheduledTimer(withTimeInterval: 3, repeats: false) { [weak self] _ in
			MainActor.assumeIsolated { self?.hideControls() }
		}
	}

	private func hideControls() {
		// Not while stopped: there is nothing to watch.
		guard model.status != .stopped else { return }
		withAnimation(.easeIn(duration: 0.6)) { model.showsControls = false }
		NSCursor.setHiddenUntilMouseMoves(true)
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
			showControls()
		}
		updateRunning()
	}
}

/// How finely projectM draws its per-pixel effects; finer costs more GPU.
enum ProjectMDetail: Int, CaseIterable {
	case low, standard, high

	var meshSize: NSSize {
		switch self {
		case .low: NSSize(width: 16, height: 12)
		case .standard: NSSize(width: 32, height: 24)
		case .high: NSSize(width: 64, height: 48)
		}
	}
}

extension UserDefaults {
	@objc dynamic var projectMShuffle: Bool { bool(forKey: "projectMShuffle") }
	@objc dynamic var projectMPresetDuration: Double { double(forKey: "projectMPresetDuration") }
	@objc dynamic var projectMCrossfade: Double { double(forKey: "projectMCrossfade") }
	@objc dynamic var projectMHardCuts: Bool { bool(forKey: "projectMHardCuts") }
	@objc dynamic var projectMHardCutThreshold: Double { double(forKey: "projectMHardCutThreshold") }
	@objc dynamic var projectMDetail: Int { integer(forKey: "projectMDetail") }
	/// 1 for full resolution, 2 for half, 4 for quarter.
	@objc dynamic var projectMResolution: Int { integer(forKey: "projectMResolution") }
	/// Frames per second at most; 0 for every display refresh.
	@objc dynamic var projectMFrameRate: Int { integer(forKey: "projectMFrameRate") }
	@objc dynamic var projectMExcludedThemes: [String] { stringArray(forKey: "projectMExcludedThemes") ?? [] }
	@objc dynamic var projectMPresetFolder: String { string(forKey: "projectMPresetFolder") ?? "" }
}

// MARK: - The pane

private struct ProjectMPaneView: View {
	@ObservedObject var model: ProjectMModel

	var body: some View {
		ZStack {
			Color.black
			if let view = model.view {
				ProjectMSurface(view: view)
			}
			caption
			if model.isFullScreen {
				ProjectMHUD(model: model)
					.opacity(model.showsControls ? 1 : 0)
					.allowsHitTesting(model.showsControls)
			}
			// Always there in a window or docked; in full screen it comes and
			// goes with the controls.
			ProjectMPresetPicker(model: model)
				.frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topTrailing)
				.padding(12)
				.opacity(showsPresetPicker ? 1 : 0)
				.allowsHitTesting(showsPresetPicker)
		}
	}

	private var showsPresetPicker: Bool {
		!model.isFullScreen || model.showsControls
	}

	/// Blurs in, out, and from one track's caption to the next's.
	private var caption: some View {
		ZStack(alignment: .bottomLeading) {
			if model.showsCaption, let entry = model.entry {
				ProjectMCaption(entry: entry)
					.id(ObjectIdentifier(entry))
					.blurReplaceTransition()
			}
		}
		.frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .bottomLeading)
		.animation(.easeInOut(duration: 0.8), value: model.showsCaption ? model.entry.map(ObjectIdentifier.init) : nil)
		.allowsHitTesting(false)
	}
}

private extension View {
	/// The blur replace transition where the system has it.
	@ViewBuilder func blurReplaceTransition() -> some View {
		if #available(macOS 14.0, *) {
			transition(.blurReplace)
		} else {
			transition(.opacity)
		}
	}
}

/// The track in the lower left corner, as music television captions a
/// video: artist, "title", album.
private struct ProjectMCaption: View {
	let entry: PlaylistEntry

	var body: some View {
		VStack(alignment: .leading, spacing: 1) {
			if let artist = entry.artist, !artist.isEmpty {
				Text(artist)
					.fontWeight(.semibold)
			}
			if !entry.title.isEmpty {
				Text(verbatim: "“\(entry.title)”")
			}
			if let album = entry.album, !album.isEmpty {
				Text(album)
					.font(.system(size: 15))
					.opacity(0.85)
			}
		}
		.font(.system(size: 18))
		.foregroundColor(.white)
		.lineLimit(1)
		.shadow(color: .black.opacity(0.9), radius: 2, x: 1, y: 1)
		.padding(.leading, 28)
		.padding(.bottom, 24)
	}
}

private struct ProjectMSurface: NSViewRepresentable {
	let view: ProjectMView

	func makeNSView(context: Context) -> ProjectMView {
		view
	}

	func updateNSView(_ nsView: ProjectMView, context: Context) {}
}

/// The preset showing, with the way to the previous and next and to keep it.
private struct ProjectMPresetPicker: View {
	@ObservedObject var model: ProjectMModel
	@State private var showsBrowser = false

	var body: some View {
		HStack(spacing: 10) {
			Button(action: { model.view?.previousPreset() }) {
				Image(systemName: "chevron.backward")
			}
			.help(Text("Previous Preset", comment: "projectM HUD button"))
			// The name opens the list of presets, by folder.
			Button(action: { showsBrowser.toggle() }) {
				HStack(spacing: 4) {
					Text(model.presetName)
						.lineLimit(1)
						.truncationMode(.middle)
					Image(systemName: "chevron.down")
						.imageScale(.small)
				}
				.font(.caption)
				.frame(maxWidth: 260)
			}
			.help(Text("Choose a Preset", comment: "projectM HUD button: open the list of presets"))
			.popover(isPresented: $showsBrowser, arrowEdge: .bottom) {
				ProjectMPresetBrowser(model: model)
			}
			Button(action: { model.view?.nextPreset() }) {
				Image(systemName: "chevron.forward")
			}
			.help(Text("Next Preset", comment: "projectM HUD button"))
			Button(action: { model.isPresetLocked.toggle() }) {
				Image(systemName: model.isPresetLocked ? "lock.fill" : "lock.open")
			}
			.help(model.isPresetLocked
				? Text("Unlock Preset", comment: "projectM HUD button: let presets change again")
				: Text("Lock Preset", comment: "projectM HUD button: keep the current preset"))
		}
		.buttonStyle(.plain)
		.padding(.horizontal, 12)
		.padding(.vertical, 6)
		.background(.ultraThinMaterial, in: Capsule())
		.environment(\.colorScheme, .dark)
	}
}

/// A preset, or a folder of them, in the browser.
private struct ProjectMPresetNode: Identifiable {
	let id: String
	let name: String
	/// The preset's place in projectM's playlist; nil for a folder.
	let index: Int?
	var children: [ProjectMPresetNode]?

	/// The presets as the folders under `root` hold them: folders first,
	/// then presets, each in Finder order.
	static func tree(paths: [String], root: URL?) -> [ProjectMPresetNode] {
		final class Folder {
			var folders: [String: Folder] = [:]
			var presets: [ProjectMPresetNode] = []
		}
		let top = Folder()
		let prefix = root.map { $0.standardizedFileURL.path + "/" }
		for (index, path) in paths.enumerated() {
			var relative = path
			if let prefix, path.hasPrefix(prefix) {
				relative = String(path.dropFirst(prefix.count))
			}
			var components = relative.split(separator: "/").map(String.init)
			guard let file = components.popLast() else { continue }
			var folder = top
			for component in components {
				if let next = folder.folders[component] {
					folder = next
				} else {
					let next = Folder()
					folder.folders[component] = next
					folder = next
				}
			}
			let name = (file as NSString).deletingPathExtension
			folder.presets.append(ProjectMPresetNode(id: path, name: name, index: index, children: nil))
		}
		func nodes(_ folder: Folder, path: String) -> [ProjectMPresetNode] {
			let folders = folder.folders.keys.sorted { $0.localizedStandardCompare($1) == .orderedAscending }.map { name in
				ProjectMPresetNode(id: path + "/" + name, name: name, index: nil, children: nodes(folder.folders[name]!, path: path + "/" + name))
			}
			let presets = folder.presets.sorted { $0.name.localizedStandardCompare($1.name) == .orderedAscending }
			return folders + presets
		}
		return nodes(top, path: "")
	}

	var presetCount: Int {
		children?.reduce(0) { $0 + $1.presetCount } ?? 1
	}
}

/// Every preset in the playlist, by folder; a click shows it.
private struct ProjectMPresetBrowser: View {
	@ObservedObject var model: ProjectMModel
	@State private var tree: [ProjectMPresetNode] = []
	@State private var presets: [ProjectMPresetNode] = []
	@State private var search = ""
	@State private var isLoading = true

	var body: some View {
		VStack(spacing: 0) {
			TextField(String(localized: "Search", comment: "projectM preset browser search field"), text: $search)
				.textFieldStyle(.roundedBorder)
				.padding(8)
			if isLoading {
				ProgressView()
					.frame(maxWidth: .infinity, maxHeight: .infinity)
			} else if search.isEmpty {
				List(tree, children: \.children) { node in
					row(node)
				}
			} else {
				List(presets.filter { $0.name.localizedCaseInsensitiveContains(search) }) { node in
					row(node)
				}
			}
		}
		.frame(width: 380, height: 480)
		.onAppear(perform: load)
	}

	@ViewBuilder private func row(_ node: ProjectMPresetNode) -> some View {
		if let index = node.index {
			Button(action: { model.view?.selectPreset(at: UInt(index)) }) {
				HStack {
					Text(node.name)
						.font(.body.weight(node.id == model.presetPath ? .semibold : .regular))
						.lineLimit(1)
						.truncationMode(.middle)
					Spacer()
					if node.id == model.presetPath {
						Image(systemName: "sparkles")
							.foregroundColor(.accentColor)
					}
				}
				.contentShape(Rectangle())
			}
			.buttonStyle(.plain)
		} else {
			HStack {
				Label(node.name, systemImage: "folder")
				Spacer()
				Text(node.presetCount, format: .number)
					.foregroundColor(.secondary)
					.monospacedDigit()
			}
		}
	}

	private func load() {
		guard let view = model.view else {
			isLoading = false
			return
		}
		view.fetchPresets { paths, _ in
			let tree = ProjectMPresetNode.tree(paths: paths, root: model.presetFolder)
			self.tree = tree
			presets = paths.enumerated().map { index, path in
				ProjectMPresetNode(id: path, name: URL(fileURLWithPath: path).deletingPathExtension().lastPathComponent, index: index, children: nil)
			}
			isLoading = false
		}
	}
}

/// The controls over the visuals in full screen: the track and transport at
/// the bottom.
private struct ProjectMHUD: View {
	@ObservedObject var model: ProjectMModel
	@State private var scrubbing: Double?

	var body: some View {
		VStack(spacing: 0) {
			Spacer(minLength: 8)
			transport
		}
		.padding(12)
		.environment(\.colorScheme, .dark)
	}

	private var transport: some View {
		VStack(alignment: .leading, spacing: 8) {
			HStack(alignment: .firstTextBaseline) {
				VStack(alignment: .leading, spacing: 2) {
					Text(model.entry?.title ?? "")
						.font(.headline)
					Text([model.entry?.artist, model.entry?.album].compactMap { $0 }.filter { !$0.isEmpty }.joined(separator: " — "))
						.font(.subheadline)
						.foregroundStyle(.secondary)
				}
				.lineLimit(1)
				Spacer()
				Image(systemName: "speaker.fill")
					.foregroundStyle(.secondary)
				Slider(value: Binding(get: { model.volume }, set: { model.setVolume($0) }), in: 0...100)
					.controlSize(.small)
					.frame(width: 90)
			}
			HStack(spacing: 14) {
				Button(action: model.previous) {
					Image(systemName: "backward.fill")
				}
				.help(Text("Previous"))
				Button(action: model.playPause) {
					Image(systemName: model.status == .playing ? "pause.fill" : "play.fill")
						.font(.title2)
						.frame(width: 24)
				}
				.help(model.status == .playing
					? Text("Pause", comment: "projectM HUD button")
					: Text("Play", comment: "projectM HUD button"))
				Button(action: model.next) {
					Image(systemName: "forward.fill")
				}
				.help(Text("Next"))
				Text(Self.time(scrubbing ?? model.position))
					.monospacedDigit()
					.font(.caption)
				Slider(value: Binding(get: { scrubbing ?? model.position }, set: { scrubbing = $0 }),
				       in: 0...max(length, 1)) { editing in
					if !editing, let time = scrubbing {
						model.seek(to: time)
						scrubbing = nil
					}
				}
				.controlSize(.small)
				.disabled(!model.isSeekable || length <= 0)
				Text(Self.time(length))
					.monospacedDigit()
					.font(.caption)
			}
			.buttonStyle(.plain)
		}
		.padding(14)
		.background(.ultraThinMaterial, in: RoundedRectangle(cornerRadius: 14, style: .continuous))
		.frame(maxWidth: 720)
		.frame(maxWidth: .infinity)
	}

	private var length: Double {
		model.entry?.length.doubleValue ?? 0
	}

	private static func time(_ seconds: Double) -> String {
		guard seconds.isFinite, seconds >= 0 else { return "0:00" }
		let total = Int(seconds)
		return total >= 3600
			? String(format: "%d:%02d:%02d", total / 3600, total / 60 % 60, total % 60)
			: String(format: "%d:%02d", total / 60, total % 60)
	}
}
