//
//  PlaylistEntry+Extension.swift
//  CogPlaylist
//
//  What an entry is beyond its stored attributes, wherever Cog runs: its URL,
//  its tags (kept in the metadata blob), and what is shown of them. Moved
//  from the macOS app's PlaylistEntry (Extension) category under the same
//  Objective-C names, with the key paths that keep its bindings current; the
//  parts that need AppKit, the app's art cache or its strings stay in that
//  category.
//

import CoreData
import Foundation

extension PlaylistEntry {
	// MARK: - Tag names

	// Tag names with periods would be taken as key paths, so the blob keeps
	// them with U+2024 instead.
	@objc(keyForMetaTag:) public class func key(forMetaTag tagName: String) -> String {
		tagName.replacingOccurrences(of: ".", with: "\u{2024}")
	}

	@objc(metaTagForKey:) public class func metaTag(forKey key: String) -> String {
		key.replacingOccurrences(of: "\u{2024}", with: ".")
	}

	// MARK: - Dependent keys

	@objc public class var keyPathsForValuesAffectingUrl: Set<String> { ["urlString"] }
	@objc public class var keyPathsForValuesAffectingTrashUrl: Set<String> { ["trashUrlString"] }
	@objc public class var keyPathsForValuesAffectingTitle: Set<String> { ["rawTitle"] }
	@objc public class var keyPathsForValuesAffectingDisplay: Set<String> { ["artist", "title"] }
	@objc public class var keyPathsForValuesAffectingLength: Set<String> { ["metadataLoaded", "totalFrames", "sampleRate"] }
	@objc public class var keyPathsForValuesAffectingPath: Set<String> { ["url"] }
	@objc public class var keyPathsForValuesAffectingFilename: Set<String> { ["url"] }
	@objc public class var keyPathsForValuesAffectingFilenameFragment: Set<String> { ["url"] }
	@objc public class var keyPathsForValuesAffectingStatus: Set<String> { ["current", "queued", "error", "stopAfter"] }
	@objc public class var keyPathsForValuesAffectingTrackText: Set<String> { ["track", "disc"] }
	@objc public class var keyPathsForValuesAffectingYearText: Set<String> { ["year"] }
	@objc public class var keyPathsForValuesAffectingCuesheetPresent: Set<String> { ["cuesheet"] }
	@objc public class var keyPathsForValuesAffectingUnsigned: Set<String> { ["unSigned"] }
	@objc public class var keyPathsForValuesAffectingSoundcheckDisplay: Set<String> { ["soundcheck"] }
	@objc public class var keyPathsForValuesAffectingSoundcheckVolume: Set<String> { ["soundcheck"] }

	// The named tags all live in the metadata blob.
	@objc public class var keyPathsForValuesAffectingAlbum: Set<String> { ["metadataBlob"] }
	@objc public class var keyPathsForValuesAffectingAlbumartist: Set<String> { ["metadataBlob"] }
	@objc public class var keyPathsForValuesAffectingArtist: Set<String> { ["metadataBlob"] }
	@objc public class var keyPathsForValuesAffectingComposer: Set<String> { ["metadataBlob"] }
	@objc public class var keyPathsForValuesAffectingRawTitle: Set<String> { ["metadataBlob"] }
	@objc public class var keyPathsForValuesAffectingGenre: Set<String> { ["metadataBlob"] }
	@objc public class var keyPathsForValuesAffectingDisc: Set<String> { ["metadataBlob"] }
	@objc public class var keyPathsForValuesAffectingTrack: Set<String> { ["metadataBlob"] }
	@objc public class var keyPathsForValuesAffectingYear: Set<String> { ["metadataBlob"] }
	@objc public class var keyPathsForValuesAffectingDate: Set<String> { ["metadataBlob"] }
	@objc public class var keyPathsForValuesAffectingUnsyncedlyrics: Set<String> { ["metadataBlob"] }
	@objc public class var keyPathsForValuesAffectingComment: Set<String> { ["metadataBlob"] }

	// MARK: - URL

	@objc public var url: URL? {
		get { Self.url(forPath: urlString) }
		set { urlString = newValue.map { Self.normalize($0).absoluteString } }
	}

	@objc public var trashUrl: URL? {
		get { Self.url(forPath: trashUrlString) }
		set { trashUrlString = newValue.map { Self.normalize($0).absoluteString } }
	}

	/// The same as `url`, for the key Core Data's lowercase rule kept from it.
	@objc(URL) public var urlKey: URL? {
		get { url }
		set { url = newValue }
	}

