//
//  SharedWithMac.swift
//  Cog (iOS)
//
//  What the macOS sources the app compiles as they are (lyrics, scrobbling)
//  expect of the app around them.
//

import CogAudio
import CogPlaylist
import CoreData
import CryptoKit
import Foundation

/// Where LyricsLookup and ListenBrainzScrobbler find the store, as macOS's
/// PlaylistController.sharedPersistentContainer() gives it.
enum PlaylistController {
	@MainActor static var container: NSPersistentContainer!

	@MainActor static func sharedPersistentContainer() -> NSPersistentContainer {
		container
	}
}

extension PlaylistEntry {
	/// The entry as the scrobblers take it, as PlaylistEntry.m makes it.
	var audioScrobblerTrack: AudioScrobblerTrack {
		let track = AudioScrobblerTrack(title: title, artist: artist, albumArtist: albumartist, album: album,
		                                trackNumber: Int(self.track), length: length)
		// TagLib, FLAC, Vorbis and Opus lowercase the Picard names; FFmpeg
		// keeps the MP4/ID3 spelling.
		track.recordingMBID = readAllValuesAsString("musicbrainz_trackid") ?? readAllValuesAsString("musicbrainz track id")
		track.releaseMBID = readAllValuesAsString("musicbrainz_albumid") ?? readAllValuesAsString("musicbrainz album id")
		let artistIDs = readAllValuesAsString("musicbrainz_artistid") ?? readAllValuesAsString("musicbrainz artist id") ?? ""
		track.artistMBIDs = artistIDs.components(separatedBy: CharacterSet(charactersIn: ",/;"))
			.map { $0.trimmingCharacters(in: .whitespaces) }
			.filter { !$0.isEmpty }
		return track
	}
}

extension NSData {
	/// Lowercase hex MD5, as Utils/NSData+MD5.m gives it to LastFMAPI (which
	/// signs requests with it, as Last.fm's API requires).
	@objc(MD5) func md5() -> String {
		Insecure.MD5.hash(data: self as Data).map { String(format: "%02x", $0) }.joined()
	}
}
