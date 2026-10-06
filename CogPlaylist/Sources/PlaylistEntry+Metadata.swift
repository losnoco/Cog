//
//  PlaylistEntry+Metadata.swift
//  CogPlaylist
//
//  Storing what the plugins report for an entry: audio properties and
//  ReplayGain in their attributes, the album art by its hash, and every other
//  tag in the metadata blob. Moved from the macOS app's PlaylistEntry
//  (Extension) category under the same Objective-C name.
//

import CoreData
import Foundation

/// Posted (object: the entry) once an entry's metadata has been loaded; the
/// macOS app names it CogPlaylistEntryMetadataLoadedNotification as well.
public let CogPlaylistEntryMetadataLoadedNotification = Notification.Name("CogPlaylistEntryMetadataLoadedNotification")

extension PlaylistEntry {
	/// Stores what PlaylistEntryInfo read for the entry; nil marks it as
	/// unreadable. Either way the entry's metadata counts as loaded.
	@objc(setMetadata:) public func setMetadata(_ metadata: [String: Any]?) {
		if let metadata {
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
				let number = NSString(string: firstValue ?? "")
				switch lowerKey {
				case "bitrate": bitrate = number.intValue
				case "bitspersample": bitsPerSample = number.intValue
				case "channelconfig": channelConfig = number.intValue
				case "channels": channels = number.intValue
				case "codec": codec = firstValue
				case "cuesheet": cuesheet = firstValue
				case "encoding": encoding = firstValue
				case "endian": endian = firstValue
				case "floatingpoint": floatingPoint = number.boolValue
				case "samplerate": sampleRate = number.floatValue
				case "seekable": seekable = number.boolValue
				case "totalframes": totalFrames = Int64(number.integerValue)
				case "unsigned": unSigned = number.boolValue
				case "replaygain_album_gain": replayGainAlbumGain = number.floatValue
				case "replaygain_album_peak": replayGainAlbumPeak = number.floatValue
				case "replaygain_track_gain": replayGainTrackGain = number.floatValue
				case "replaygain_track_peak": replayGainTrackPeak = number.floatValue
				case "soundcheck": soundcheck = firstValue
				case "volume": volume = number.floatValue
				case "albumart": albumArtInternal = valueObject as? Data
				default: dictionary[key] = genericValue
				}
			}
			metadataBlob = NSDictionary(dictionary: dictionary)
		} else {
			error = true
			errorMessage = Bundle.main.localizedString(forKey: "ErrorMetadata", value: "Unable to retrieve metadata.", table: nil)
		}
		metadataLoaded = true
		NotificationCenter.default.post(name: CogPlaylistEntryMetadataLoadedNotification, object: self)
	}
}

extension PlaylistEntry {
	/// What ReplayGain needs, as the engine takes it (`EngineTrack.rgInfo`).
	public var replayGainInfo: [String: Any] {
		["replaygain_album_gain": replayGainAlbumGain, "replaygain_album_peak": replayGainAlbumPeak,
		 "replaygain_track_gain": replayGainTrackGain, "replaygain_track_peak": replayGainTrackPeak,
		 "volume": volume]
	}
}
