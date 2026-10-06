//
//  PlaylistEntryInfo.swift
//  CogPlaylist
//
//  An entry's properties and tags, read through the plugins and merged, as
//  the playlist loader stores them with -[PlaylistEntry setMetadata:]. Moved
//  from the macOS app's PlaylistLoader.m (entryInfoForURL() and
//  isCueSheetTrackURL()).
//

import CogAudio
import Foundation

@objc public final class PlaylistEntryInfo: NSObject {
	/// Properties and tags for the entry at `url`; nil if the file cannot be
	/// read. A cue sheet track's own tags win over the shared audio file's.
	@objc(infoForURL:) public static func info(for url: URL) -> [String: Any]? {
		PluginCalls.run(for: url) { readInfo(for: url) }
	}

	private static func readInfo(for url: URL) -> [String: Any]? {
		let cueSheetTrack = isCueSheetTrack(url)
		// Resolve the cue track's own tags before opening a decoder for
		// properties: that may read the shared audio file, whose tags do not
		// describe one track.
		var metadata = cueSheetTrack ? AudioMetadataReader.metadata(for: url) as? [String: Any] : nil
		guard let properties = AudioPropertiesReader.properties(for: url) as? [String: Any] else { return nil }
		if metadata == nil {
			metadata = AudioMetadataReader.metadata(for: url) as? [String: Any] ?? [:]
		}
		if cueSheetTrack {
			// The decoder's properties may carry the album file's tags; the
			// cue track's replace them unconditionally.
			return properties.merging(metadata ?? [:]) { _, cue in cue }
		}
		return merge(properties, with: metadata ?? [:])
	}

	/// Whether `url` is one track of a cue sheet: a fragment of a .cue file,
	/// or of an audio file with one embedded (album.flac#01), not merely a
	/// subsong.
	@objc(isCueSheetTrackURL:) public static func isCueSheetTrack(_ url: URL) -> Bool {
		PluginCalls.run(for: url) { checkCueSheetTrack(url) } ?? false
	}

	private static func checkCueSheetTrack(_ url: URL) -> Bool {
		guard url.isFileURL, let fragment = url.fragment, !fragment.isEmpty else { return false }
		if url.pathExtension.caseInsensitiveCompare("cue") == .orderedSame { return true }
		var components = URLComponents(url: url, resolvingAgainstBaseURL: false)
		components?.fragment = nil
		guard let base = components?.url else { return false }
		if hasContent(AudioMetadataReader.metadata(for: base, skipCue: true)?["cuesheet"]) { return true }
		return hasContent(AudioPropertiesReader.properties(for: base, skipCue: true)?["cuesheet"])
	}

	private static func hasContent(_ value: Any?) -> Bool {
		if let string = value as? String { return !string.isEmpty }
		if let array = value as? [Any] { return array.contains { ($0 as? String)?.isEmpty == false } }
		return false
	}

	/// The first dictionary, with what the second has that it lacks or has
	/// empty, nested dictionaries merged alike, as
	/// +[NSDictionary dictionaryByMerging:with:] does.
	static func merge(_ first: [String: Any], with second: [String: Any]) -> [String: Any] {
		var result = first
		for (key, value) in second {
			guard let existing = first[key] else {
				result[key] = value
				continue
			}
			if let existingDictionary = existing as? [String: Any], let valueDictionary = value as? [String: Any] {
				result[key] = merge(existingDictionary, with: valueDictionary)
			} else if isEmpty(existing) {
				result[key] = value
			}
		}
		return result
	}

	private static func isEmpty(_ value: Any) -> Bool {
		switch value {
		case let string as String: return string.isEmpty
		case let number as NSNumber: return number == 0
		case let data as Data: return data.isEmpty
		default: return false
		}
	}
}
