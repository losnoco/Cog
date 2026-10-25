//
//  PlaylistTabs.swift
//  Cog
//
//  The tabs above the playlist, one per playlist: click to show one (the
//  one playing keeps playing), double-click to rename, drag to reorder, and
//  drop files or rows on one to add them there (rows are copied, or moved
//  with Option). Also the playlist commands' menu items, made here so the
//  menus need no outlets for them.
//

import Cocoa
import CogPlaylist
import SwiftUI

@MainActor
final class PlaylistTabsModel: ObservableObject {
	struct Tab: Identifiable, Equatable {
		let id: UUID
		let name: String
		let playlist: Playlist
	}

	@Published private(set) var tabs: [Tab] = []
	@Published private(set) var shownID: UUID?
	@Published private(set) var playingID: UUID?
	/// The tab whose name is being edited in place.
	@Published var renamingID: UUID?
	/// Where a drag would drop: a tab, or the new playlist button.
	@Published var dropTargetID: UUID?
	@Published var dropsOnNewPlaylist = false

	/// Each tab's frame, and the new playlist button's, in the tabs' space,
	/// for finding what a drag is over.
	var tabFrames: [UUID: CGRect] = [:]
	var newPlaylistFrame: CGRect = .zero

	/// Marks a tab's own drag, as text.
	static let dragPrefix = "org.cogx.cog.playlist-tab:"

	let controller: PlaylistController
	private var observations: [NSKeyValueObservation] = []
	private var notifications: [NSObjectProtocol] = []

	init(controller: PlaylistController) {
		self.controller = controller
		observations = [
			controller.observe(\.shownPlaylist) { [weak self] _, _ in
				DispatchQueue.main.async { self?.refresh() }
			},
			controller.observe(\.currentEntry) { [weak self] _, _ in
				DispatchQueue.main.async { self?.refresh() }
			}
		]
		let center = NotificationCenter.default
		notifications = [
			center.addObserver(forName: .cogPlaylistsDidChange, object: nil, queue: .main) { [weak self] _ in
				MainActor.assumeIsolated { self?.refresh() }
			},
			center.addObserver(forName: .cogRenamePlaylist, object: nil, queue: .main) { [weak self] note in
				let uuid = (note.object as? Playlist)?.uuid
				MainActor.assumeIsolated {
					guard let uuid else { return }
					self?.refresh()
					self?.renamingID = uuid
				}
			}
		]
		refresh()
	}

	func refresh() {
		let newTabs = controller.playlists.compactMap { playlist in
			playlist.uuid.map { Tab(id: $0, name: playlist.name ?? "", playlist: playlist) }
		}
		if newTabs != tabs { tabs = newTabs }
		let shown = controller.shownPlaylist?.uuid
		if shown != shownID { shownID = shown }
		let playing = controller.playingPlaylist?.uuid
		if playing != playingID { playingID = playing }
	}

	func show(_ tab: Tab) {
		controller.show(tab.playlist)
	}

	func rename(_ tab: Tab, to name: String) {
		renamingID = nil
		controller.rename(tab.playlist, to: name)
	}

	func newPlaylist() {
		controller.newPlaylist(nil)
	}

	func duplicate(_ tab: Tab) {
		controller.makePlaylist(named: tab.name, copying: tab.playlist.orderedEntries)
	}

	func delete(_ tab: Tab, in window: NSWindow?) {
		controller.confirmDelete(tab.playlist, in: window)
	}

	/// The tab at `x` in the tabs' space, if any.
	func tab(atX x: CGFloat) -> Tab? {
		tabs.first { tab in
			guard let frame = tabFrames[tab.id] else { return false }
			return x >= frame.minX && x < frame.maxX
		}
	}
}

// MARK: - SwiftUI

private struct TabFramesKey: PreferenceKey {
	static let defaultValue: [UUID: CGRect] = [:]
	static func reduce(value: inout [UUID: CGRect], nextValue: () -> [UUID: CGRect]) {
		value.merge(nextValue()) { $1 }
	}
}

private struct NewPlaylistFrameKey: PreferenceKey {
	static let defaultValue: CGRect = .zero
	static func reduce(value: inout CGRect, nextValue: () -> CGRect) {
		value = nextValue()
	}
}

struct PlaylistTabsView: View {
	@ObservedObject var model: PlaylistTabsModel

