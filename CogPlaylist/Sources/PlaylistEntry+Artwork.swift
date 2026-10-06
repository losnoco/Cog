//
//  PlaylistEntry+Artwork.swift
//  CogPlaylist
//
//  Album art, kept as macOS keeps it: each picture once, as an AlbumArtwork
//  named by the SHA-256 of its data (lowercase hex, as SHA256Digest writes
//  it), which entries name in `artHash`.
//

import CoreData
import CryptoKit
import Foundation

extension PlaylistEntry {
	/// Stores `data` as the entry's art, reusing a picture already stored.
	public func setAlbumArt(_ data: Data) {
		guard !data.isEmpty, let context = managedObjectContext else { return }
		let hash = SHA256.hash(data: data).map { String(format: "%02x", $0) }.joined()
		artHash = hash
		if AlbumArtwork.find(hash: hash, in: context) == nil {
			let artwork = AlbumArtwork(context: context)
			artwork.artHash = hash
			artwork.artData = data
		}
	}

	/// The entry's art as stored (any format the plugins found: JPEG, PNG,
	/// AVIF...); nil if it has none.
	public var albumArtData: Data? {
		guard let artHash, let context = managedObjectContext else { return nil }
		return AlbumArtwork.find(hash: artHash, in: context)?.artData
	}
}

extension AlbumArtwork {
	static func find(hash: String, in context: NSManagedObjectContext) -> AlbumArtwork? {
		let request = NSFetchRequest<AlbumArtwork>(entityName: "AlbumArtwork")
		request.predicate = NSPredicate(format: "artHash == %@", hash)
		request.fetchLimit = 1
		return (try? context.fetch(request))?.first
	}
}
