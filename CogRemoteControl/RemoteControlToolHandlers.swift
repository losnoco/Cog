//
//  RemoteControlToolHandlers.swift
//  CogRemoteControl
//
//  Created by Kevin López Brante on 2026-07-01.
//

import Foundation
import MCP

/// Builds MCP `Server` instances exposing Cog's remote-control tools,
/// dispatching every call through a `CogRemoteControlTarget`.
@available(macOS 13.0, *)
enum RemoteControlTools {
	/// The host app's version (Bundle.main is Cog.app at runtime).
	private static let hostAppVersion =
		Bundle.main.infoDictionary?["CFBundleShortVersionString"] as? String
			?? Bundle.main.infoDictionary?["CFBundleVersion"] as? String
			?? "unknown"

	static func makeServer(target: any CogRemoteControlTarget) async -> Server {
		let server = Server(
			name: "Cog",
			version: hostAppVersion,
			instructions: "Controls the Cog audio player running on this Mac: playback transport, volume, seeking, shuffle/repeat, and playlist management.",
			capabilities: Server.Capabilities(tools: .init(listChanged: false))
		)

		await server.withMethodHandler(ListTools.self) { _ in
			.init(tools: allTools)
		}

		await server.withMethodHandler(CallTool.self) { params in
			try await call(name: params.name, arguments: params.arguments ?? [:], target: target)
		}

		return server
	}

	// MARK: - Tool definitions

	private static let emptySchema: Value = .object(["type": "object", "properties": [:]])

	private static let allTools: [Tool] = [
		Tool(name: "play", description: "Start playback (resumes if paused; otherwise starts the selected or first track).", inputSchema: emptySchema),
		Tool(name: "pause", description: "Toggle pause: pauses if playing, resumes if paused.", inputSchema: emptySchema),
		Tool(name: "resume", description: "Resume playback if paused.", inputSchema: emptySchema),
		Tool(name: "stop", description: "Stop playback.", inputSchema: emptySchema),
		Tool(name: "next", description: "Skip to the next track.", inputSchema: emptySchema),
		Tool(name: "previous", description: "Go back to the previous track.", inputSchema: emptySchema),
		Tool(name: "now_playing", description: "Get the current track and playback state as JSON (title, artist, album, position, length, status, seekable).", inputSchema: emptySchema),
		Tool(name: "get_volume", description: "Get the current playback volume as a percentage (0-100).", inputSchema: emptySchema),
		Tool(name: "set_volume", description: "Set the playback volume.", inputSchema: .object([
			"type": "object",
			"properties": ["volume": ["type": "number", "description": "Volume percentage, 0-100."]],
			"required": ["volume"],
		])),
		Tool(name: "get_position", description: "Get the playback position in seconds within the current track.", inputSchema: emptySchema),
		Tool(name: "seek", description: "Seek to a position in the current track. Fails if the track is not seekable.", inputSchema: .object([
			"type": "object",
			"properties": ["position": ["type": "number", "description": "Position in seconds from the start of the track."]],
			"required": ["position"],
		])),
		Tool(name: "get_shuffle", description: "Get the current shuffle mode (off, albums, or all).", inputSchema: emptySchema),
		Tool(name: "set_shuffle", description: "Set the shuffle mode.", inputSchema: .object([
			"type": "object",
			"properties": ["mode": ["type": "string", "enum": ["off", "albums", "all"], "description": "Shuffle mode."]],
			"required": ["mode"],
		])),
		Tool(name: "get_repeat", description: "Get the current repeat mode (none, one, album, or all).", inputSchema: emptySchema),
		Tool(name: "set_repeat", description: "Set the repeat mode.", inputSchema: .object([
			"type": "object",
			"properties": ["mode": ["type": "string", "enum": ["none", "one", "album", "all"], "description": "Repeat mode."]],
			"required": ["mode"],
		])),
		Tool(name: "list_playlist", description: "List playlist entries as JSON, with their index, metadata, queued flag, and whether each is the current track.", inputSchema: .object([
			"type": "object",
			"properties": [
				"offset": ["type": "integer", "description": "Index of the first entry to return (default 0)."],
				"limit": ["type": "integer", "description": "Maximum number of entries to return (default: all)."],
			],
		])),
		Tool(name: "add_to_playlist", description: "Add tracks to the playlist by file path or URL. Note: Cog is sandboxed, so file paths must be within folders Cog has been granted access to; other paths are added but will fail to play.", inputSchema: .object([
			"type": "object",
			"properties": ["urls": ["type": "array", "items": ["type": "string"], "description": "Absolute file paths or URLs of tracks or folders to add."]],
			"required": ["urls"],
		])),
		Tool(name: "remove_from_playlist", description: "Remove the playlist entry at the given index.", inputSchema: .object([
			"type": "object",
			"properties": ["index": ["type": "integer", "description": "Zero-based playlist index."]],
			"required": ["index"],
		])),
		Tool(name: "play_playlist_entry", description: "Start playing the playlist entry at the given index.", inputSchema: .object([
			"type": "object",
			"properties": ["index": ["type": "integer", "description": "Zero-based playlist index."]],
			"required": ["index"],
		])),
		Tool(name: "toggle_queue", description: "Toggle whether the playlist entry at the given index is in the play queue.", inputSchema: .object([
			"type": "object",
			"properties": ["index": ["type": "integer", "description": "Zero-based playlist index."]],
			"required": ["index"],
		])),
	]

	// MARK: - Dispatch

	private static func call(name: String, arguments: [String: Value], target: any CogRemoteControlTarget) async throws -> CallTool.Result {
		// Run the whole dispatch on the main actor: the target's ObjC-bridged
		// results aren't Sendable, so nothing may cross isolation mid-call.
		try await MainActor.run {
			try callOnMain(name: name, arguments: arguments, target: target)
		}
	}

