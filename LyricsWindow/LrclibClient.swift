//
//  LrclibClient.swift
//  Cog
//

import Foundation

/// What we ask LRCLIB: the tags of one track, as they are in the file.
///
/// The server does its own normalisation (case, punctuation) and matches the
/// duration within about two seconds, so nothing is cleaned up here.
struct LrclibQuery: Hashable, Sendable {
	var title: String
	var artist: String
	var album: String
	/// Seconds; 0 when unknown.
	var duration: Double

	/// The cache key: two copies of the same song share one answer.
	var key: String {
		"\(artist)\n\(title)\n\(album)\n\(Int(duration.rounded()))"
	}
}

/// A definite answer from the server. "Not found" is an answer, not a failure.
enum LrclibAnswer: Sendable, Equatable {
	case found(plain: String, synced: String)
	case instrumental(synced: String)
	case notFound
}

enum LrclibError: Error, Sendable {
	/// Artist or title missing; nothing was sent.
	case incomplete
	/// The request never got an HTTP answer.
	case transport(String)
	/// Rate limited or a server error; worth asking again later.
	case transient(status: Int)
	/// Any other HTTP error.
	case api(status: Int, message: String)
	/// A 200 whose body is not a lyrics record.
	case malformed
}

/// Talks to the LRCLIB `/api/get` endpoint.
///
/// Only the exact-match endpoint is used, never `/api/search`: a guessed
/// match shown as the song's lyrics is worse than an honest blank.
struct LrclibClient: Sendable {
	static let defaultRoot = "https://lrclib.net"

	var root: String
	var session: URLSession

	init(root: String = LrclibClient.defaultRoot, session: URLSession = LrclibClient.makeSession()) {
		self.root = root
		self.session = session
	}

	static func makeSession() -> URLSession {
		let configuration = URLSessionConfiguration.default
		configuration.timeoutIntervalForRequest = 20
		configuration.timeoutIntervalForResource = 20
		configuration.httpAdditionalHeaders = ["User-Agent": clientName]
		return URLSession(configuration: configuration)
	}

	/// Identifies us to the server, as LRCLIB asks clients to do.
	static let clientName: String = {
		let version = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as? String ?? "unknown"
		return "Cog/\(version) (+https://github.com/losnoco/Cog)"
	}()

	/// `root` with trailing slashes removed and `/api` appended unless it is
	/// already there.
	func endpoint(_ name: String) -> URL? {
		var base = root
		while base.hasSuffix("/") {
			base.removeLast()
		}
		if !base.hasSuffix("/api") {
			base += "/api"
		}
		return URL(string: "\(base)/\(name)")
	}

	func request(for query: LrclibQuery) -> URLRequest? {
		guard let url = endpoint("get"),
		      var components = URLComponents(url: url, resolvingAgainstBaseURL: false) else {
			return nil
		}
		var items = [
			URLQueryItem(name: "track_name", value: query.title),
			URLQueryItem(name: "artist_name", value: query.artist)
		]
		if !query.album.isEmpty {
			items.append(URLQueryItem(name: "album_name", value: query.album))
		}
		// The server rejects durations outside this range.
		let seconds = Int(query.duration.rounded())
		if (1...3600).contains(seconds) {
			items.append(URLQueryItem(name: "duration", value: String(seconds)))
		}
		components.queryItems = items
		// URLComponents leaves "+" alone, which servers read as a space.
		components.percentEncodedQuery = components.percentEncodedQuery?
			.replacingOccurrences(of: "+", with: "%2B")
		guard let finalURL = components.url else { return nil }
		var request = URLRequest(url: finalURL)
		request.setValue(Self.clientName, forHTTPHeaderField: "Lrclib-Client")
		request.setValue("application/json", forHTTPHeaderField: "Accept")
		return request
	}

	func get(_ query: LrclibQuery) async throws -> LrclibAnswer {
		guard !query.title.isEmpty, !query.artist.isEmpty,
		      let request = request(for: query) else {
			throw LrclibError.incomplete
		}

		let data: Data
		let response: URLResponse
		do {
			(data, response) = try await session.data(for: request)
		} catch {
			throw LrclibError.transport(error.localizedDescription)
		}
		guard let http = response as? HTTPURLResponse else {
			throw LrclibError.transport("No HTTP response")
		}

		switch http.statusCode {
		case 200:
			return try Self.answer(from: data)
		case 404:
			return .notFound
		case 429, 500...599:
			throw LrclibError.transient(status: http.statusCode)
		default:
			let message = (try? JSONDecoder().decode(ErrorBody.self, from: data))?.message
			throw LrclibError.api(status: http.statusCode, message: message ?? "HTTP \(http.statusCode)")
		}
	}

	static func answer(from data: Data) throws -> LrclibAnswer {
		guard let record = try? JSONDecoder().decode(Record.self, from: data) else {
			throw LrclibError.malformed
		}
		let plain = record.plainLyrics ?? ""
		let synced = record.syncedLyrics ?? ""
		if record.instrumental == true {
			return .instrumental(synced: synced)
		}
		if plain.isEmpty && synced.isEmpty {
			return .notFound
		}
		return .found(plain: plain, synced: synced)
	}

	private struct Record: Decodable {
		// Required: a 200 without an id is not a lyrics record.
		var id: Int
		var plainLyrics: String?
		var syncedLyrics: String?
		var instrumental: Bool?
	}

	private struct ErrorBody: Decodable {
		var message: String?
	}
}
