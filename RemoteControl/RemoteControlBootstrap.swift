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
/// on the `remoteControlEnabled` default. Talks to the CogRemoteControl
/// framework only through its ObjC bridge, loaded at runtime.
@available(macOS 13.0, *)
@MainActor final class RemoteControlBootstrap {
	static let shared = RemoteControlBootstrap()

	/// The bridge class from the embedded CogRemoteControl framework, loaded
	/// on first use. The framework requires macOS 13 while the app runs back
	/// to 10.15, so it must not be linked (dyld loads even weak-linked
	/// frameworks eagerly, and its Swift 5.7 runtime dylibs are missing
	/// before 13, which would abort every launch); a failure to load it here
	/// just surfaces as a Preferences-pane error.
	private static let bridgeClass: CogRemoteControlServerBridging.Type? = {
		guard let frameworkURL = Bundle.main.privateFrameworksURL?
			.appendingPathComponent("CogRemoteControl.framework"),
			let framework = Bundle(url: frameworkURL),
			framework.load()
		else { return nil }
		return NSClassFromString("CogRemoteControlServerBridge") as? CogRemoteControlServerBridging.Type
	}()

	/// The bridge class only if the framework is already loaded, so stop
	/// paths never load the framework just to tear down a server that was
	/// never started.
	private static var bridgeClassIfLoaded: CogRemoteControlServerBridging.Type? {
		NSClassFromString("CogRemoteControlServerBridge") as? CogRemoteControlServerBridging.Type
	}

	static let enabledKey = "remoteControlEnabled"

	/// The Unix domain socket the server listens on, shared with the bundled
	/// cog-mcp helper via the app-group container (both declare the group in
	/// their entitlements, so neither trips container-protection prompts).
	private static let appGroupIdentifier = "group.org.cogx.cog"
	private static let socketName = "ipc.sock"

	let statusModel = RemoteControlStatusModel()
	private let adapter = CogRemoteControlAdapter()

	private init() {
		UserDefaults.standard.register(defaults: [
			Self.enabledKey: false,
		])
	}

	private var socketPath: String? {
		FileManager.default
			.containerURL(forSecurityApplicationGroupIdentifier: Self.appGroupIdentifier)?
			.appendingPathComponent(Self.socketName).path
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

	/// The command MCP clients should launch to reach the running server:
	/// the bundled stdio bridge.
	private func clientCommand() -> String {
		Bundle.main.bundleURL
			.appendingPathComponent("Contents/Helpers/cog-mcp").path
	}

	private func start() {
		guard let socketPath else {
			statusModel.isRunning = false
			statusModel.clientCommand = nil
			statusModel.lastError = NSLocalizedString("The shared app-group folder is unavailable.", comment: "Remote Control error")
			return
		}
		guard let bridge = Self.bridgeClass else {
			statusModel.isRunning = false
			statusModel.clientCommand = nil
			statusModel.lastError = NSLocalizedString("The Remote Control component could not be loaded.", comment: "Remote Control error")
			return
		}
		bridge.start(withSocketPath: socketPath, target: adapter) { [weak self] errorDescription in
			Task { @MainActor in
				guard let self else { return }
				let statusModel = self.statusModel
				statusModel.isRunning = errorDescription == nil
				statusModel.clientCommand = errorDescription == nil ? self.clientCommand() : nil
				statusModel.lastError = errorDescription
			}
		}
	}

	private func stop() {
		Self.bridgeClassIfLoaded?.stop(completion: nil)
		statusModel.isRunning = false
		statusModel.clientCommand = nil
		statusModel.lastError = nil
	}

	/// Best-effort shutdown for app termination.
	func shutdown() {
		Self.bridgeClassIfLoaded?.stop(completion: nil)
	}
}
