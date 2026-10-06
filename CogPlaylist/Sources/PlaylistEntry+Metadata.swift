//
//  PlaylistEntry+Metadata.swift
//  CogPlaylist
//
//  What macOS's PlaylistEntry (Extension) category gives an entry, for the
//  parts that are not AppKit: its URL, its tags (kept in the metadata blob)
//  and how plugin dictionaries are stored. Ported from Playlist/PlaylistEntry.m;
//  keep the two in step.
//

import CoreData
import Foundation

/// Posted (object: the entry) once an entry's metadata has been loaded.
public let CogPlaylistEntryMetadataLoadedNotification = Notification.Name("CogPlaylistEntryMetadataLoadedNotification")

extension PlaylistEntry {
	// MARK: - URL

	/// The entry's URL, from its stored string (or a path, as older
	/// playlists kept it), file reference URLs resolved.
	public var url: URL? {
		get { Self.url(forPath: urlString) }
		set { urlString = newValue.map { Self.normalize($0).absoluteString } }
	}

	static func url(forPath path: String?) -> URL? {
		guard let path, !path.isEmpty else { return URL(string: "silence://10") }
		if path.contains("://") {
			return URL(string: path).map(normalize)
		}
		// A bare path, perhaps with a cue track fragment ("/a/b.flac#01").
		var filePath = path
		var fragment = ""
		if let hash = path.lastIndex(of: "#"), path[path.index(after: hash)...].allSatisfy(\.isNumber) {
			fragment = String(path[hash...])
			filePath = String(path[..<hash])
		}
		return URL(string: URL(fileURLWithPath: filePath).absoluteString + fragment)
	}

	/// File reference URLs (file:///.file/id=…) resolved to paths, as
	/// CogNormalizeURL does.
	static func normalize(_ url: URL) -> URL {
		guard (url as NSURL).isFileReferenceURL(), let resolved = (url as NSURL).filePathURL else { return url }
		return resolved
	}

	public var filename: String { url?.lastPathComponent ?? "" }

	// MARK: - Display

	/// The title tag, or the file name when there is none.
	public var title: String {
		get {
			if let rawTitle, !rawTitle.isEmpty { return rawTitle }
			return url?.lastPathComponent ?? ""
		}
		set { rawTitle = newValue }
	}

	/// "Artist - Title", or the title alone.
	public var display: String {
		guard let artist, !artist.isEmpty else { return title }
		return "\(artist) - \(title)"
	}

	/// Seconds, once the properties have loaded.
	public var length: Double {
		metadataLoaded && sampleRate > 0 ? Double(totalFrames) / Double(sampleRate) : 0
	}

	// MARK: - Tags

	// Tag names with periods would be taken as key paths, so the blob keeps
	// them with U+2024 instead.
	static func key(forMetaTag tagName: String) -> String {
		tagName.replacingOccurrences(of: ".", with: "\u{2024}")
	}

	static func metaTag(forKey key: String) -> String {
		key.replacingOccurrences(of: "\u{2024}", with: ".")
	}

	private var metadataDictionary: [String: Any]? {
		metadataBlob as? [String: Any]
	}

	/// Every value of a tag, joined with ", ".
	public func readAllValuesAsString(_ tagName: String) -> String? {
		guard let values = metadataDictionary?[Self.key(forMetaTag: tagName)] as? [Any] else { return nil }
		return values.map { "\($0)" }.joined(separator: ", ")
	}

	/// Sets a tag's values from a ", "-separated string; nil removes it.
	public func setValue(_ tagName: String, fromString value: String?) {
		guard let value else {
			deleteValue(tagName)
			return
		}
		guard var dictionary = metadataDictionary else { return }
		dictionary[Self.key(forMetaTag: tagName)] = value.components(separatedBy: ", ")
		metadataBlob = dictionary as NSDictionary
	}

	public func deleteValue(_ tagName: String) {
		guard var dictionary = metadataDictionary else { return }
		dictionary.removeValue(forKey: Self.key(forMetaTag: tagName))
		metadataBlob = dictionary as NSDictionary
	}

	private func firstOf(_ tagNames: String...) -> String? {
		for tagName in tagNames {
			if let value = readAllValuesAsString(tagName) { return value }
		}
		return nil
	}

	public var album: String? {
		get { readAllValuesAsString("album") }
		set { setValue("album", fromString: newValue) }
	}

	public var albumartist: String? {
		get { firstOf("albumartist", "album artist", "album_artist") }
		set {
			setValue("albumartist", fromString: newValue)
			setValue("album artist", fromString: nil)
			setValue("album_artist", fromString: nil)
		}
	}

