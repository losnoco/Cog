//
//  RemoteControlBootstrap.swift
//  Cog
//
//  Created by Kevin López Brante on 2026-07-01.
//

import Cocoa
import Combine

/// Observable facade for the Preferences pane, updated after start/stop.
@available(macOS 13.0, *)
@MainActor final class RemoteControlStatusModel: ObservableObject {
	@Published var isRunning = false
	@Published var lastError: String?
	/// The stdio command MCP clients should launch while the server runs.
	@Published var clientCommand: String?
}

/// Owns the adapter and status model, and starts/stops the MCP server based
/// on the `remoteControlEnabled` / `remoteControlPort` defaults. Talks to the
/// weak-linked CogRemoteControl framework only through its ObjC bridge.
@available(macOS 13.0, *)
@MainActor final class RemoteControlBootstrap {
	static let shared = RemoteControlBootstrap()

	static let enabledKey = "remoteControlEnabled"
	static let portKey = "remoteControlPort"

	let statusModel = RemoteControlStatusModel()
	private let adapter = CogRemoteControlAdapter()

	private init() {
		UserDefaults.standard.register(defaults: [
			Self.enabledKey: false,
			Self.portKey: 8089,
		])
	}

	var port: Int {
		let value = UserDefaults.standard.integer(forKey: Self.portKey)
		return (1...65535).contains(value) ? value : 8089
	}

	func autostartIfNeeded() {
		guard UserDefaults.standard.bool(forKey: Self.enabledKey) else { return }
		start()
	}

	/// Called by the Preferences pane when the toggle changes.
	func applyEnabledChange(_ enabled: Bool) {
		if enabled {
			start()
		} else {
			stop()
		}
	}

	/// Called by the Preferences pane when the port changes.
	func restartIfRunning() {
		guard statusModel.isRunning else { return }
		CogRemoteControlServerBridge.stop { [weak self] in
			Task { @MainActor in
				self?.start()
			}
		}
	}

	/// The command MCP clients should launch to reach the running server:
	/// the bundled stdio bridge, plus the port when it isn't the default.
	private func clientCommand(port: Int) -> String {
		let helper = Bundle.main.bundleURL
			.appendingPathComponent("Contents/Helpers/cog-mcp").path
		return port == 8089 ? helper : "\(helper) --port \(port)"
	}

	private func start() {
		let port = self.port
		CogRemoteControlServerBridge.start(withPort: port, target: adapter) { [weak self] errorDescription in
			Task { @MainActor in
				guard let self else { return }
				let statusModel = self.statusModel
				statusModel.isRunning = errorDescription == nil
				statusModel.clientCommand = errorDescription == nil ? self.clientCommand(port: port) : nil
				statusModel.lastError = errorDescription
			}
		}
	}

	private func stop() {
		CogRemoteControlServerBridge.stop(completion: nil)
		statusModel.isRunning = false
		statusModel.clientCommand = nil
		statusModel.lastError = nil
	}

	/// Best-effort shutdown for app termination.
	func shutdown() {
		CogRemoteControlServerBridge.stop(completion: nil)
	}
}