	var body: some View {
		HStack(spacing: 0) {
			ScrollViewReader { proxy in
				ScrollView(.horizontal, showsIndicators: false) {
					HStack(spacing: 2) {
						ForEach(model.tabs) { tab in
							PlaylistTabView(tab: tab, model: model)
								.id(tab.id)
								.background(GeometryReader { geometry in
									Color.clear.preference(key: TabFramesKey.self, value: [tab.id: geometry.frame(in: .named("playlistTabs"))])
								})
						}
					}
					.padding(.horizontal, 8)
				}
				.onChange(of: model.shownID) { id in
					guard let id else { return }
					withAnimation(.easeOut(duration: 0.2)) { proxy.scrollTo(id) }
				}
			}
			Button {
				model.newPlaylist()
			} label: {
				Image(systemName: "plus")
					.frame(width: 22, height: 22)
					.background(RoundedRectangle(cornerRadius: 6)
						.fill(model.dropsOnNewPlaylist ? Color.accentColor.opacity(0.25) : Color.clear))
					.contentShape(Rectangle())
			}
			.buttonStyle(.borderless)
			.help(Text("New Playlist", comment: "Playlist tabs: the button that adds a playlist"))
			.accessibilityLabel(Text("New Playlist", comment: "Playlist tabs: the button that adds a playlist"))
			.background(GeometryReader { geometry in
				Color.clear.preference(key: NewPlaylistFrameKey.self, value: geometry.frame(in: .named("playlistTabs")))
			})
			.padding(.trailing, 8)
		}
		.frame(maxWidth: .infinity, maxHeight: .infinity)
		.coordinateSpace(name: "playlistTabs")
		.onPreferenceChange(TabFramesKey.self) { model.tabFrames = $0 }
		.onPreferenceChange(NewPlaylistFrameKey.self) { model.newPlaylistFrame = $0 }
	}
}

private struct PlaylistTabView: View {
	let tab: PlaylistTabsModel.Tab
	@ObservedObject var model: PlaylistTabsModel
	@State private var name = ""
	@FocusState private var nameFocused: Bool

	private var isShown: Bool { model.shownID == tab.id }
	private var isPlaying: Bool { model.playingID == tab.id }
	private var isRenaming: Bool { model.renamingID == tab.id }
	private var isDropTarget: Bool { model.dropTargetID == tab.id }

	var body: some View {
		HStack(spacing: 4) {
			if isPlaying {
				Image(systemName: "speaker.wave.2.fill")
					.imageScale(.small)
					.foregroundColor(.accentColor)
					.accessibilityLabel(Text("Playing", comment: "Playlist tabs: marks the playlist playback is in"))
			}
			if isRenaming {
				TextField(String(""), text: $name)
					.textFieldStyle(.plain)
					.focused($nameFocused)
					.frame(minWidth: 80, maxWidth: 220)
					.fixedSize()
					.onSubmit { model.rename(tab, to: name) }
					.onExitCommand { model.renamingID = nil }
					.onChange(of: nameFocused) { focused in
						if !focused && model.renamingID == tab.id { model.rename(tab, to: name) }
					}
					.onAppear {
						name = tab.name
						DispatchQueue.main.async { nameFocused = true }
					}
			} else {
				Text(tab.name)
					.lineLimit(1)
			}
		}
		.font(.system(size: 12, weight: isShown ? .semibold : .regular))
		.foregroundColor(isShown ? .primary : .secondary)
		.padding(.horizontal, 10)
		.padding(.vertical, 4)
		.background(RoundedRectangle(cornerRadius: 6).fill(background))
		.overlay(RoundedRectangle(cornerRadius: 6)
			.strokeBorder(Color.accentColor, lineWidth: isDropTarget ? 1.5 : 0))
		.contentShape(Rectangle())
		.onTapGesture(count: 2) { model.renamingID = tab.id }
		.simultaneousGesture(TapGesture().onEnded { model.show(tab) })
		.onDrag { NSItemProvider(object: (PlaylistTabsModel.dragPrefix + tab.id.uuidString) as NSString) }
		.contextMenu {
			Button(String(localized: "Rename…", comment: "Playlist tab menu: edit the playlist's name")) { model.renamingID = tab.id }
			Button(String(localized: "Duplicate", comment: "Playlist tab menu: a copy of the playlist")) { model.duplicate(tab) }
			Divider()
			Button(String(localized: "Delete…", comment: "Playlist tab menu: delete the playlist")) {
				model.delete(tab, in: NSApp.mainWindow)
			}
			.disabled(model.tabs.count < 2)
		}
		.help(tab.name)
		.accessibilityElement(children: .combine)
		.accessibilityAddTraits(isShown ? [.isButton, .isSelected] : .isButton)
	}

