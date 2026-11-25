//
//  PlaylistEntry+AppKit.swift
//  Cog
//
//  Created by Vincent Spader on 3/14/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//
//  The AppKit-facing half of the entry: what needs the art cache, the Core
//  Data stack, or the app's strings. The tag/URL half is in CogPlaylist.
//

import AppKit
import CogPlaylist
import CoreData

extension PlaylistEntry {
	// The following read-only keys depend on the values of other properties.

	@objc static var keyPathsForValuesAffectingStatusMessage: Set<String> {
		["current", "queued", "queuePosition", "error", "errorMessage", "stopAfter"]
	}
	@objc static var keyPathsForValuesAffectingSpam: Set<String> {
		["albumartist", "artist", "rawTitle", "album", "track", "disc", "totalFrames", "currentPosition", "bitrate"]
	}
	@objc static var keyPathsForValuesAffectingIndexedSpam: Set<String> {
		["albumartist", "artist", "rawTitle", "album", "track", "disc", "totalFrames", "currentPosition", "bitrate", "index"]
	}
	@objc static var keyPathsForValuesAffectingPositionText: Set<String> { ["currentPosition"] }
	@objc static var keyPathsForValuesAffectingLengthText: Set<String> { ["length"] }
	@objc static var keyPathsForValuesAffectingLengthInfo: Set<String> { ["length"] }
	@objc static var keyPathsForValuesAffectingAlbumArt: Set<String> { ["albumArtInternal", "artId"] }
	@objc static var keyPathsForValuesAffectingGainCorrection: Set<String> {
		["replayGainAlbumGain", "replayGainAlbumPeak", "replayGainTrackGain", "replayGainTrackPeak", "soundcheck", "volume"]
	}
	@objc static var keyPathsForValuesAffectingGainInfo: Set<String> {
		["replayGainAlbumGain", "replayGainAlbumPeak", "replayGainTrackGain", "replayGainTrackPeak", "soundcheck", "volume"]
	}

	@objc var indexedSpam: String {
		return "\(index). \(spam)"
	}

	@objc var spam: String {
		let hasBitrate = bitrate != 0
		let hasArtist = (artist ?? "").isEmpty == false
		let hasAlbumArtist = (albumartist ?? "").isEmpty == false
		let hasTrackArtist = hasArtist && hasAlbumArtist && albumartist != artist
		let hasAlbum = (album ?? "").isEmpty == false
		let hasTrack = track != 0
		let hasLength = totalFrames != 0
		let hasCurrentPosition = currentPosition != 0 && current
		let hasTitle = (rawTitle ?? "").isEmpty == false
		let hasCodec = (codec ?? "").isEmpty == false

		var filename = self.filename
		var ext: String?
		if let dot = filename.lastIndex(of: ".") {
			ext = String(filename[filename.index(after: dot)...])
			filename = String(filename[..<dot])
		}
		let hasExtension = ext != nil

		var elements: [String] = []

		if hasExtension {
			elements.append("[")
			elements.append(hasCodec ? codec! : ext!.uppercased())
			if hasBitrate {
				elements.append("@")
				elements.append("\(bitrate)")
				elements.append("kbps")
			}
			elements.append("] ")
		}

		if hasArtist {
			elements.append(hasAlbumArtist ? albumartist! : artist!)
			elements.append(" - ")
		}

		if hasAlbum {
			elements.append("[")
			elements.append(album!)
			if hasTrack {
				elements.append(" #")
				elements.append(trackText)
			}
			elements.append("] ")
		}

		elements.append(hasTitle ? rawTitle! : filename)

		if hasTrackArtist {
			elements.append(" // ")
			elements.append(artist!)
		}

		if hasCurrentPosition || hasLength {
			let formatter = SecondsFormatter()
			elements.append(" (")
			if hasCurrentPosition {
				elements.append(formatter.string(for: currentPosition) ?? "")
			}
			if hasLength {
				if hasCurrentPosition { elements.append(" / ") }
				elements.append(formatter.string(for: length) ?? "")
			}
			elements.append(")")
		}

		return elements.joined()
	}

	@objc var gainCorrection: String {
		if replayGainAlbumGain != 0 {
			return replayGainAlbumPeak != 0 ? NSLocalizedString("GainAlbumGainPeak", comment: "") : NSLocalizedString("GainAlbumGain", comment: "")
		} else if replayGainTrackGain != 0 {
			return replayGainTrackPeak != 0 ? NSLocalizedString("GainTrackGainPeak", comment: "") : NSLocalizedString("GainTrackGain", comment: "")
		} else if let soundcheck, !soundcheck.isEmpty {
			return NSLocalizedString("GainSoundcheck", comment: "")
		} else if volume != 0 && volume != 1.0 {
			return NSLocalizedString("GainVolumeScale", comment: "")
		} else {
			return NSLocalizedString("GainNone", comment: "")
		}
	}

	@objc var gainInfo: String {
		var items: [String] = []
		if replayGainAlbumGain != 0 {
			items.append(String(format: "%@: %+.2f dB", NSLocalizedString("GainAlbumGain", comment: ""), replayGainAlbumGain))
		}
		if replayGainAlbumPeak != 0 {
			items.append(String(format: "%@: %.6f", NSLocalizedString("GainAlbumPeak", comment: ""), replayGainAlbumPeak))
		}
		if replayGainTrackGain != 0 {
			items.append(String(format: "%@: %+.2f dB", NSLocalizedString("GainTrackGain", comment: ""), replayGainTrackGain))
		}
		if replayGainTrackPeak != 0 {
			items.append(String(format: "%@: %.6f", NSLocalizedString("GainTrackPeak", comment: ""), replayGainTrackPeak))
		}
		if let soundcheck, !soundcheck.isEmpty, let scdisplay = soundcheckDisplay, !scdisplay.isEmpty {
			items.append("\(NSLocalizedString("GainSoundcheck", comment: "")): \(scdisplay)")
		}
		if volume != 0 && volume != 1 {
			items.append(String(format: "%@: %.2f\u{00D7}", NSLocalizedString("GainVolumeScale", comment: ""), volume))
		}
		return items.joined(separator: "\n")
	}