	@MainActor private static func callOnMain(name: String, arguments: [String: Value], target: any CogRemoteControlTarget) throws -> CallTool.Result {
		switch name {
		case "play":
			target.remotePlay()
			return text("Playback started.")
		case "pause":
			target.remotePause()
			return text("Toggled pause.")
		case "resume":
			target.remoteResume()
			return text("Playback resumed.")
		case "stop":
			target.remoteStop()
			return text("Playback stopped.")
		case "next":
			target.remoteNext()
			return text("Skipped to next track.")
		case "previous":
			target.remotePrevious()
			return text("Went back to previous track.")

		case "now_playing":
			if let info = target.remoteNowPlaying() {
				return try json(info)
			}
			return try json(["status": "stopped"])

		case "get_volume":
			let volume = target.remoteVolume()
			return text("Volume: \(format(volume))%")
		case "set_volume":
			guard let volume = number(arguments["volume"]) else {
				throw MCPError.invalidParams("volume (number, 0-100) is required")
			}
			let clamped = min(max(volume, 0.0), 100.0)
			target.remoteSetVolume(clamped)
			return text("Volume set to \(format(clamped))%")

		case "get_position":
			guard let position = target.remotePosition() else {
				return errorText("No track is playing.")
			}
			return text("Position: \(format(position.doubleValue)) seconds")
		case "seek":
			guard target.remotePosition() != nil else {
				return errorText("No track is playing.")
			}
			guard target.remoteIsSeekable() else {
				return errorText("The current track is not seekable.")
			}
			guard let position = number(arguments["position"]), position >= 0 else {
				throw MCPError.invalidParams("position (number of seconds, >= 0) is required")
			}
			target.remoteSeek(to: position)
			return text("Seeked to \(format(position)) seconds.")

		case "get_shuffle":
			return text("Shuffle mode: \(target.remoteShuffleMode())")
		case "set_shuffle":
			guard let mode = arguments["mode"]?.stringValue, ["off", "albums", "all"].contains(mode) else {
				throw MCPError.invalidParams("mode must be one of: off, albums, all")
			}
			target.remoteSetShuffleMode(mode)
			return text("Shuffle mode set to \(mode).")

		case "get_repeat":
			return text("Repeat mode: \(target.remoteRepeatMode())")
		case "set_repeat":
			guard let mode = arguments["mode"]?.stringValue, ["none", "one", "album", "all"].contains(mode) else {
				throw MCPError.invalidParams("mode must be one of: none, one, album, all")
			}
			target.remoteSetRepeatMode(mode)
			return text("Repeat mode set to \(mode).")

		case "list_playlist":
			let entries = target.remoteListPlaylist()
			let offset = arguments["offset"]?.intValue ?? 0
			let limit = arguments["limit"]?.intValue ?? entries.count
			guard offset >= 0, limit >= 0 else {
				throw MCPError.invalidParams("offset and limit must be non-negative")
			}
			let page = Array(entries.dropFirst(offset).prefix(limit))
			return try json(["total": entries.count, "offset": offset, "entries": page])

		case "add_to_playlist":
			guard let values = arguments["urls"]?.arrayValue else {
				throw MCPError.invalidParams("urls (array of strings) is required")
			}
			let urls = values.compactMap(\.stringValue)
			guard !urls.isEmpty, urls.count == values.count else {
				throw MCPError.invalidParams("urls must be a non-empty array of strings")
			}
			let count = target.remoteAdd(toPlaylist: urls)
			return text("Added \(count) entr\(count == 1 ? "y" : "ies") to the playlist.")

		case "remove_from_playlist":
			guard let index = arguments["index"]?.intValue else {
				throw MCPError.invalidParams("index (integer) is required")
			}
			if target.remoteRemoveFromPlaylist(at: index) {
				return text("Removed playlist entry \(index).")
			}
			return errorText("No playlist entry at index \(index).")

		case "play_playlist_entry":
			guard let index = arguments["index"]?.intValue else {
				throw MCPError.invalidParams("index (integer) is required")
			}
			if target.remotePlayPlaylistEntry(at: index) {
				return text("Playing playlist entry \(index).")
			}
			return errorText("No playlist entry at index \(index).")

		case "toggle_queue":
			guard let index = arguments["index"]?.intValue else {
				throw MCPError.invalidParams("index (integer) is required")
			}
			if let queued = target.remoteToggleQueue(at: index) {
				return text("Playlist entry \(index) is \(queued.boolValue ? "now" : "no longer") queued.")
			}
			return errorText("No playlist entry at index \(index).")

		default:
			throw MCPError.methodNotFound("Unknown tool: \(name)")
		}
	}

	// MARK: - Helpers

	private static func number(_ value: Value?) -> Double? {
		guard let value else { return nil }
		return value.doubleValue ?? value.intValue.map(Double.init)
	}

	private static func format(_ value: Double) -> String {
		String(format: "%.1f", value)
	}

	private static func text(_ message: String) -> CallTool.Result {
		.init(content: [.text(text: message, annotations: nil, _meta: nil)], isError: false)
	}

	private static func errorText(_ message: String) -> CallTool.Result {
		.init(content: [.text(text: message, annotations: nil, _meta: nil)], isError: true)
	}

	private static func json(_ object: Any) throws -> CallTool.Result {
		let data = try JSONSerialization.data(
			withJSONObject: object,
			options: [.prettyPrinted, .sortedKeys, .withoutEscapingSlashes]
		)
		let string = String(data: data, encoding: .utf8) ?? "{}"
		return .init(content: [.text(text: string, annotations: nil, _meta: nil)], isError: false)
	}
}
