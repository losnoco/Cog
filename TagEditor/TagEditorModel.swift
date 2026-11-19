//
//  TagEditorModel.swift
//  Cog
//
//  The tag editor's state, with no views: the fields it lists, what the
//  files behind it can hold, and what Save hands TagSaver. A Swift port of
//  XPCog's TagEditorModel (uicore/src/TagEditorModel.hpp), with chapters.
//
//  Capability-driven, not format-driven: nothing here knows "ID3" or
//  "Vorbis", only what the writer's probe answered for the files. Notes
//  that report folding annotate rows; they never block a save.
//
//  Two faces:
//
//  * One file: the whole set of tags in, the whole set out, replacing the
//    file's. Untouched fields go back as they were, repeated values and
//    all; a blank field is left out, so the save removes it.
//  * Several files: the named fields only, and only those that were
//    touched, merged into each file. Free-form rows, the cover and the
//    chapters stay single-file: several files' differing extra tags don't
//    fit one table.
//

import AppKit
import CogAudio
import SwiftUI

@MainActor final class TagEditorModel: ObservableObject {
	/// One file behind the editor, as TagSaver found and read it.
	struct Source {
		let file: URL
		/// The file's own tags, cover and chapters, as the writer reads them:
		/// no folder art, nothing another reader adds.
		let tags: [String: Any]
		let capabilities: [String: Any]
		/// The playlist shows a cover for it, its own or its folder's.
		let showsArt: Bool
		/// The chapter the selected row plays, when it is one.
		let chapter: Int?
	}

	/// The fields every editor lists, in order.
	enum Field: CaseIterable, Identifiable {
		case title, artist, albumArtist, album, genre, composer, date, track, disc, comment, lyrics

		var id: Self { self }

		var label: LocalizedStringKey {
			switch self {
				case .title: return "Title"
				case .artist: return "Artist"
				case .albumArtist: return "Album Artist"
				case .album: return "Album"
				case .genre: return "Genre"
				case .composer: return "Composer"
				case .date: return "Date"
				case .track: return "Track"
				case .disc: return "Disc"
				case .comment: return "Comment"
				case .lyrics: return "Lyrics"
			}
		}

		/// The names the field is read under, as the playlist reads them; a
		/// save writes the first and retires the rest.
		var names: [String] {
			switch self {
				case .title: return ["title"]
				case .artist: return ["artist"]
				case .albumArtist: return ["albumartist", "album artist", "album_artist"]
				case .album: return ["album"]
				case .genre: return ["genre"]
				case .composer: return ["composer"]
				case .date: return ["date", "recording_date", "year"]
				case .track: return ["track"]
				case .disc: return ["disc"]
				// DESCRIPTION is the Vorbis comment spec's field for it, and
				// what ffmpeg writes a comment as.
				case .comment: return ["comment", "description"]
				case .lyrics: return ["lyrics", "unsyncedlyrics", "unsynced lyrics"]
			}
		}

		var isLong: Bool { self == .comment || self == .lyrics }
	}

	struct FieldState {
		var text = ""
		var original = ""
		/// Several files disagree, so the field starts blank.
		var differs = false
		/// The file holds more than one value; retyping makes it one.
		var valueCount = 0
		var touched = false

		/// Whether a save writes it.
		var isChanged: Bool { touched && (text != original || differs) }
	}

	struct FreeRow: Identifiable, Equatable {
		let id = UUID()
		var name: String
		var values: [String]

		static func == (a: FreeRow, b: FreeRow) -> Bool { a.name == b.name && a.values == b.values }
	}

	struct ChapterRow: Identifiable, Equatable {
		let id = UUID()
		var start: Int // milliseconds
		var title: String
		/// Kept from the read, so a chapter keeps its identity.
		var identifier: NSObject?

		static func == (a: ChapterRow, b: ChapterRow) -> Bool { a.start == b.start && a.title == b.title && a.identifier == b.identifier }
	}

	enum ArtAction: Equatable {
		case keep, remove
		case replace(Data)
	}

