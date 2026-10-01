//
//  ListenBrainzAPI.swift
//  Cog
//

import Foundation

/// One listen as ListenBrainz takes it. Optional fields left nil are left out
/// of the JSON.
struct ListenBrainzListen: Codable, Sendable {
    struct AdditionalInfo: Codable, Sendable {
        var durationMs: Int?
        var tracknumber: String?
        var recordingMbid: String?
        var releaseMbid: String?
        var artistMbids: [String]?
        var mediaPlayer = "Cog"
        var mediaPlayerVersion = ListenBrainzAPI.version
        var submissionClient = "Cog"
        var submissionClientVersion = ListenBrainzAPI.version
    }

    struct TrackMetadata: Codable, Sendable {
        var artistName: String
        var trackName: String
        var releaseName: String?
        var additionalInfo: AdditionalInfo
    }

    /// Unix seconds when the track started; absent for "playing now".
    var listenedAt: Int?
    var trackMetadata: TrackMetadata

    init?(track: AudioScrobblerTrack, listenedAt: Date?) {
        guard let artist = track.artist, !artist.isEmpty, !track.title.isEmpty else { return nil }
        var info = AdditionalInfo()
        if track.length > 0 {
            info.durationMs = Int((track.length * 1000).rounded())
        }
        if track.trackNumber > 0 {
            info.tracknumber = String(track.trackNumber)
        }
        info.recordingMbid = track.recordingMBID.flatMap { $0.isEmpty ? nil : $0 }
        info.releaseMbid = track.releaseMBID.flatMap { $0.isEmpty ? nil : $0 }
        info.artistMbids = track.artistMBIDs.isEmpty ? nil : track.artistMBIDs
        trackMetadata = TrackMetadata(artistName: artist,
                                      trackName: track.title,
                                      releaseName: track.album.flatMap { $0.isEmpty ? nil : $0 },
                                      additionalInfo: info)
        self.listenedAt = listenedAt.map { Int($0.timeIntervalSince1970) }
    }

    static let encoder: JSONEncoder = {
        let encoder = JSONEncoder()
        encoder.keyEncodingStrategy = .convertToSnakeCase
        return encoder
    }()

    static let decoder: JSONDecoder = {
        let decoder = JSONDecoder()
        decoder.keyDecodingStrategy = .convertFromSnakeCase
        return decoder
    }()
}

enum ListenBrainzError: Error, Sendable {
    /// The request never got an HTTP answer.
    case transport(String)
    /// The token is missing, wrong or revoked.
    case unauthorized
    /// Rate limited or a server error; worth sending again later.
    case transient(status: Int)
    /// The server refused what was sent; sending it again won't help.
    case rejected(status: Int, message: String)
    /// An answer that isn't what the API returns.
    case malformed

    var isRetryable: Bool {
        switch self {
        case .transport, .transient, .malformed: return true
        case .unauthorized, .rejected: return false
        }
    }
}

/// Talks to the ListenBrainz API, or a server speaking it (Maloja, Koito…).
struct ListenBrainzAPI: Sendable {
    static let defaultRoot = "https://api.listenbrainz.org"
    /// The server takes up to 1000 per request; a refused batch loses fewer
    /// plays when it is smaller.
    static let maxBatch = 50

    enum ListenType: String, Sendable {
        case single
        case importListens = "import"
        case playingNow = "playing_now"
    }

    static let version: String = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as? String ?? "unknown"
    static let userAgent = "Cog/\(version) (+https://github.com/losnoco/Cog)"

    var root: String
    var token: String
    var session: URLSession = .shared

    /// `root` with or without a trailing slash or `/1`.
    func endpoint(_ name: String) -> URL? {
        var base = root.trimmingCharacters(in: .whitespaces)
        if base.isEmpty {
            base = Self.defaultRoot
        }
        while base.hasSuffix("/") {
            base.removeLast()
        }
        if base.hasSuffix("/1") {
            base.removeLast(2)
        }
        return URL(string: "\(base)/1/\(name)")
    }

    private func request(_ name: String) -> URLRequest? {
        guard let url = endpoint(name) else { return nil }
        var request = URLRequest(url: url, timeoutInterval: 20)
        request.setValue("Token \(token)", forHTTPHeaderField: "Authorization")
        request.setValue(Self.userAgent, forHTTPHeaderField: "User-Agent")
        return request
    }

    private func send(_ request: URLRequest) async throws -> Data {
        let data: Data
        let response: URLResponse
        do {
            (data, response) = try await session.data(for: request)
        } catch {
            throw ListenBrainzError.transport(error.localizedDescription)
        }
        guard let http = response as? HTTPURLResponse else {
            throw ListenBrainzError.transport("No HTTP response")
        }
        switch http.statusCode {
        case 200..<300:
            return data
        case 401:
            throw ListenBrainzError.unauthorized
        case 429, 500...599:
            throw ListenBrainzError.transient(status: http.statusCode)
        default:
            let message = (try? JSONDecoder().decode(ErrorBody.self, from: data))?.error
            throw ListenBrainzError.rejected(status: http.statusCode, message: message ?? "HTTP \(http.statusCode)")
        }
    }

    /// The user name the token belongs to.
    func validateToken() async throws -> String {
        guard let request = request("validate-token") else { throw ListenBrainzError.malformed }
        let data = try await send(request)
        guard let body = try? JSONDecoder().decode(ValidateBody.self, from: data) else {
            throw ListenBrainzError.malformed
        }
        // A wrong token is a 200 that says so.
        guard body.valid == true, let name = body.user_name, !name.isEmpty else {
            throw ListenBrainzError.unauthorized
        }
        return name
    }

    func submit(_ listens: [ListenBrainzListen], as type: ListenType) async throws {
        guard var request = request("submit-listens") else { throw ListenBrainzError.malformed }
        request.httpMethod = "POST"
        request.setValue("application/json", forHTTPHeaderField: "Content-Type")
        request.httpBody = try ListenBrainzListen.encoder.encode(SubmitBody(listenType: type.rawValue, payload: listens))
        _ = try await send(request)
    }

    private struct SubmitBody: Encodable {
        var listenType: String
        var payload: [ListenBrainzListen]
    }

    private struct ValidateBody: Decodable {
        var valid: Bool?
        var user_name: String?
    }

    private struct ErrorBody: Decodable {
        var error: String?
    }
}