	private var background: Color {
		if isDropTarget { return Color.accentColor.opacity(0.2) }
		return isShown ? Color.primary.opacity(0.1) : Color.clear
	}
}

// MARK: - The bar

/// Holds the tabs at the top of the playlist, below the toolbar the main
/// window's content runs under, and takes what is dropped on them.
@objc final class PlaylistTabBar: NSView {
	private let model: PlaylistTabsModel
	private let hostingView: NSHostingView<PlaylistTabsView>
	private var heightConstraint: NSLayoutConstraint!
	private var hiddenObservation: NSKeyValueObservation?

	private static let tabHeight: CGFloat = 30
	private static let rowsType = NSPasteboard.PasteboardType("org.cogx.cog.dnd-index")
	private static let urlsType = NSPasteboard.PasteboardType("org.cogx.cog.url")
	private static let iTunesType = NSPasteboard.PasteboardType("com.apple.tv.metadata")

	/// Puts the tabs at the top of `stack`, the playlist's column.
	@objc(installInStackView:controller:)
	static func install(in stack: NSStackView, controller: PlaylistController) {
		let bar = PlaylistTabBar(controller: controller)
		stack.insertArrangedSubview(bar, at: 0)
		bar.widthAnchor.constraint(equalTo: stack.widthAnchor).isActive = true
		PlaylistMenus.install(controller: controller)
	}

	init(controller: PlaylistController) {
		model = PlaylistTabsModel(controller: controller)
		hostingView = NSHostingView(rootView: PlaylistTabsView(model: model))
		super.init(frame: .zero)
		translatesAutoresizingMaskIntoConstraints = false
		if #available(macOS 13.0, *) {
			hostingView.sizingOptions = []
		}
		hostingView.translatesAutoresizingMaskIntoConstraints = false
		addSubview(hostingView)
		heightConstraint = heightAnchor.constraint(equalToConstant: Self.tabHeight)
		NSLayoutConstraint.activate([
			heightConstraint,
			hostingView.leadingAnchor.constraint(equalTo: leadingAnchor),
			hostingView.trailingAnchor.constraint(equalTo: trailingAnchor),
			hostingView.bottomAnchor.constraint(equalTo: bottomAnchor),
			hostingView.heightAnchor.constraint(equalToConstant: Self.tabHeight)
		])
		registerForDraggedTypes([Self.rowsType, Self.urlsType, .fileURL, Self.iTunesType, .string])