	@objc var positionText: String {
		return SecondsFormatter().string(for: currentPosition) ?? ""
	}

	@objc var lengthText: String {
		return SecondsFormatter().string(for: length) ?? ""
	}

	@objc var lengthInfo: String {
		return SecondsFractionFormatter().string(for: length) ?? ""
	}

	@objc var albumArt: NSImage? {
		guard let data = albumArtInternal, !data.isEmpty else { return nil }

		let tag = artHash ?? ""
		if let image = NSImage(named: tag) { return image }

		var image: NSImage?
		if #available(macOS 13.0, *) {
			image = NSImage(data: data)
		} else {
			if AVIFDecoder.isAVIFFormat(for: data) {
				if let cg = AVIFDecoder.createAVIFImage(with: data) {
					image = NSImage(cgImage: cg, size: .zero)
				}
			} else {
				image = NSImage(data: data)
			}
		}
		image?.setName(tag)
		return image
	}

	@objc func setAlbumArt(_ data: Any?) {
		if let data = data as? Data {
			albumArtInternal = data
		}
	}

	@objc var statusMessage: String? {
		if stopAfter {
			return NSLocalizedString("StatusStopAfter", comment: "")
		} else if current {
			return NSLocalizedString("StatusPlaying", comment: "")
		} else if queued {
			return String(format: NSLocalizedString("StatusQueued", comment: ""), queuePosition + 1)
		} else if error {
			return errorMessage
		}
		return nil
	}

	@objc var playCountItem: PlayCount? {
		let albumPredicate = NSPredicate(format: "album == %@", album ?? "")
		let artistPredicate = NSPredicate(format: "artist == %@", artist ?? "")
		let titlePredicate = NSPredicate(format: "title == %@", title)
		let predicate = NSCompoundPredicate(andPredicateWithSubpredicates: [albumPredicate, artistPredicate, titlePredicate])

		var fixtags = false
		var item: PlayCount?

		kPersistentContainer.viewContext.performAndWait {
			let request = NSFetchRequest<PlayCount>(entityName: "PlayCount")
			request.predicate = predicate
			var results = (try? kPersistentContainer.viewContext.fetch(request)) ?? []

			if results.isEmpty {
				request.predicate = NSPredicate(format: "filename == %@", filenameFragment)
				results = (try? kPersistentContainer.viewContext.fetch(request)) ?? []
				if results.isEmpty {
					request.predicate = NSPredicate(format: "filename == %@", filename)
					results = (try? kPersistentContainer.viewContext.fetch(request)) ?? []
				}
				if !results.isEmpty { fixtags = true }
			}

			item = results.first
		}

		if fixtags, let item {
			// shoot, something inserted the play counts without the tags
			kPersistentContainer.viewContext.performAndWait {
				item.album = album
				item.artist = artist
				item.title = title
				item.filename = filenameFragment
			}
			try? kPersistentContainer.viewContext.save()
		}

		return item
	}

	@objc var playCount: String {
		if let pc = playCountItem { return "\(pc.count)" }
		return "0"
	}

	@objc var playCountInfo: String {
		guard let pc = playCountItem else { return "" }
		let dateFormatter = DateFormatter()
		dateFormatter.dateStyle = .medium
		dateFormatter.timeStyle = .short
		if pc.count != 0 {
			return "\(NSLocalizedString("TimeFirstSeen", comment: "")): \(dateFormatter.string(from: pc.firstSeen ?? Date()))\n\(NSLocalizedString("TimeLastPlayed", comment: "")): \(dateFormatter.string(from: pc.lastPlayed ?? Date()))"
		}
		return "\(NSLocalizedString("TimeFirstSeen", comment: "")): \(dateFormatter.string(from: pc.firstSeen ?? Date()))"
	}

	@objc var rating: Float {
		return playCountItem?.rating ?? 0
	}

	@objc var audioScrobblerTrack: AudioScrobblerTrack {
		let track = AudioScrobblerTrack(title: title, artist: artist, albumArtist: albumartist, album: album, trackNumber: Int(track), length: (length as NSNumber?)?.doubleValue ?? 0)
		// TagLib, FLAC, Vorbis and Opus lowercase the Picard names; FFmpeg keeps
		// the MP4/ID3 spelling.
		track.recordingMBID = readAllValuesAsString("musicbrainz_trackid") ?? readAllValuesAsString("musicbrainz track id")
		track.releaseMBID = readAllValuesAsString("musicbrainz_albumid") ?? readAllValuesAsString("musicbrainz album id")
		let artistIDs = readAllValuesAsString("musicbrainz_artistid") ?? readAllValuesAsString("musicbrainz artist id")
		if let artistIDs, !artistIDs.isEmpty {
			var ids: [String] = []
			for part in artistIDs.components(separatedBy: CharacterSet(charactersIn: ",/;")) {
				let trimmed = part.trimmingCharacters(in: .whitespaces)
				if !trimmed.isEmpty { ids.append(trimmed) }
			}
			track.artistMBIDs = ids
		}
		return track
	}
}