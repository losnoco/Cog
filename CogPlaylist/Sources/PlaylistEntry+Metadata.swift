//
//  PlaylistEntry+Metadata.swift
//  CogPlaylist
//
//  Storing what the plugins report for an entry, as the macOS app's
//  -[PlaylistEntry setMetadata:] does; what an entry is otherwise is in
//  PlaylistEntry+Extension.swift. Keep the two setMetadata in step until the
//  loader is shared too.
//

import CoreData
import Foundation

/// Posted (object: the entry) once an entry's metadata has been loaded.
public let CogPlaylistEntryMetadataLoadedNotification = Notification.Name("CogPlaylistEntryMetadataLoadedNotification")

extension PlaylistEntry {
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
		var dictionary = metadataBlob as? [String: Any] ?? [:]
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
				if let data = valueObject as? Data { setAlbumArt(data) }
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