		UserDefaults.standard.register(defaults: ["showPlaylistTabs": true])
		hiddenObservation = UserDefaults.standard.observe(\.showPlaylistTabs, options: [.initial, .new]) { [weak self] defaults, _ in
			let shows = defaults.showPlaylistTabs
			Task { @MainActor in self?.isHidden = !shows }
		}
	}

	required init?(coder: NSCoder) {
		fatalError("PlaylistTabBar is made in code")
	}

	override func layout() {
		// The window's content runs under the toolbar; the tabs keep below
		// it, as docked panes' captions do.
		if let window {
			let layoutRect = convert(window.contentLayoutRect, from: nil)
			let inset = max(0, bounds.maxY - layoutRect.maxY)
			if heightConstraint.constant != Self.tabHeight + inset {
				heightConstraint.constant = Self.tabHeight + inset
			}
		}
		super.layout()
	}

	// MARK: Dropping

	private enum Target: Equatable {
		case tab(PlaylistTabsModel.Tab)
		case newPlaylist
	}

	private func target(for info: NSDraggingInfo) -> Target? {
		let point = hostingView.convert(info.draggingLocation, from: nil)
		if model.newPlaylistFrame.insetBy(dx: -4, dy: 0).contains(CGPoint(x: point.x, y: model.newPlaylistFrame.midY)) {
			return .newPlaylist
		}
		return model.tab(atX: point.x).map(Target.tab)
	}

	private func draggedTabID(_ info: NSDraggingInfo) -> UUID? {
		guard let text = info.draggingPasteboard.string(forType: .string), text.hasPrefix(PlaylistTabsModel.dragPrefix) else { return nil }
		return UUID(uuidString: String(text.dropFirst(PlaylistTabsModel.dragPrefix.count)))
	}

	private func isFromTable(_ info: NSDraggingInfo) -> Bool {
		guard let source = info.draggingSource as? NSTableView else { return false }
		return source === model.controller.tableView
	}

	private func operation(for info: NSDraggingInfo) -> NSDragOperation {
		let target = target(for: info)
		if let dragged = draggedTabID(info) {
			guard case let .tab(tab) = target, tab.id != dragged else { return [] }
			return .move
		}
		guard let target else { return [] }
		if isFromTable(info) {
			if case let .tab(tab) = target, tab.id == model.shownID { return [] }
			return NSEvent.modifierFlags.contains(.option) ? .move : .copy
		}
		let types: [NSPasteboard.PasteboardType] = [Self.urlsType, .fileURL, Self.iTunesType]
		return info.draggingPasteboard.availableType(from: types) != nil ? .copy : []
	}

	private func highlight(for info: NSDraggingInfo?) {
		var tabID: UUID?
		var onNew = false
		if let info, !operation(for: info).isEmpty {
			switch target(for: info) {
			case let .tab(tab)?: tabID = tab.id
			case .newPlaylist?: onNew = true
			case nil: break
			}
		}
		if model.dropTargetID != tabID { model.dropTargetID = tabID }
		if model.dropsOnNewPlaylist != onNew { model.dropsOnNewPlaylist = onNew }
	}

	override func draggingEntered(_ sender: NSDraggingInfo) -> NSDragOperation {
		highlight(for: sender)
		return operation(for: sender)
	}

	override func draggingUpdated(_ sender: NSDraggingInfo) -> NSDragOperation {
		highlight(for: sender)
		return operation(for: sender)
	}

	override func draggingExited(_ sender: NSDraggingInfo?) {
		highlight(for: nil)
	}

	override func draggingEnded(_ sender: NSDraggingInfo) {
		highlight(for: nil)
	}

	override func performDragOperation(_ sender: NSDraggingInfo) -> Bool {
		defer { highlight(for: nil) }
		let operation = operation(for: sender)
		guard !operation.isEmpty, let target = target(for: sender) else { return false }
		let controller = model.controller

		if let dragged = draggedTabID(sender) {
			guard case let .tab(tab) = target,
			      let playlist = model.tabs.first(where: { $0.id == dragged })?.playlist,
			      let position = model.tabs.firstIndex(of: tab) else { return false }
			controller.move(playlist, to: position)
			return true
		}

		if isFromTable(sender) {
			let arranged = controller.arrangedObjects as? [PlaylistEntry] ?? []
			let rows = (sender.draggingPasteboard.pasteboardItems ?? []).compactMap { $0.string(forType: Self.rowsType).flatMap(Int.init) }
			let entries = rows.filter { $0 >= 0 && $0 < arranged.count }.sorted().map { arranged[$0] }
			guard !entries.isEmpty else { return false }
			let moving = operation == .move
			switch target {
			case let .tab(tab):
				controller.add(entries, to: tab.playlist, moving: moving)
			case .newPlaylist:
				let source = controller.shownPlaylist
				let playlist = controller.makePlaylist(copying: entries)
				if moving, let source {
					// Out of the playlist they came from, which is shown again
					// for it.
					controller.show(source)
					let removed = IndexSet(entries.map { controller.row(for: $0) }.filter { $0 != NSNotFound })
					controller.remove(atArrangedObjectIndexes: removed)
					controller.show(playlist)
				}
				NotificationCenter.default.post(name: .cogRenamePlaylist, object: playlist)
			}
			return true
		}

		let urls = Self.urls(from: sender.draggingPasteboard)
		guard !urls.isEmpty else { return false }
		switch target {
		case let .tab(tab):
			controller.add(urls, to: tab.playlist)
		case .newPlaylist:
			let name = urls.count == 1 ? urls[0].deletingPathExtension().lastPathComponent : nil
			controller.add(urls, to: controller.makePlaylist(named: name))
		}
		return true
	}

	/// What a drop brings to add, as the table takes it: Cog's own URLs,
	/// files, or Music tracks.
	private static func urls(from pasteboard: NSPasteboard) -> [URL] {
		switch pasteboard.availableType(from: [urlsType, .fileURL, iTunesType]) {
		case urlsType?:
			guard let data = pasteboard.data(forType: urlsType) else { return [] }
			return (try? NSKeyedUnarchiver.unarchivedArrayOfObjects(ofClass: NSURL.self, from: data))?.map { $0 as URL } ?? []
		case .fileURL?:
			return pasteboard.readObjects(forClasses: [NSURL.self]) as? [URL] ?? []
		case iTunesType?:
			let tracks = (pasteboard.propertyList(forType: iTunesType) as? [String: Any])?["Tracks"] as? [String: [String: Any]] ?? [:]
			return tracks.values.compactMap { ($0["Location"] as? String).flatMap(URL.init(string:)) }
		default:
			return []
		}
	}
}

