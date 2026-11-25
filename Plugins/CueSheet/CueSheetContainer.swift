//
//  CueSheetContainer.swift
//  CueSheet
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

import CogAudio
import Foundation

@objc(CueSheetContainer)
class CueSheetContainer: NSObject, CogContainer {
	@objc class func fileTypes() -> [Any] {
		return ["cue", "ogg", "opus", "flac", "wv", "mp3"]
	}

	@objc class func mimeTypes() -> [Any] {
		return ["application/x-cue"] // This is basically useless
	}

	@objc class func priority() -> Float {
		return 16.0
	}

	@objc(urlsForContainerURL:)
	class func urls(forContainerURL url: URL) -> [Any] {
		return urls(forContainerURL: url, trackInfo: nil)
	}

	@objc(urlsForContainerURL:trackInfo:)
	class func urls(forContainerURL url: URL, trackInfo: AutoreleasingUnsafeMutablePointer<NSDictionary?>?) -> [Any] {
		if !url.isFileURL {
			return []
		}

		if url.fragment != nil {
			// input url already has fragment defined - no need to expand further
			return [url]
		}

		var tracks: [URL] = []

		var embedded = false
		var cuesheet: CueSheet?
		var fileMetadata: [AnyHashable: Any]?
		var alsoMetadata: [AnyHashable: Any]?

		let ext = url.pathExtension
		if ext.caseInsensitiveCompare("cue") != .orderedSame {
			// Embedded cuesheet check
			fileMetadata = AudioMetadataReader.metadata(for: url, skipCue: true)

			alsoMetadata = AudioPropertiesReader.properties(for: url, skipCue: true)

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
			// A file with no sheet reads with this plugin as without it, as just
			// read: the file itself, then, with no need to read it again.
			if let trackInfo, let alsoMetadata {
				trackInfo.pointee = [url: merging(alsoMetadata, with: fileMetadata ?? [:])] as NSDictionary
			}
			return tracks
		}

		for track in cuesheet.tracks {
			if let trackURL = URL(string: url.absoluteString + "#\(track.track)") {
				tracks.append(trackURL)
			}
		}

		if let trackInfo {
			// An embedded sheet's file was just read, with and without its
			// decoder; a .cue file's are read as they come up.
			var fileProperties: [URL: Any] = [:]
			var fileTags: [URL: Any] = [:]
			if embedded {
				fileProperties[url] = alsoMetadata ?? NSNull()
				fileTags[url] = fileMetadata ?? NSNull()
			}
			let info = trackInfoDictionary(forURLs: tracks, ofCueSheet: cuesheet, url: url, embedded: embedded, fileProperties: &fileProperties, fileMetadata: &fileTags)
			trackInfo.pointee = info as NSDictionary?
		}

		return tracks
	}

	// Every track's properties and tags, as the decoder and the metadata reader
	// would give them, from the sheet just parsed and the properties and tags of
	// each audio file it plays, read once. The caches hold those, by file URL.
	private class func trackInfoDictionary(forURLs urls: [URL], ofCueSheet cuesheet: CueSheet, url: URL, embedded: Bool, fileProperties: inout [URL: Any], fileMetadata: inout [URL: Any]) -> [URL: Any]? {
		// The decoder opens the file it plays itself; a file whose properties
		// come from a properties reader instead is read the usual way. So is a
		// track whose properties would come from one.
		let propertiesReaders = PluginController.shared()?.propertiesReadersByExtension() ?? [:]
		if propertiesReaders[url.pathExtension.lowercased()] != nil {
			return nil
		}

		let cueTracks = cuesheet.tracks
		var info: [URL: Any] = [:]
		for trackURL in urls {
			guard let fragment = trackURL.fragment, !fragment.isEmpty, info[trackURL] == nil else {
				continue
			}

			// The first track numbered so, as the decoder and metadata reader find it
			guard let index = cueTracks.firstIndex(where: { $0.track == fragment }) else {
				continue
			}
			let track = cueTracks[index]

			guard let fileURL = embedded ? url : track.url else {
				continue
			}
			if !fileURL.isFileURL || propertiesReaders[fileURL.pathExtension.lowercased()] != nil {
				continue
			}

			if fileProperties[fileURL] == nil {
				fileProperties[fileURL] = AudioPropertiesReader.properties(for: fileURL, skipCue: true) ?? NSNull()
				fileMetadata[fileURL] = AudioMetadataReader.metadata(for: fileURL, skipCue: true) ?? NSNull()
			}
			guard let properties = fileProperties[fileURL] as? [AnyHashable: Any] else {
				continue
			}
			var tags = fileMetadata[fileURL] as? [AnyHashable: Any]
			if fileMetadata[fileURL] is NSNull {
				tags = nil
			}

			let range = CueSheetDecoder.trackRange(at: index, ofTracks: cueTracks, embedded: embedded, fileProperties: properties)
			// The decoder's properties merge with its tags, which would fill a
			// length of 0 if they had one; the merged file properties can't say.
			if range.end - range.start == 0 {
				continue
			}

			let metadata = CueSheetMetadataReader.metadata(forTrack: track, fileMetadata: tags)
			// Where the tags have no art, the cover beside the sheet would
			// replace the decoder's; that is added later, only where none is.
			if metadata["albumArt"] == nil && properties["albumArt"] != nil {
				continue
			}

			// The track's tags replace the properties' unconditionally, as the
			// playlist merges a cue sheet track's.
			var entryInfo = CueSheetDecoder.properties(withFileProperties: properties, start: range.start, end: range.end)
			for (key, value) in metadata {
				entryInfo[key] = value
			}
			info[trackURL] = entryInfo
		}

		return info
	}

	// As the app's NSDictionary+Merge category, which Objective-C plugins
	// share through the runtime: dict1 wins, except where its value is empty,
	// and dictionaries merge recursively.
	private class func merging(_ dict1: [AnyHashable: Any], with dict2: [AnyHashable: Any]) -> [AnyHashable: Any] {
		var result = dict1
		for (key, obj) in dict2 {
			guard let objTarget = dict1[key] else {
				result[key] = obj
				continue
			}
			if let sub2 = obj as? [AnyHashable: Any], let sub1 = objTarget as? [AnyHashable: Any] {
				result[key] = merging(sub1, with: sub2)
				continue
			}
			var isEmpty = false
			if let val = objTarget as? String {
				isEmpty = val.isEmpty
			} else if let val = objTarget as? NSNumber {
				isEmpty = val.isEqual(to: 0)
			} else if let val = objTarget as? Data {
				isEmpty = val.isEmpty
			}
			if isEmpty {
				result[key] = obj
			}
		}
		return result
	}

	@objc(dependencyUrlsForContainerURL:)
	class func dependencyUrls(forContainerURL url: URL) -> [Any] {
		var url = url
		if !url.isFileURL {
			return []
		}

		if url.fragment != nil {
			let pathString = url.path
			let lastComponent = url.lastPathComponent

			// Find that last component in the string from the end to make sure
			// to get the last one
			if let fragmentRange = pathString.range(of: lastComponent, options: .backwards) {
				// Chop the fragment.
				let newPathString = String(pathString[..<fragmentRange.upperBound])

				url = URL(fileURLWithPath: newPathString)
			}
		}

		var tracks: [URL] = []

		let ext = url.pathExtension
		if ext.caseInsensitiveCompare("cue") != .orderedSame {
			return []
		}
		let cuesheet = CueSheet(file: url.path)

		for track in cuesheet.tracks {
			if let trackURL = track.url, !tracks.contains(trackURL) {
				tracks.append(trackURL)
			}
		}

		return tracks
	}
}