	/// A stored URL string, or a path as older playlists kept it, perhaps
	/// with a cue track fragment ("/a/b.flac#01"); nothing is ten seconds of
	/// silence.
	static func url(forPath path: String?) -> URL? {
		guard let path, !path.isEmpty else { return URL(string: "silence://10") }

		if path.contains("://") {
			// Anything stored before file reference URLs were normalized on the
			// way in still has to be usable now.
			return URL(string: path).map(normalize)
		}

		let unixPath = NSMutableString(string: path)
		var fragment = ""
		let scanner = Scanner(string: path)
		let fragmentCharacters = CharacterSet(charactersIn: "#1234567890")
		while !scanner.isAtEnd {
			_ = scanner.scanUpToString("#")
			if let possibleFragment = scanner.scanCharacters(from: fragmentCharacters), scanner.isAtEnd {
				fragment = possibleFragment
				let location = path.utf16.distance(from: path.startIndex, to: scanner.currentIndex)
				let length = (possibleFragment as NSString).length
				unixPath.deleteCharacters(in: NSRange(location: location - length, length: length))
				break
			}
		}

		let filePath = normalize(filePath: unixPath as String)
		return URL(string: URL(fileURLWithPath: filePath).absoluteString + fragment)
	}

	/// File reference URLs (file:///.file/id=…), which name a file by volume
	/// and inode and go stale, resolved to paths, as CogNormalizeURL does.
	static func normalize(_ url: URL) -> URL {
		guard (url as NSURL).isFileReferenceURL(), let resolved = (url as NSURL).filePathURL else { return url }
		return resolved
	}

	/// A file reference path (/.file/id=…) resolved, as CogNormalizeFilePath
	/// does.
	static func normalize(filePath path: String) -> String {
		guard path.hasPrefix("/.file/id="), let url = URL(string: "file://" + path) else { return path }
		return (normalize(url) as NSURL).path ?? path
	}

	@objc public var path: String {
		guard let url else { return "" }
		if url.isFileURL {
			return ((url as NSURL).path as NSString?)?.abbreviatingWithTildeInPath ?? ""
		}
		return url.absoluteString
	}

	@objc public var filename: String {
		((url as NSURL?)?.path as NSString?)?.lastPathComponent ?? ""
	}

	@objc public var filenameFragment: String {
		if let fragment = (url as NSURL?)?.fragment {
			return filename + "#" + fragment
		}
		return filename
	}

	// MARK: - What is shown

	/// The title tag, or the file name when there is none.
	@objc public var title: String {
		get {
			if rawTitle?.isEmpty ?? true, let url {
				return ((url as NSURL).path as NSString?)?.lastPathComponent ?? ""
			}
			return rawTitle ?? ""
		}
		set { rawTitle = newValue }
	}

	/// "Artist - Title", or the title alone.
	@objc public var display: String {
		guard let artist, !artist.isEmpty else { return title }
		return "\(artist) - \(title)"
	}

	/// Seconds, once the properties have loaded.
	@objc public var length: NSNumber {
		metadataLoaded ? NSNumber(value: Double(totalFrames) / Double(sampleRate)) : NSNumber(value: 0.0)
	}

	/// What the playlist's status column draws.
	@objc public var status: String? {
		if stopAfter { return "stopAfter" }
		if current { return "playing" }
		if queued { return "queued" }
		if error { return "error" }
		return nil
	}

	/// "2.05" for disc 2, track 5; "05" without a disc; nothing without a
	/// track.
	@objc public var trackText: String {
		guard track != 0 else { return "" }
		if disc != 0 {
			return String(format: "%u.%02u", disc, track)
		}
		return String(format: "%02u", track)
	}

	@objc public var yearText: String {
		year != 0 ? String(format: "%u", year) : ""
	}

	@objc public var cuesheetPresent: String {
		(cuesheet?.isEmpty ?? true) ? "no" : "yes"
	}

	/// The same as `unSigned`, for the key Core Data's lowercase rule kept
	/// from it.
	@objc(Unsigned) public var unsignedKey: Bool {
		get { unSigned }
		set { unSigned = newValue }
	}

	// MARK: - Tags

	@objc public var album: String? {
		get { readAllValuesAsString("album") }
		set { setValue("album", fromString: newValue) }
	}

	@objc public var albumartist: String? {
		get { firstValue(of: "albumartist", "album artist", "album_artist") }
		set {
			setValue("albumartist", fromString: newValue)
			setValue("album artist", fromString: nil)
			setValue("album_artist", fromString: nil)
		}
	}

	@objc public var artist: String? {
		get { readAllValuesAsString("artist") }
		set { setValue("artist", fromString: newValue) }
	}

	@objc public var composer: String? {
		get { readAllValuesAsString("composer") }
		set { setValue("composer", fromString: newValue) }
	}

	@objc public var rawTitle: String? {
		get { readAllValuesAsString("title") }
		set { setValue("title", fromString: newValue) }
	}

	@objc public var genre: String? {
		get { readAllValuesAsString("genre") }
		set { setValue("genre", fromString: newValue) }
	}

	@objc public var disc: Int32 {
		get { firstValue(of: "discnumber", "discnum", "disc").map(Self.leadingInt32) ?? 0 }
		set {
			setValue("discnumber", fromString: String(format: "%u", newValue))
			setValue("discnum", fromString: nil)
			setValue("disc", fromString: nil)
		}
	}

