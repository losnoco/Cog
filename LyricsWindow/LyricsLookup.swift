//
//  LyricsLookup.swift
//  Cog
//

import CoreData
import Foundation

// On iOS the Core Data classes come from CogPlaylist; macOS generates its own.
#if canImport(CogPlaylist)
import CogPlaylist
#endif

/// Fetches lyrics from LRCLIB for tracks that carry none of their own, and
/// remembers the answers in memory and in the Core Data store.
///
/// How long an answer is trusted:
/// - found or instrumental: forever;
/// - not found: a week, then the server is asked again;
/// - failure: a minute, in memory only, never written to the store.
///
/// Requests go out one at a time, and a question already in flight is not
/// asked twice. Everything here runs on the main actor, which is where the
/// rest of the app uses the persistent container's view context.
@MainActor
@objc(CogLyricsLookup)
final class LyricsLookup: NSObject {
	@objc static let shared = LyricsLookup()

	static let enabledKey = "enableLrclib"
	static let rootKey = "lrclibUrl"

	static let notFoundLifetime: TimeInterval = 7 * 24 * 60 * 60
	static let failureLifetime: TimeInterval = 60
	static let maxInMemory = 512

	private enum Outcome {
		case answer(LrclibAnswer)
		case failure(LrclibError)
	}

	private struct Remembered {
		var outcome: Outcome
		var at: Date
	}

	private var memory: [String: Remembered] = [:]
	private var waiting: [String: [(String) -> Void]] = [:]
	private var tail: Task<Void, Never>?
	private var client = LrclibClient()

	@objc var isEnabled: Bool {
		UserDefaults.standard.bool(forKey: Self.enabledKey)
	}

	// MARK: - Entry point for the lyrics window

	/// The text to show for a track without lyrics of its own.
	///
	/// Returns nil when LRCLIB is off or the track can't be asked about (no
	/// artist or title). Otherwise returns either the final text, or a
	/// placeholder while the server is asked, in which case `completion` is
	/// called later on the main actor with the final text.
	@objc func displayText(title: String?, artist: String?, album: String?, duration: Double,
	                       completion: @escaping (String) -> Void) -> String? {
		guard isEnabled else { return nil }
		let query = LrclibQuery(title: title ?? "", artist: artist ?? "", album: album ?? "", duration: duration)
		guard !query.title.isEmpty, !query.artist.isEmpty else { return nil }

		updateRoot()
		if let remembered = cached(query.key) {
			return Self.text(for: remembered.outcome)
		}
		lookup(query, completion: completion)
		return NSLocalizedString("Asking LRCLIB…", comment: "Lyrics window placeholder while LRCLIB is queried")
	}

	// MARK: - Cache

	private func updateRoot() {
		let root = UserDefaults.standard.string(forKey: Self.rootKey) ?? LrclibClient.defaultRoot
		if root != client.root {
			// Answers from another server don't apply; the store keeps its
			// rows, which are replaced as they are asked again.
			client = LrclibClient(root: root, session: client.session)
			memory.removeAll()
		}
	}

	private func isFresh(_ remembered: Remembered, now: Date = Date()) -> Bool {
		switch remembered.outcome {
		case .answer(.found), .answer(.instrumental):
			return true
		case .answer(.notFound):
			return now.timeIntervalSince(remembered.at) < Self.notFoundLifetime
		case .failure:
			return now.timeIntervalSince(remembered.at) < Self.failureLifetime
		}
	}

	private func cached(_ key: String) -> Remembered? {
		if let remembered = memory[key], isFresh(remembered) {
			return remembered
		}
		guard let remembered = stored(key), isFresh(remembered) else { return nil }
		remember(remembered, for: key)
		return remembered
	}

	private func remember(_ remembered: Remembered, for key: String) {
		if memory.count >= Self.maxInMemory {
			memory.removeAll()
		}
		memory[key] = remembered
	}

	private var context: NSManagedObjectContext {
		PlaylistController.sharedPersistentContainer().viewContext
	}

	private func row(_ key: String) -> LyricsCache? {
		let request = LyricsCache.fetchRequest()
		request.predicate = NSPredicate(format: "key == %@", key)
		request.fetchLimit = 1
		return (try? context.fetch(request))?.first
	}

