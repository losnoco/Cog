//
//  SpotlightPanelView.swift
//  Cog
//
//  The Spotlight panel in SwiftUI: the search, the folder it looks in and
//  the button that adds what it found. The results stay an AppKit table
//  (SpotlightPanel.xib), with its columns, drag and drop and menu, hosted
//  here as it is. SpotlightWindowController runs the query.
//

import AppKit
import SwiftUI

@objc(SpotlightPanel)
@MainActor
final class SpotlightPanel: NSObject {
	/// The panel's window, around `controller`'s results table.
	@objc static func makeWindow(controller: SpotlightWindowController) -> NSWindow {
		let panel = NSPanel(contentRect: NSRect(x: 196, y: 192, width: 480, height: 320),
		                    styleMask: [.titled, .closable, .miniaturizable, .resizable, .utilityWindow],
		                    backing: .buffered, defer: true)
		panel.title = String(localized: "Spotlight", comment: "The Spotlight panel's title")
		panel.hidesOnDeactivate = true
		panel.isReleasedWhenClosed = false
		panel.contentMinSize = NSSize(width: 400, height: 260)

		let results = controller.resultsView ?? NSScrollView()
		let model = SpotlightPanelModel(controller: controller)
		panel.contentView = NSHostingView(rootView: SpotlightPanelView(model: model, results: results))
		panel.setFrameAutosaveName("Spotlight")
		return panel
	}
}

/// What the panel shows of the controller, kept in step with it: a search
/// for an artist or album from the playlist sets the text too.
@MainActor
final class SpotlightPanelModel: ObservableObject {
	private let controller: SpotlightWindowController
	private var observations: [NSKeyValueObservation] = []
	// Set up once in init, read again only in deinit.
	nonisolated(unsafe) private var tokens: [NSObjectProtocol] = []

	@Published var searchText: String {
		didSet {
			if controller.searchString != searchText { controller.searchString = searchText }
		}
	}

	@Published private(set) var folder: URL
	@Published private(set) var isGathering = false
	@Published private(set) var hasResults = false

	init(controller: SpotlightWindowController) {
		self.controller = controller
		searchText = controller.searchString ?? ""
		folder = controller.searchFolder

		observations.append(controller.observe(\.searchString, options: [.new]) { [weak self] controller, _ in
			MainActor.assumeIsolated {
				guard let self else { return }
				let text = controller.searchString ?? ""
				if self.searchText != text { self.searchText = text }
			}
		})

		let query = controller.query
		let center = NotificationCenter.default
		for name in [Notification.Name.NSMetadataQueryDidStartGathering, .NSMetadataQueryGatheringProgress,
		             .NSMetadataQueryDidFinishGathering, .NSMetadataQueryDidUpdate] {
			tokens.append(center.addObserver(forName: name, object: query, queue: .main) { [weak self] _ in
				MainActor.assumeIsolated { self?.queryChanged() }
			})
		}
	}

	deinit {
		for token in tokens {
			NotificationCenter.default.removeObserver(token)
		}
	}

	private func queryChanged() {
		isGathering = controller.query.isGathering
		hasResults = controller.query.resultCount > 0
	}

	func setFolder(_ url: URL) {
		folder = url
		controller.searchFolder = url
	}

	func addToPlaylist() {
		controller.addToPlaylist(nil)
	}
}

struct SpotlightPanelView: View {
	@ObservedObject var model: SpotlightPanelModel
	let results: NSView
	@FocusState private var searchFocused: Bool

	var body: some View {
		VStack(spacing: 10) {
			HStack(spacing: 8) {
				searchField
				Button("Add to Playlist") { model.addToPlaylist() }
					.keyboardShortcut(.defaultAction)
					.disabled(!model.hasResults)
			}

			ResultsTable(view: results)
				.frame(minHeight: 120)

			HStack(spacing: 6) {
				if model.isGathering {
					ProgressView()
						.controlSize(.small)
				}
				Spacer()
				Text("Search Location:")
				FolderControl(url: model.folder) { model.setFolder($0) }
					.fixedSize()
			}
		}
		.padding(EdgeInsets(top: 12, leading: 16, bottom: 12, trailing: 16))
		.frame(maxWidth: .infinity, maxHeight: .infinity)
		.onAppear { searchFocused = true }
	}

	private var searchField: some View {
		HStack(spacing: 4) {
			Image(systemName: "magnifyingglass")
				.foregroundStyle(.secondary)
			TextField("Search", text: $model.searchText)
				.textFieldStyle(.plain)
				.focused($searchFocused)
			if !model.searchText.isEmpty {
				Button {
					model.searchText = ""
				} label: {
					Image(systemName: "xmark.circle.fill")
						.foregroundStyle(.secondary)
				}
				.buttonStyle(.plain)
				.help(Text("Clear"))
			}
		}
		.padding(.horizontal, 6)
		.padding(.vertical, 4)
		.background(RoundedRectangle(cornerRadius: 6).fill(Color(nsColor: .textBackgroundColor)))
		.overlay(RoundedRectangle(cornerRadius: 6).strokeBorder(Color(nsColor: .separatorColor)))
		.help(Text(verbatim: Self.searchHelp))
	}

	/// The search's prefixes, passed in so they aren't taken for formats.
	private static var searchHelp: String {
		let codes = "%a %l %t %g %c"
		let example = "%a\"The Beatles\""
		return String(localized: "Words of three letters or more find titles, albums and artists. Before a word, \(codes) look only at artists, albums, titles, genres or comments, in that order; quote it for a phrase, as in \(example).",
		              comment: "The Spotlight panel's search field tooltip. The first argument is the prefixes (%a %l %t %g %c), the second an example search.")
	}
}

/// The AppKit results table, as SpotlightPanel.xib has it.
private struct ResultsTable: NSViewRepresentable {
	let view: NSView

	func makeNSView(context: Context) -> NSView { view }

	func updateNSView(_ nsView: NSView, context: Context) {}
}

/// The folder the search looks in, as a pop-up path control: the folder and
/// those it is in, and Choose… for another.
private struct FolderControl: NSViewRepresentable {
	let url: URL
	let onChange: (URL) -> Void

	func makeCoordinator() -> Coordinator { Coordinator(onChange: onChange) }

	func makeNSView(context: Context) -> NSPathControl {
		let control = NSPathControl()
		control.pathStyle = .popUp
		control.isEditable = true
		control.focusRingType = .none
		control.url = url
		control.target = context.coordinator
		control.action = #selector(Coordinator.changed(_:))
		return control
	}

	func updateNSView(_ control: NSPathControl, context: Context) {
		context.coordinator.onChange = onChange
		if control.url?.standardizedFileURL != url.standardizedFileURL { control.url = url }
	}

	@MainActor final class Coordinator: NSObject {
		var onChange: (URL) -> Void

		init(onChange: @escaping (URL) -> Void) {
			self.onChange = onChange
		}

		/// A folder above, from the menu, or one from Choose….
		@objc func changed(_ control: NSPathControl) {
			// Picking a folder above sets nothing by itself.
			if let item = control.clickedPathItem, let url = item.url { control.url = url }
			if let url = control.url { onChange(url) }
		}
	}
}
