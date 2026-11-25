//
//  CueSheetMetadataReader.swift
//  CueSheet
//
//  Created by Vincent Spader on 10/12/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

import CogAudio
import Foundation

@objc(CueSheetMetadataReader)
class CueSheetMetadataReader: NSObject, CogMetadataReader {
	@objc class func fileTypes() -> [Any] {
		return CueSheetDecoder.fileTypes()
	}

	@objc class func mimeTypes() -> [Any] {
		return CueSheetDecoder.mimeTypes()
	}

	@objc class func priority() -> Float {
		return 16.0
	}

	@objc(metadataForURL:)
	class func metadata(for url: URL) -> [AnyHashable: Any]? {
		if !url.isFileURL {
			return nil
		}

		var embedded = false
		var cuesheet: CueSheet?
		var fileMetadata: [AnyHashable: Any]?

		let ext = url.pathExtension
		if ext.caseInsensitiveCompare("cue") != .orderedSame {
			// Embedded cuesheet check
			fileMetadata = AudioMetadataReader.metadata(for: url, skipCue: true)

			let alsoMetadata = AudioPropertiesReader.properties(for: url, skipCue: true)

			var sheetString = CueSheet.sheetString(in: fileMetadata)
			if sheetString == nil || sheetString!.isEmpty {
				sheetString = CueSheet.sheetString(in: alsoMetadata)
			}

			if let sheetString, !sheetString.isEmpty {
				cuesheet = CueSheet(string: sheetString, withFilename: url.path)
				embedded = true
			}
		} else {
			cuesheet = CueSheet(file: url.path)
		}

		guard let cuesheet else {
			return fileMetadata
		}

		for track in cuesheet.tracks {
			if url.fragment == track.track {
				if !embedded {
					fileMetadata = track.url.flatMap { AudioMetadataReader.metadata(for: $0, skipCue: true) }
				}

				return metadata(forTrack: track, fileMetadata: fileMetadata)
			}
		}

		return nil
	}

	class func metadata(forTrack track: CueSheetTrack, fileMetadata: [AnyHashable: Any]?) -> [AnyHashable: Any] {
		let cuesheetMetadata = processData(forTrack: track)

		// Cue-sheet fields describe this logical track and take priority over
		// the metadata shared by the underlying audio file. Do not use the
		// NSDictionary merge category here: plug-ins load their own category
		// implementations into one Objective-C runtime, so selector collisions
		// can silently change which dictionary wins based on bundle load order.
		var metadata = fileMetadata ?? [:]
		for (key, value) in cuesheetMetadata {
			metadata[key] = value
		}
		return metadata
	}

	class func processData(forTrack track: CueSheetTrack) -> [AnyHashable: Any] {
		var metadata: [AnyHashable: Any] = [:]
		if let artist = track.artist { metadata["artist"] = artist }
		if let album = track.album { metadata["album"] = album }
		if let title = track.title { metadata["title"] = title }
		metadata["track"] = (track.track as NSString).intValue
		if let genre = track.genre { metadata["genre"] = genre }
		if let year = track.year { metadata["date"] = year }
		metadata["replaygain_album_gain"] = track.albumGain
		metadata["replaygain_album_peak"] = track.albumPeak
		metadata["replaygain_track_gain"] = track.trackGain
		metadata["replaygain_track_peak"] = track.trackPeak
		return metadata
	}
}