	private func stored(_ key: String) -> Remembered? {
		guard let row = row(key) else { return nil }
		let plain = row.plain ?? ""
		let synced = row.synced ?? ""
		let answer: LrclibAnswer
		if !row.known {
			answer = .notFound
		} else if row.instrumental {
			answer = .instrumental(synced: synced)
		} else if plain.isEmpty && synced.isEmpty {
			answer = .notFound
		} else {
			answer = .found(plain: plain, synced: synced)
		}
		return Remembered(outcome: .answer(answer), at: row.fetchedAt ?? .distantPast)
	}

	private func store(_ answer: LrclibAnswer, for key: String, at date: Date) {
		let row = row(key) ?? LyricsCache(context: context)
		row.key = key
		row.fetchedAt = date
		switch answer {
		case let .found(plain, synced):
			row.known = true
			row.instrumental = false
			row.plain = plain
			row.synced = synced
		case let .instrumental(synced):
			row.known = true
			row.instrumental = true
			row.plain = ""
			row.synced = synced
		case .notFound:
			row.known = false
			row.instrumental = false
			row.plain = ""
			row.synced = ""
		}
		do {
			try context.save()
		} catch {
			NSLog("LRCLIB: could not save lyrics: %@", error.localizedDescription)
		}
	}

	// MARK: - Network

	private func lookup(_ query: LrclibQuery, completion: @escaping (String) -> Void) {
		let key = query.key
		if waiting[key] != nil {
			waiting[key]?.append(completion)
			return
		}
		waiting[key] = [completion]

		let previous = tail
		let client = client
		tail = Task { [weak self] in
			await previous?.value
			let outcome: Outcome
			do {
				outcome = .answer(try await client.get(query))
			} catch let error as LrclibError {
				outcome = .failure(error)
			} catch {
				outcome = .failure(.transport(error.localizedDescription))
			}
			self?.deliver(outcome, for: key)
		}
	}

	private func deliver(_ outcome: Outcome, for key: String) {
		let now = Date()
		remember(Remembered(outcome: outcome, at: now), for: key)
		if case let .answer(answer) = outcome {
			store(answer, for: key, at: now)
		}
		let text = Self.text(for: outcome)
		for completion in waiting.removeValue(forKey: key) ?? [] {
			completion(text)
		}
	}

	// MARK: - Presentation

	private static func text(for outcome: Outcome) -> String {
		switch outcome {
		case let .answer(.found(plain, synced)):
			return plain.isEmpty ? unsynced(synced) : plain
		case .answer(.instrumental):
			return NSLocalizedString("LRCLIB lists this track as instrumental.", comment: "Lyrics window: LRCLIB says the track has no words")
		case .answer(.notFound):
			return NSLocalizedString("LRCLIB has no lyrics for this track.", comment: "Lyrics window: LRCLIB has no record for the track")
		case .failure(.transport), .failure(.transient):
			return NSLocalizedString("Could not reach LRCLIB.", comment: "Lyrics window: LRCLIB is unreachable or busy")
		case .failure:
			return NSLocalizedString("LRCLIB could not answer for this track.", comment: "Lyrics window: LRCLIB returned an error")
		}
	}

	/// Plain text from LRC: timestamps dropped, header tags such as `[ar:…]`
	/// removed, blank lines kept as stanza breaks.
	static func unsynced(_ lrc: String) -> String {
		let timestamp = try! NSRegularExpression(pattern: #"\[\d+:\d+(?:[.:]\d+)?\]"#)
		let header = try! NSRegularExpression(pattern: #"^\s*\[[A-Za-z#]+:[^\]]*\]\s*$"#)
		var lines: [String] = []
		lrc.enumerateLines { line, _ in
			let range = NSRange(line.startIndex..., in: line)
			if header.firstMatch(in: line, range: range) != nil {
				return
			}
			let text = timestamp.stringByReplacingMatches(in: line, range: range, withTemplate: "")
			lines.append(text.trimmingCharacters(in: .whitespaces))
		}
		while lines.first?.isEmpty == true {
			lines.removeFirst()
		}
		while lines.last?.isEmpty == true {
			lines.removeLast()
		}
		return lines.joined(separator: "\n")
	}
}