extension UserDefaults {
	@objc dynamic var showPlaylistTabs: Bool { bool(forKey: "showPlaylistTabs") }
}

// MARK: - Menus

/// The playlist commands' items, added to the Playlist and View menus and
/// the table's menu.
@MainActor
enum PlaylistMenus {
	private static var addToPlaylistDelegate: AddToPlaylistMenuDelegate?

	static func install(controller: PlaylistController) {
		guard let mainMenu = NSApp.mainMenu else { return }
		func menu(containing action: Selector) -> NSMenu? {
			mainMenu.items.compactMap(\.submenu).first { $0.items.contains { $0.action == action } }
		}

		if let playlistMenu = menu(containing: #selector(PlaylistController.emptyQueueList(_:))) {
			let items: [NSMenuItem] = [
				item(NSLocalizedString("New Playlist", comment: "Playlist menu: adds an empty playlist"), #selector(PlaylistController.newPlaylist(_:)), "n", [.command], controller),
				item(NSLocalizedString("New Playlist from Selection", comment: "Playlist menu: a new playlist with copies of the selected tracks"), #selector(PlaylistController.newPlaylistFromSelection(_:)), "n", [.command, .shift], controller),
				item(NSLocalizedString("Rename Playlist…", comment: "Playlist menu: edit the shown playlist's name"), #selector(PlaylistController.renamePlaylist(_:)), "", [], controller),
				item(NSLocalizedString("Delete Playlist…", comment: "Playlist menu: delete the shown playlist"), #selector(PlaylistController.deletePlaylist(_:)), "", [], controller),
				.separator(),
				item(NSLocalizedString("Show Next Playlist", comment: "Playlist menu: the tab to the right"), #selector(PlaylistController.showNextPlaylist(_:)), "\t", [.control], controller),
				item(NSLocalizedString("Show Previous Playlist", comment: "Playlist menu: the tab to the left"), #selector(PlaylistController.showPreviousPlaylist(_:)), "\t", [.control, .shift], controller),
				.separator()
			]
			for (offset, item) in items.enumerated() {
				playlistMenu.insertItem(item, at: offset)
			}
		}

		if let viewMenu = menu(containing: Selector(("toggleSideView:"))),
		   let fileTree = viewMenu.items.firstIndex(where: { $0.action == Selector(("toggleSideView:")) }) {
			let item = NSMenuItem(title: NSLocalizedString("Show Playlist Tabs", comment: "View menu: the tabs above the playlist"), action: nil, keyEquivalent: "")
			item.bind(.value, to: NSUserDefaultsController.shared, withKeyPath: "values.showPlaylistTabs", options: nil)
			viewMenu.insertItem(item, at: fileTree + 1)
		}

		if let tableMenu = controller.tableView?.menu {
			let item = NSMenuItem(title: NSLocalizedString("Add to Playlist", comment: "Playlist row menu: copies the selected tracks to another playlist"), action: nil, keyEquivalent: "")
			let submenu = NSMenu(title: item.title)
			let delegate = AddToPlaylistMenuDelegate(controller: controller)
			addToPlaylistDelegate = delegate
			submenu.delegate = delegate
			item.submenu = submenu
			// Beside Add to Queue, at the top.
			tableMenu.insertItem(item, at: min(1, tableMenu.items.count))
		}
	}

	private static func item(_ title: String, _ action: Selector, _ key: String, _ modifiers: NSEvent.ModifierFlags, _ target: AnyObject) -> NSMenuItem {
		let item = NSMenuItem(title: title, action: action, keyEquivalent: key)
		item.keyEquivalentModifierMask = modifiers
		item.target = target
		return item
	}
}

private final class AddToPlaylistMenuDelegate: NSObject, NSMenuDelegate {
	let controller: PlaylistController

	init(controller: PlaylistController) {
		self.controller = controller
	}

	func menuNeedsUpdate(_ menu: NSMenu) {
		controller.fillAddToPlaylistMenu(menu)
	}
}