	let sources: [Source]
	var isBatch: Bool { sources.count > 1 }
	/// Selected files left out because their tags can't be saved.
	let leftOut: Int

	@Published var fields: [Field: FieldState] = [:]
	@Published var freeRows: [FreeRow] = []
	@Published var chapters: [ChapterRow] = []
	@Published var art: ArtAction = .keep
	@Published var isSaving = false

	private var originalFreeRows: [FreeRow] = []
	private var originalChapters: [ChapterRow] = []

	/// The file's own cover, single file only.
	let embeddedArt: Data?

	init(sources: [Source], leftOut: Int = 0) {
		self.sources = sources
		self.leftOut = leftOut
		embeddedArt = sources.count == 1 ? sources[0].tags[CogTagAlbumArt] as? Data : nil

		for field in Field.allCases {
			let values = sources.map { Self.values(of: field, in: $0.tags) }
			var state = FieldState()
			if let first = values.first, values.allSatisfy({ $0 == first }) {
				state.text = first.joined(separator: "; ")
				state.valueCount = first.count
			} else {
				state.differs = true
			}
			state.original = state.text
			fields[field] = state
		}

		if sources.count == 1 {
			let source = sources[0]
			let named = Set(Field.allCases.flatMap(\.names))
			freeRows = source.tags.keys
				.filter { !named.contains($0) && $0 != CogTagAlbumArt && $0 != CogTagChapters }
				.sorted()
				.map { FreeRow(name: $0, values: Self.strings(source.tags[$0])) }
			if supportsChapters, let list = source.tags[CogTagChapters] as? [[String: Any]] {
				chapters = list.map {
					ChapterRow(start: ($0[CogChapterStart] as? NSNumber)?.intValue ?? 0,
					           title: $0[CogChapterTitle] as? String ?? "",
					           identifier: $0[CogChapterID] as? NSObject)
				}
			}
		}
		originalFreeRows = freeRows
		originalChapters = chapters
	}

	// MARK: - What the files can hold

	/// Whether every file can do it: the notes are then true of each.
	private func all(_ capability: String) -> Bool {
		sources.allSatisfy { ($0.capabilities[capability] as? Bool) == true }
	}

	var keepsRepeatedValues: Bool { all(CogTagWriterMultiValue) }
	var supportsFreeRows: Bool { !isBatch && (all(CogTagWriterArbitraryKeys) || all(CogTagWriterFoldsKeys)) }
	/// Other names become a custom field of the format's (TXXX, ----).
	var foldsNames: Bool { all(CogTagWriterFoldsKeys) && !all(CogTagWriterArbitraryKeys) }
	var supportsArt: Bool { !isBatch && all(CogTagWriterPictures) }
	var supportsChapters: Bool { !isBatch && all(CogTagWriterChapters) }

	/// The playlist shows a cover the file doesn't have: its folder's.
	var artFromFolder: Bool { !isBatch && embeddedArt == nil && sources.first?.showsArt == true }

	/// The chapter the selected row plays, to point it out.
	var highlightedChapter: Int? { isBatch ? nil : sources.first?.chapter }

	/// A free row's note, when the save won't keep it as it shows.
	func note(for row: FreeRow) -> LocalizedStringKey? {
		let name = row.name.trimmingCharacters(in: .whitespaces).lowercased()
		if Field.allCases.contains(where: { $0.names.contains(name) }) {
			return "This is one of the fields above, which takes its place when edited."
		}
		if row.values.filter({ !$0.isEmpty }).count > 1 && !keepsRepeatedValues {
			return "This format keeps only the first value."
		}
		return nil
	}

	// MARK: - Editing

	func binding(for field: Field) -> Binding<String> {
		Binding(get: { self.fields[field]?.text ?? "" },
		        set: { text in
		        	self.fields[field]?.text = text
		        	self.fields[field]?.touched = true
		        })
	}

	func addFreeRow() {
		freeRows.append(FreeRow(name: "", values: [""]))
	}

	func addChapter() {
		let start = (chapters.map(\.start).max() ?? -1000) + 1000
		chapters.append(ChapterRow(start: start, title: "", identifier: nil))
	}