	public var artist: String? {
		get { readAllValuesAsString("artist") }
		set { setValue("artist", fromString: newValue) }
	}

	public var composer: String? {
		get { readAllValuesAsString("composer") }
		set { setValue("composer", fromString: newValue) }
	}

	public var rawTitle: String? {
		get { readAllValuesAsString("title") }
		set { setValue("title", fromString: newValue) }
	}

	public var genre: String? {
		get { readAllValuesAsString("genre") }
		set { setValue("genre", fromString: newValue) }
	}

	public var disc: Int32 {
		Int32(firstOf("discnumber", "discnum", "disc").flatMap { Int32(leadingInteger: $0) } ?? 0)
	}

	public var track: Int32 {
		Int32(firstOf("tracknumber", "tracknum", "track").flatMap { Int32(leadingInteger: $0) } ?? 0)
	}

	public var date: String? {
		firstOf("date", "recording_date", "year")
	}

	public var year: Int32 {
		date.flatMap { Int32(leadingInteger: $0) } ?? 0
	}

	public var comment: String? { readAllValuesAsString("comment") }

	// MARK: - Loading

	/// Stores what the plugins report for the entry, as macOS does: audio
	/// properties and ReplayGain in their attributes, the album art aside,
	/// and every other tag in the metadata blob. nil marks the entry as
	/// unreadable.
	public func setMetadata(_ metadata: [String: Any]?) {
		guard let metadata else {
			error = true
			errorMessage = NSLocalizedString("ErrorMetadata", bundle: Bundle(for: PlaylistStore.self), value: "Unable to retrieve metadata.", comment: "An entry whose file could not be read")
			metadataLoaded = true
			NotificationCenter.default.post(name: CogPlaylistEntryMetadataLoadedNotification, object: self)
			return
		}
		var dictionary = metadataDictionary ?? [:]
		volume = 1
		for (key, valueObject) in metadata {
			let lowerKey = Self.metaTag(forKey: key).lowercased()
			var firstValue: String?
			let genericValue: Any
			switch valueObject {
			case let values as [Any]:
				firstValue = values.first.map { "\($0)" }
				genericValue = values
			case let string as String:
				firstValue = string
				genericValue = [string]
			case let number as NSNumber:
				firstValue = number.stringValue
				genericValue = [number.stringValue]
			default:
				genericValue = valueObject
			}
			let number = firstValue.map { NSString(string: $0) }
			switch lowerKey {
			case "bitrate": bitrate = number?.intValue ?? 0
			case "bitspersample": bitsPerSample = number?.intValue ?? 0
			case "channelconfig": channelConfig = number?.intValue ?? 0
			case "channels": channels = number?.intValue ?? 0
			case "codec": codec = firstValue
			case "cuesheet": cuesheet = firstValue
			case "encoding": encoding = firstValue
			case "endian": endian = firstValue
			case "floatingpoint": floatingPoint = number?.boolValue ?? false
			case "samplerate": sampleRate = number?.floatValue ?? 0
			case "seekable": seekable = number?.boolValue ?? false
			case "totalframes": totalFrames = number?.longLongValue ?? 0
			case "unsigned": unSigned = number?.boolValue ?? false
			case "replaygain_album_gain": replayGainAlbumGain = number?.floatValue ?? 0
			case "replaygain_album_peak": replayGainAlbumPeak = number?.floatValue ?? 0
			case "replaygain_track_gain": replayGainTrackGain = number?.floatValue ?? 0
			case "replaygain_track_peak": replayGainTrackPeak = number?.floatValue ?? 0
			case "soundcheck": soundcheck = firstValue
			case "volume": volume = number?.floatValue ?? 1
			case "albumart":
				// Album art is the app's to store (macOS keeps it in
				// AlbumArtwork, by hash); the entry keeps none.
				break
			default:
				dictionary[key] = genericValue
			}
		}
		metadataBlob = dictionary as NSDictionary
		metadataLoaded = true
		NotificationCenter.default.post(name: CogPlaylistEntryMetadataLoadedNotification, object: self)
	}

	/// What ReplayGain needs, as the engine takes it (`EngineTrack.rgInfo`).
	public var replayGainInfo: [String: Any] {
		["replaygain_album_gain": replayGainAlbumGain, "replaygain_album_peak": replayGainAlbumPeak,
		 "replaygain_track_gain": replayGainTrackGain, "replaygain_track_peak": replayGainTrackPeak,
		 "volume": volume]
	}
}

private extension Int32 {
	/// The number a string starts with, as -[NSString intValue] reads it
	/// ("3/12" is 3, "2001-05-02" is 2001).
	init?(leadingInteger string: String) {
		self = NSString(string: string).intValue
	}
}
