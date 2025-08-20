//
//  AudioScrobbler.swift
//  Cog
//
//  Created by Leonardo Lobato on 08/08/25.
//

import Foundation
import Cocoa

@objc
public class AudioScrobblerTrack: NSObject {
    public let title: String
    public let artist: String?
    public let albumArtist: String?
    public let album: String?
    public let trackNumber: Int
    public let length: TimeInterval
    public var position: TimeInterval

    @objc
    public init(title: String, artist: String?, albumArtist: String?, album: String?, trackNumber: Int, length: TimeInterval, position: TimeInterval) {
        self.title = title
        self.artist = artist
        self.albumArtist = albumArtist
        self.album = album
        self.trackNumber = trackNumber
        self.length = length
        self.position = position
    }

    public override func isEqual(_ object: Any?) -> Bool {
        guard let rhs = object as? AudioScrobblerTrack else { return false }
        let lhs = self
        guard lhs.title == rhs.title else { return false }
        guard lhs.artist == rhs.artist else { return false }
        guard lhs.albumArtist == rhs.albumArtist else { return false }
        guard lhs.album == rhs.album else { return false }
        guard lhs.trackNumber == rhs.trackNumber else { return false }
        guard lhs.length == rhs.length else { return false }
        return true
    }

}

public class AudioScrobbler: NSObject {

    let lastFM: LastFMAPI

    @objc
    public static let shared: AudioScrobbler = AudioScrobbler()

    public var enabled: Bool {
        get {
            guard Secrets.lastFmApiSecret.count > 0 && Secrets.lastFmApiKey.count > 0 else {
                return false
            }
            guard lastFM.isAuthenticated else {
                return false
            }
            return UserDefaults.standard.bool(forKey: "enableAudioScrobbler")
        }
        set {
            UserDefaults.standard.set(newValue, forKey: "enableAudioScrobbler")
        }
    }

    override init() {
        lastFM = LastFMAPI(
            apiKey: Secrets.lastFmApiKey,
            apiSecret: Secrets.lastFmApiSecret
        )

        // TODO: move to settings screen
        lastFM.authenticate()
    }

    private func setNowPlaying(_ track: AudioScrobblerTrack) {
        guard enabled else {
            return
        }
        var params: [String: String] = [
            "track": track.title
        ]

        let artist: String? = track.artist
        if let artist {
            params["artist"] = artist
        }
        if let albumArtist = track.albumArtist, albumArtist != artist {
            params["albumArtist"] = albumArtist
        }
        if let album = track.album {
            params["album"] = album
        }
        if track.trackNumber > 0 {
            params["trackNumber"] = "\(track.trackNumber)"
        }
        if track.length > 0 {
            params["duration"] = "\(Int(track.length))"
        }
        guard params.count > 0 else {
            return
        }
        lastFM.request("track.updateNowPlaying", params: params)
    }

    // MARK: - Scrobbling

    struct NowPlaying: Equatable {
        let track: AudioScrobblerTrack
        let start: Date
        static func == (lhs: NowPlaying, rhs: NowPlaying) -> Bool {
            guard lhs.track.isEqual(rhs.track) else { return false }
            guard lhs.start == rhs.start else { return false }
            return true
        }
    }

    var nowPlaying: NowPlaying?
    var lastScrobble: NowPlaying?

    @objc
    public func updateNowPlaying(_ track: AudioScrobblerTrack, isPlaying: Bool) {
        print("AudioScrobbler: [isPlaying: \(isPlaying)]: [\(String(format: "%.2f", track.position))] [\(track.title)]")

        // Update currently playing track timestamp
        if nowPlaying?.track == track {
            nowPlaying?.track.position = track.position
        }

        // "It is often most convenient to send a scrobble request when a track has finished playing."
        let trackStopped = (isPlaying == false) || (self.nowPlaying?.track != track)
        if let nowPlaying = self.nowPlaying, trackStopped {
            scrobble(nowPlaying)
        }

        // Change currently playing track after the previous one has been scrobbled.
        if self.nowPlaying?.track != track {
            lastScrobble = nil
            nowPlaying = NowPlaying(track: track, start: Date())
            self.setNowPlaying(track)
        }
    }

    private func scrobble(_ nowPlaying: NowPlaying) {
        guard enabled else {
            return
        }

        // Guidelines:
        // https://www.last.fm/api/scrobbling
        // - The track must be longer than 30 seconds.
        // - And the track has been played for at least half its duration, or for 4 minutes (whichever occurs earlier.)
        guard nowPlaying.track.length >= 30.0 else {
            // Track is shorter than 30 seconds
            return
        }
        guard nowPlaying.track.position >= min(240, nowPlaying.track.length / 2.0) else {
            // the track has been played for at least half its duration, or for 4 minutes
            return
        }
        guard lastScrobble != nowPlaying else {
            // has already been scrobbled
            return
        }

        print("AudioScrobbler: will scrobble [\(nowPlaying.track.title)]")
        let track = nowPlaying.track
        var params: [String: String] = [
            "track": track.title,
            "timestamp": "\(Int(nowPlaying.start.timeIntervalSince1970))"
        ]

        let artist: String? = track.artist
        if let artist {
            params["artist"] = artist
        }
        if let albumArtist = track.albumArtist, albumArtist != artist {
            params["albumArtist"] = albumArtist
        }
        if let album = track.album {
            params["album"] = album
        }
        if track.trackNumber > 0 {
            params["trackNumber"] = "\(track.trackNumber)"
        }
        if track.length > 0 {
            params["duration"] = "\(Int(track.length))"
        }
        guard params.count > 0 else {
            return
        }
        lastScrobble = nowPlaying

        // TODO: setup a local cache queue to send scrobbles in
        // case the request fails
        lastFM.request("track.scrobble", params: params)
    }

}