	// MARK: - Saving

	var hasChanges: Bool {
		fields.values.contains(where: \.isChanged) || freeRows != originalFreeRows || chapters != originalChapters || art != .keep
	}

	/// What each file's save writes.
	func fileTags() -> [TagSaver.FileTags] {
		isBatch ? batchChanges() : [singleFile()]
	}

	private func singleFile() -> TagSaver.FileTags {
		let source = sources[0]
		var tags: [String: Any] = [:]

		for row in freeRows {
			let name = row.name.trimmingCharacters(in: .whitespaces).lowercased()
			let values = row.values.filter { !$0.isEmpty }
			guard !name.isEmpty, !values.isEmpty else { continue }
			tags[name] = values
		}

		for field in Field.allCases {
			guard let state = fields[field] else { continue }
			if state.isChanged {
				for name in field.names { tags.removeValue(forKey: name) }
				if !state.text.isEmpty { tags[field.names[0]] = [state.text] }
			} else {
				// As the file had them, under the names it had them by.
				for name in field.names where source.tags[name] != nil {
					tags[name] = source.tags[name]
				}
			}
		}

		switch art {
			case .keep: break
			case .remove: tags[CogTagAlbumArt] = Data()
			case let .replace(data): tags[CogTagAlbumArt] = data
		}

		if supportsChapters && chapters != originalChapters {
			tags[CogTagChapters] = chapters.map { chapter -> [String: Any] in
				var item: [String: Any] = [CogChapterStart: chapter.start, CogChapterTitle: chapter.title]
				if let identifier = chapter.identifier { item[CogChapterID] = identifier }
				return item
			}
		}

		return TagSaver.FileTags(url: source.file, tags: tags, replace: true)
	}

	private func batchChanges() -> [TagSaver.FileTags] {
		var changes: [String: Any] = [:]
		for field in Field.allCases {
			guard let state = fields[field], state.isChanged else { continue }
			// Every spelling but the one written is retired, or the next
			// read could take a stale one for the field.
			for name in field.names.dropFirst() { changes[name] = [String]() }
			changes[field.names[0]] = state.text.isEmpty ? [String]() : [state.text]
		}
		guard !changes.isEmpty else { return [] }
		return sources.map { TagSaver.FileTags(url: $0.file, tags: changes, replace: false) }
	}

	// MARK: - Reading

	private static func strings(_ value: Any?) -> [String] {
		if let array = value as? [Any] { return array.map { "\($0)" } }
		if let string = value as? String { return [string] }
		return []
	}

	/// A field's values in a file: those of the first of its names the file
	/// has.
	private static func values(of field: Field, in tags: [String: Any]) -> [String] {
		for name in field.names {
			let values = strings(tags[name])
			if !values.isEmpty { return values }
		}
		return []
	}

	// MARK: - Chapter times

	/// "m:ss.mmm", or "h:mm:ss.mmm" from an hour on.
	static func timeText(_ milliseconds: Int) -> String {
		let ms = max(0, milliseconds)
		let hours = ms / 3_600_000, minutes = ms / 60000 % 60, seconds = ms / 1000 % 60, rest = ms % 1000
		return hours > 0 ? String(format: "%d:%02d:%02d.%03d", hours, minutes, seconds, rest)
			: String(format: "%d:%02d.%03d", minutes, seconds, rest)
	}

	/// Reads "h:mm:ss.mmm", "m:ss", "ss.s" and so on; nil when it isn't a time.
	static func milliseconds(from text: String) -> Int? {
		let parts = text.trimmingCharacters(in: .whitespaces).split(separator: ":", omittingEmptySubsequences: false)
		guard (1...3).contains(parts.count), let last = Double(parts.last!), last >= 0 else { return nil }
		var total = last
		for (power, part) in parts.dropLast().reversed().enumerated() {
			guard let value = Int(part), value >= 0 else { return nil }
			total += Double(value) * (power == 0 ? 60 : 3600)
		}
		return Int((total * 1000).rounded())
	}
}