	// The category declared this settable without implementing the setter, so
	// the one caller (SQLiteStore's import) would have failed on it.
	@objc public var track: Int32 {
		get { firstValue(of: "tracknumber", "tracknum", "track").map(Self.leadingInt32) ?? 0 }
		set {
			setValue("tracknumber", fromString: String(format: "%u", newValue))
			setValue("tracknum", fromString: nil)
			setValue("track", fromString: nil)
		}
	}

	@objc public var year: Int32 {
		get { date.map(Self.leadingInt32) ?? 0 }
		set { date = newValue != 0 ? String(format: "%u", newValue) : nil }
	}

	@objc public var date: String? {
		get { firstValue(of: "date", "recording_date", "year") }
		set {
			setValue("date", fromString: newValue)
			setValue("recording_date", fromString: nil)
			setValue("year", fromString: nil)
		}
	}

	/// Lyrics the file carries, unsynced.
	@objc public var unsyncedlyrics: String? {
		get { firstValue(of: "unsyncedlyrics", "unsynced lyrics", "lyrics") }
		set {
			setValue("unsyncedlyrics", fromString: newValue)
			setValue("unsynced lyrics", fromString: nil)
			setValue("lyrics", fromString: nil)
		}
	}

	@objc public var comment: String? {
		get { readAllValuesAsString("comment") }
		set { setValue("comment", fromString: newValue) }
	}

	private func firstValue(of tagNames: String...) -> String? {
		for tagName in tagNames {
			if let value = readAllValuesAsString(tagName) { return value }
		}
		return nil
	}

	/// The number a string starts with, as -[NSString intValue] reads it
	/// ("3/12" is 3, "2001-05-02" is 2001).
	private static func leadingInt32(_ string: String) -> Int32 {
		(string as NSString).intValue
	}

	// MARK: - The metadata blob

	private var metadataDictionary: NSDictionary? {
		metadataBlob as? NSDictionary
	}

	/// Every value of a tag, joined with ", ".
	@objc public func readAllValuesAsString(_ tagName: String) -> String? {
		(metadataDictionary?[Self.key(forMetaTag: tagName)] as? NSArray)?.componentsJoined(by: ", ")
	}

	@objc public func deleteAllValues() {
		metadataBlob = nil
	}

	@objc public func deleteValue(_ tagName: String) {
		guard let dictionary = metadataDictionary?.mutableCopy() as? NSMutableDictionary else { return }
		dictionary.removeObject(forKey: Self.key(forMetaTag: tagName))
		metadataBlob = NSDictionary(dictionary: dictionary)
	}

	/// Sets a tag's values from a ", "-separated string; nil removes it.
	@objc public func setValue(_ tagName: String, fromString value: String?) {
		guard let value else {
			deleteValue(tagName)
			return
		}
		guard let dictionary = metadataDictionary?.mutableCopy() as? NSMutableDictionary else { return }
		dictionary[Self.key(forMetaTag: tagName)] = value.components(separatedBy: ", ")
		metadataBlob = NSDictionary(dictionary: dictionary)
	}

	@objc public func addValue(_ tagName: String, fromString value: String) {
		guard let dictionary = metadataDictionary?.mutableCopy() as? NSMutableDictionary else { return }
		let key = Self.key(forMetaTag: tagName)
		let values = (dictionary[key] as? NSArray)?.mutableCopy() as? NSMutableArray ?? NSMutableArray()
		values.add(value)
		dictionary[key] = NSArray(array: values)
		metadataBlob = NSDictionary(dictionary: dictionary)
	}

	// MARK: - Sound Check

	/// The decibels an iTunNORM tag asks for: the lower of its first two
	/// adjustments, of ten or more eight-digit hex fields; 1 if it has too
	/// few.
	@objc(calculateSoundcheck:) public class func calculateSoundcheck(_ input: String) -> Float {
		let fields = input.components(separatedBy: " ").filter { ($0 as NSString).length == 8 }
		guard fields.count >= 10 else { return 1 }
		let value1 = Scanner(string: fields[0]).scanUInt64(representation: .hexadecimal) ?? 0
		let value2 = Scanner(string: fields[1]).scanUInt64(representation: .hexadecimal) ?? 0
		let volume1 = Float(-log10(Double(value1) / 1000) * 10)
		let volume2 = Float(-log10(Double(value2) / 1000) * 10)
		return min(volume1, volume2)
	}

	@objc public var soundcheckDisplay: String? {
		guard let soundcheck, !soundcheck.isEmpty else { return nil }
		return String(format: "%.6f dB", Self.calculateSoundcheck(soundcheck))
	}

	@objc public var soundcheckVolume: Float {
		guard let soundcheck, !soundcheck.isEmpty else { return 1 }
		return Float(pow(10.0, Double(Self.calculateSoundcheck(soundcheck)) / 20.0))
	}
}
