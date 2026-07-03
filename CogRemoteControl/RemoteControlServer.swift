//
//  RemoteControlServer.swift
//  CogRemoteControl
//
//  Created by Kevin López Brante on 2026-07-01.
//

import Foundation

/// Public facade the host app uses to run the MCP server. Binds to
/// 127.0.0.1 only; being loopback-only plus opt-in is the security boundary.
@available(macOS 13.0, *)
public actor RemoteControlServer {
	public static let shared = RemoteControlServer()

	private var app: RemoteControlTCPApp?

	private init() {}

	public var isRunning: Bool { app != nil }

	/// Starts the server on 127.0.0.1:<port>. Throws if the port can't be
	/// bound (e.g. already in use). No-op if already running.
	public func start(port: Int, target: any CogRemoteControlTarget) async throws {
		guard app == nil else { return }

		let app = RemoteControlTCPApp(host: "127.0.0.1", port: port) {
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
