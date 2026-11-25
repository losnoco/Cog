//
//  SpotlightPlaylistEntry.swift
//  Cog
//
//  Created by Matthew Grinshpun on 11/02/08.
//  Copyright 2008 Matthew Leon Grinshpun. All rights reserved.
//

import CogPlaylist
import CoreData
import CoreServices
import Foundation

@objc(SpotlightPlaylistEntry)
class SpotlightPlaylistEntry: NSObject {
	// Translates an mdKey to an entryKey; an array means the value needs the
	// named transformer first: (entryKey, transformerName).
	private static let importKeys: [String: any Sendable] = [
		"kMDItemTitle": "title",
		"kMDItemAlbum": "album",
		"kMDItemAudioTrackNumber": "track",
		"kMDItemRecordingYear": "date",
		"kMDItemMusicalGenre": "genre",
		"kMDItemDurationSeconds": "length",
		"kMDItemPath": ["url", "PathToURLTransformer"],
		"kMDItemAuthors": ["artist", "AuthorToArtistTransformer"],
	]

	@objc(playlistEntryWithMetadataItem:)
	class func playlistEntry(with metadataItem: NSMetadataItem) -> PlaylistEntry? {
		var entry: PlaylistEntry?
		kPersistentContainer.viewContext.performAndWait {
			let newEntry = NSEntityDescription.insertNewObject(forEntityName: "PlaylistEntry", into: kPersistentContainer.viewContext) as! PlaylistEntry

			newEntry.deLeted = true

			var dict: [String: Any] = [:]

			// loop through the keys we want to extract
			for (mdKey, importTarget) in importKeys {
				guard let value = metadataItem.value(forAttribute: mdKey) else { continue }
				// Just copy the object from metadata
				if let key = importTarget as? String {
					if key == "length" {
						// fake it
						let number = value as? NSNumber
						dict["samplerate"] = 44100.0
						dict["totalFrames"] = 44100.0 * (number?.doubleValue ?? 0)
					} else {
						dict[key] = value
					}
				}
				// Transform the value in metadata before copying it in
				else if let transform = importTarget as? [String],
				        let transformer = ValueTransformer(forName: NSValueTransformerName(transform[1])) {
					dict[transform[0]] = transformer.transformedValue(value)
				}
				// The importKeys dictionary contains something strange...
				else {
					assertionFailure("ERROR: Could not import key \(mdKey)")
				}
			}

			let url = dict["url"] as? URL
			dict.removeValue(forKey: "url")

			newEntry.url = url

			newEntry.setMetadata(dict)

			entry = newEntry
		}

		return entry
	}
}