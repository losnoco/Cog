//
//  SyncedLyrics.swift
//  CogPlaylist
//
//  Lyrics timed line by line, from LRC text: a tag's, a file's or LRCLIB's.
//

import Foundation

/// LRC text as timed lines, in order of time.
public struct SyncedLyrics: Equatable, Sendable {
	public struct Line: Equatable, Identifiable, Sendable {
		/// Its place among the lines.
		public let id: Int
		/// Seconds into the track.
		public let time: Double
		/// Its words, over several lines where the LRC ran on without a
		/// timestamp; empty for a gap between lines.
		public let text: String
	}

	public let lines: [Line]

	/// Nil unless `lrc` has lines at two different times at least: text with
	/// every line stamped zero is not timed, whatever it looks like.
	public init?(_ lrc: String) {
		let parsed = Self.parse(lrc)
		let lines = parsed.enumerated()
			.sorted { ($0.element.time, $0.offset) < ($1.element.time, $1.offset) }
			.enumerated()
			.map { Line(id: $0.offset, time: $0.element.element.time, text: $0.element.element.text) }
		guard Set(lines.map(\.time)).count > 1 else { return nil }
		self.lines = lines
	}

	/// The line being sung `seconds` into the track; nil before the first.
	public func lineIndex(at seconds: Double) -> Int? {
		var low = 0, high = lines.count
		while low < high {
			let middle = (low + high) / 2
			if lines[middle].time <= seconds {
				low = middle + 1
			} else {
				high = middle
			}
		}
		return low > 0 ? low - 1 : nil
	}

	/// `text` to read, with timestamps and header tags gone and gaps kept as
	/// blank lines. Text with no timestamps comes back as it is.
	public static func plainText(from text: String) -> String {
		let parsed = parse(text)
		guard parsed.contains(where: \.isTimed) else { return text }
		// In the order written: a line sung twice is written once.
		var seen = Set<Int>()
		return parsed.filter { seen.insert($0.group).inserted }
			.map(\.text)
			.joined(separator: "\n")
			.trimmingCharacters(in: .whitespacesAndNewlines)
	}

	// MARK: - Parsing

	private struct Parsed {
		var time: Double
		var text: String
		/// The source line it came from: `[00:10][00:50]Words` gives two.
		var group: Int
		/// False for words before any timestamp, which start at zero.
		var isTimed = true
	}

	private static func parse(_ lrc: String) -> [Parsed] {
		var parsed: [Parsed] = []
		var offset = 0.0
		var group = 0
		lrc.enumerateLines { line, _ in
			var rest = Substring(line.trimmingCharacters(in: .whitespaces))
			var times: [Double] = []
			var tagOffset: Double?
			var wasHeader = false
			while rest.hasPrefix("["), let end = rest.firstIndex(of: "]") {
				let tag = rest[rest.index(after: rest.startIndex)..<end]
				if let time = timestamp(tag) {
					times.append(time)
				} else if let value = headerValue(tag, in: rest[rest.index(after: end)...]) {
					wasHeader = true
					if tag.lowercased().hasPrefix("offset:") { tagOffset = Double(value.trimmingCharacters(in: .whitespaces)) }
				} else {
					break
				}
				rest = rest[rest.index(after: end)...]
			}
			if let tagOffset { offset = tagOffset / 1000 }
			let text = removingWordStamps(rest).trimmingCharacters(in: .whitespaces)
			if !times.isEmpty {
				group += 1
				parsed += times.map { Parsed(time: $0, text: text, group: group) }
			} else if !wasHeader, !text.isEmpty {
				// Words without a time run on from the line before.
				if let last = parsed.last, last.group == group {
					for index in parsed.indices where parsed[index].group == group {
						parsed[index].text += parsed[index].text.isEmpty ? text : "\n" + text
					}
				} else {
					group += 1
					parsed.append(Parsed(time: 0, text: text, group: group, isTimed: false))
				}
			}
		}
		// A positive offset brings the words forward.
		return parsed.map { Parsed(time: max($0.time - offset, 0), text: $0.text, group: $0.group, isTimed: $0.isTimed) }
	}

	/// Seconds from `mm:ss`, `mm:ss.xx` or `mm:ss:xx`.
	private static func timestamp(_ tag: Substring) -> Double? {
		let parts = tag.split(separator: ":", omittingEmptySubsequences: false)
		guard parts.count == 2 || parts.count == 3 else { return nil }
		let seconds = parts[1].split(separator: ".", maxSplits: 1, omittingEmptySubsequences: false)
		let fraction = parts.count == 3 ? parts[2] : (seconds.count == 2 ? seconds[1] : "0")
		guard seconds.count == 1 || parts.count == 2,
		      isDigits(parts[0]), isDigits(seconds[0]), isDigits(fraction),
		      let minutes = Double(parts[0]), let whole = Double(seconds[0]), let part = Double("0.\(fraction)") else {
			return nil
		}
		return minutes * 60 + whole + part
	}

	private static func isDigits(_ text: Substring) -> Bool {
		!text.isEmpty && text.allSatisfy { $0.isASCII && $0.isNumber }
	}

	/// The value of a tag such as `ar:Artist`, if it stands alone on its
	/// line or before more tags: `[Verse: Name] Words` is words.
	private static func headerValue(_ tag: Substring, in following: Substring) -> Substring? {
		guard let colon = tag.firstIndex(of: ":"), colon != tag.startIndex,
		      tag[..<colon].allSatisfy({ $0.isASCII && ($0.isLetter || $0 == "#") }),
		      following.trimmingCharacters(in: .whitespaces).isEmpty || following.hasPrefix("[") else {
			return nil
		}
		return tag[tag.index(after: colon)...]
	}

	/// Enhanced LRC's `<mm:ss.xx>` before each word: lines are as fine as
	/// this gets.
	private static func removingWordStamps(_ text: Substring) -> String {
		var result = ""
		var rest = text
		while let open = rest.firstIndex(of: "<"), let close = rest[open...].firstIndex(of: ">") {
			result += rest[..<open]
			if timestamp(rest[rest.index(after: open)..<close]) == nil {
				result += rest[open...close]
			}
			rest = rest[rest.index(after: close)...]
		}
		return result + rest
	}
}
