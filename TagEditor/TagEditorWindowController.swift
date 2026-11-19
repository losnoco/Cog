//
//  TagEditorWindowController.swift
//  Cog
//
//  A tag editor window for the files behind a playlist selection, and the
//  Edit Tags… action that opens one. Each editor is its own window, so an
//  edit in progress is never replaced by the next one opened.
//

import Cocoa
import CogAudio
import SwiftUI
import UniformTypeIdentifiers
// Only these: CogPlaylist's own PlaylistLoader would hide the app's.
import class CogPlaylist.PlaylistEntry

/// The user default for a save that changed a file's chapters: "ask",
/// "rebuild" its playlist rows to match, or "keep" them.
let tagEditorChapterRowsDefault = "tagEditorChapterRows"

@MainActor final class TagEditorWindowController: NSWindowController, NSWindowDelegate {
	private static var openEditors: Set<TagEditorWindowController> = []

	private let model: TagEditorModel
	private let saver: TagSaver

	/// What a file read in the background hands the main queue.
	private struct ReadFile: @unchecked Sendable {
		let url: URL
		let tags: [String: Any]
		let capabilities: [String: Any]
	}

	// MARK: - Opening

	/// Opens an editor for the files behind `entries`, once they're read.
	/// Files whose tags can't be saved are left out, and said so.
	static func edit(_ entries: [PlaylistEntry], saver: TagSaver) {
		let files = saver.files(of: entries)
		guard !files.isEmpty else {
			NSSound.beep()
			return
		}
		var chapters: [URL: Int] = [:]
		var showsArt: [URL: Bool] = [:]
		for entry in entries {
			guard let file = TagSaver.tagFileURL(for: entry) else { continue }
			if chapters[file] == nil, let chapter = TagSaver.chapterIndex(for: entry) { chapters[file] = chapter }
			showsArt[file] = (showsArt[file] ?? false) || entry.artHash != nil
		}
		let chapterOfFile = chapters, artOfFile = showsArt

		DispatchQueue.global(qos: .userInitiated).async {
			let plugins: CogPluginController = PluginController.shared()
			let read = files.compactMap { url -> ReadFile? in
				let capabilities = (plugins.tagWritingCapabilities(for: url) as? [String: Any]) ?? [:]
				guard capabilities[CogTagWriterWritable] as? Bool == true else { return nil }
				return ReadFile(url: url, tags: (plugins.writableTags(for: url) as? [String: Any]) ?? [:], capabilities: capabilities)
			}
			DispatchQueue.main.async {
				MainActor.assumeIsolated {
					guard !read.isEmpty else {
						let alert = NSAlert()
						alert.messageText = NSLocalizedString("Tags can't be saved to these files.", comment: "Tag editor: no selected file is writable")
						alert.informativeText = NSLocalizedString("Cog saves tags to local files of the kinds TagLib can write, but not to a cue sheet's tracks.", comment: "Tag editor: which files can be edited")
						alert.runModal()
						return
					}
					let sources = read.map {
						TagEditorModel.Source(file: $0.url, tags: $0.tags, capabilities: $0.capabilities,
						                      showsArt: artOfFile[$0.url] ?? false, chapter: read.count == 1 ? chapterOfFile[$0.url] : nil)
					}
					let controller = TagEditorWindowController(model: TagEditorModel(sources: sources, leftOut: files.count - read.count), saver: saver)
					openEditors.insert(controller)
					controller.showWindow(nil)
				}
			}
		}
	}

	/// The window is made here and handed to NSWindowController, rather than
	/// in loadWindow(), which it calls only in some circumstances.
	private init(model: TagEditorModel, saver: TagSaver) {
		self.model = model
		self.saver = saver
		let window = NSWindow(contentRect: NSRect(x: 0, y: 0, width: 560, height: 680),
		                      styleMask: [.titled, .closable, .resizable, .miniaturizable],
		                      backing: .buffered, defer: false)
		super.init(window: window)
		configure(window)
	}

	@available(*, unavailable)
	required init?(coder: NSCoder) { fatalError() }

	private func configure(_ window: NSWindow) {
		// Inside the screen it opens on, title bar included.
		let visible = NSScreen.main?.visibleFrame.height ?? 800
		let height = min(680, max(320, visible - 60))
		let view = TagEditorView(model: model,
		                         save: { [weak self] in self?.save() },
		                         cancel: { [weak self] in self?.close() },
		                         chooseCover: { [weak self] in self?.chooseCover() },
		                         idealHeight: height)
		window.contentViewController = NSHostingController(rootView: view)
		window.setContentSize(NSSize(width: 560, height: height))
		window.title = model.isBatch
			? String(format: NSLocalizedString("Edit Tags of %ld Files", comment: "Tag editor window title for several files"), model.sources.count)
			: String(format: NSLocalizedString("Edit Tags of %@", comment: "Tag editor window title for one file"), model.sources[0].file.lastPathComponent)
		window.isReleasedWhenClosed = false
		window.delegate = self
		window.center()
	}

