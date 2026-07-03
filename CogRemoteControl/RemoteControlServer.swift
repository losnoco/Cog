//
//  RemoteControlServer.swift
//  CogRemoteControl
//
//  Created by Kevin López Brante on 2026-07-01.
//

import Foundation

/// Public facade the host app uses to run the MCP server. Listens on a Unix
/// domain socket in the user-owned app-group container; that file being
/// reachable only by this user's processes, plus opt-in, is the security
/// boundary.
@available(macOS 13.0, *)
public actor RemoteControlServer {
	public static let shared = RemoteControlServer()

	private var app: RemoteControlSocketApp?

	private init() {}

	public var isRunning: Bool { app != nil }

	/// Starts the server on the given Unix domain socket path. Throws if the
	/// socket can't be bound. No-op if already running.
	public func start(socketPath: String, target: any CogRemoteControlTarget) async throws {
		guard app == nil else { return }

		let app = RemoteControlSocketApp(socketPath: socketPath) {
			await RemoteControlTools.makeServer(target: target)
		}
		try await app.start()
		self.app = app
	}

	public func stop() async {
		guard let app else { return }
		self.app = nil
		await app.stop()
	}
}
