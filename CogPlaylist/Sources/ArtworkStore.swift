//
//  ArtworkStore.swift
//  CogPlaylist
//
//  Album art, kept once per picture: an AlbumArtwork named by the SHA-256 of
//  its data (lowercase hex, as SHA256Digest writes it), which entries name in
//  artHash. The store keeps every picture by hash in memory, as the macOS
//  app's kArtworkDictionary did, so a playlist's rows find theirs without a
//  fetch each.
//

import CoreData
import CryptoKit
import Foundation

@objc public final class ArtworkStore: NSObject {
	/// The store entries keep their art in.
	@objc public static var shared: ArtworkStore?

	@objc public let context: NSManagedObjectContext

	/// Every picture, by hash.
	@objc public private(set) var artworks: [String: AlbumArtwork] = [:]

	@objc public init(context: NSManagedObjectContext) {
		self.context = context
		super.init()
	}

	/// Takes in every picture the store has, as the playlist loads.
	@objc public func loadAll() throws {
		let request = NSFetchRequest<AlbumArtwork>(entityName: "AlbumArtwork")
		var loaded: [String: AlbumArtwork] = [:]
		for artwork in try context.fetch(request) {
			if let hash = artwork.artHash { loaded[hash] = artwork }
		}
		artworks = loaded
	}

	@objc(artworkForHash:) public func artwork(forHash hash: String) -> AlbumArtwork? {
		artworks[hash]
	}

	/// Stores `data` as a picture, unless the same one is already stored,
	/// and returns its hash.
	@objc(storeArtData:) public func store(_ data: Data) -> String {
		let hash = Self.hash(of: data)
		if artworks[hash] == nil {
			let artwork = AlbumArtwork(context: context)
			artwork.artHash = hash
			artwork.artData = data
			artworks[hash] = artwork
		}
		return hash
	}

	static func hash(of data: Data) -> String {
		SHA256.hash(data: data).map { String(format: "%02x", $0) }.joined()
	}
}

extension PlaylistEntry {
	@objc public class var keyPathsForValuesAffectingAlbumArtInternal: Set<String> { ["artHash"] }

	/// The entry's art as stored, in whatever format the plugins found it
	/// (JPEG, PNG, AVIF...); setting stores it once, by its hash. Empty data
	/// changes nothing.
	@objc public var albumArtInternal: Data? {
		get { artHash.flatMap { ArtworkStore.shared?.artwork(forHash: $0)?.artData } }
		set {
			guard let newValue, !newValue.isEmpty, let store = ArtworkStore.shared else { return }
			artHash = store.store(newValue)
		}
	}
}