	func windowWillClose(_ notification: Notification) {
		Self.openEditors.remove(self)
	}

	// MARK: - Cover

	private func chooseCover() {
		guard let window else { return }
		let panel = NSOpenPanel()
		panel.allowedContentTypes = [.image]
		panel.allowsMultipleSelection = false
		panel.message = NSLocalizedString("Choose a picture for the cover.", comment: "Tag editor: cover picker prompt")
		panel.beginSheetModal(for: window) { [weak self] response in
			guard response == .OK, let url = panel.url else { return }
			if let data = try? Data(contentsOf: url), NSImage(data: data) != nil {
				self?.model.art = .replace(data)
			} else {
				NSSound.beep()
			}
		}
	}

	// MARK: - Saving

	private func save() {
		let files = model.fileTags()
		guard !files.isEmpty else {
			close()
			return
		}
		model.isSaving = true
		saver.save(files) { [weak self] outcome in
			guard let self else { return }
			self.model.isSaving = false
			if outcome.failures.isEmpty {
				self.close()
			} else {
				self.reportFailures(outcome.failures, savedCount: outcome.saved.count)
			}
			self.followChapters(of: outcome.rowsChanged)
		}
	}

	private func reportFailures(_ failures: [(url: URL, error: Error)], savedCount: Int) {
		guard let window else { return }
		let alert = NSAlert()
		alert.alertStyle = .warning
		alert.messageText = failures.count == 1
			? String(format: NSLocalizedString("The tags of “%@” couldn't be saved.", comment: "Tag editor: one save failed"), failures[0].url.lastPathComponent)
			: String(format: NSLocalizedString("The tags of %ld files couldn't be saved.", comment: "Tag editor: several saves failed"), failures.count)
		alert.informativeText = failures.prefix(8).map { failure in
			let reason = (failure.error as NSError).localizedFailureReason ?? failure.error.localizedDescription
			return "\(failure.url.lastPathComponent): \(reason)"
		}.joined(separator: "\n")
		alert.beginSheetModal(for: window)
	}

	/// After a save changed some files' chapters, their rows follow the
	/// setting, asking when it says to.
	private func followChapters(of files: [URL]) {
		guard !files.isEmpty else { return }
		let defaults = UserDefaults.standard
		var choice = defaults.string(forKey: tagEditorChapterRowsDefault) ?? "ask"
		if choice == "ask" {
			let alert = NSAlert()
			alert.messageText = files.count == 1
				? String(format: NSLocalizedString("The chapters of “%@” changed.", comment: "Tag editor: one file's chapters changed"), files[0].lastPathComponent)
				: String(format: NSLocalizedString("The chapters of %ld files changed.", comment: "Tag editor: several files' chapters changed"), files.count)
			alert.informativeText = NSLocalizedString("Rebuild their rows in the playlists to match? Rows for the old chapters are replaced with rows for the new ones.", comment: "Tag editor: rebuild chapter rows question")
			alert.addButton(withTitle: NSLocalizedString("Rebuild Rows", comment: "Tag editor: rebuild chapter rows"))
			alert.addButton(withTitle: NSLocalizedString("Keep Rows", comment: "Tag editor: keep chapter rows"))
			alert.showsSuppressionButton = true
			alert.suppressionButton?.title = NSLocalizedString("Do this every time", comment: "Tag editor: remember the chapter rows choice")
			choice = alert.runModal() == .alertFirstButtonReturn ? "rebuild" : "keep"
			if alert.suppressionButton?.state == .on {
				defaults.set(choice, forKey: tagEditorChapterRowsDefault)
			}
		}
		if choice == "rebuild" {
			for file in files { saver.rebuildRows(of: file) }
		}
	}
}

// MARK: - The action

@MainActor extension PlaylistController {
	private static var tagSaver: TagSaver?

	private var tagSaver: TagSaver? {
		if let saver = Self.tagSaver { return saver }
		guard let app = NSApp.delegate as? AppController else { return nil }
		let saver = TagSaver(playlistController: self, playlistLoader: app.playlistLoader)
		Self.tagSaver = saver
		return saver
	}

	/// Edit Tags…, in the Playlist menu and the playlist's contextual menu.
	@IBAction func editTags(_ sender: Any?) {
		guard let saver = tagSaver, let entries = selectedObjects as? [PlaylistEntry] else { return }
		TagEditorWindowController.edit(entries, saver: saver)
	}

	/// Whether Edit Tags… applies to the selection.
	@objc var canEditTagsOfSelection: Bool {
		guard let saver = tagSaver, let entries = selectedObjects as? [PlaylistEntry] else { return false }
		return saver.canEditTags(of: entries)
	}
}
