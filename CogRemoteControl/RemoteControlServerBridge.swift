//
//  RemoteControlServerBridge.swift
//  CogRemoteControl
//
//  Created by Kevin López Brante on 2026-07-01.
//
//  The app calls this through the generated CogRemoteControl-Swift.h header
//  (the framework's Swift module can't be imported by the 10.15 app target).
//

import Foundation

@available(macOS 13.0, *)
@objc(CogRemoteControlServerBridge)
public final class RemoteControlServerBridge: NSObject {
	override private init() {}

	/// Starts the server on the given Unix domain socket path. The completion
	/// receives an error description, or nil on success.
	/// Selector pinned to match the app-side mirror declaration in
	/// RemoteControl/CogRemoteControlServerBridgeInterface.h.
	@objc(startWithSocketPath:target:completion:) public static func start(
		socketPath: String,
		target: any CogRemoteControlTarget,
		completion: @escaping @Sendable (_ errorDescription: String?) -> Void
	) {
		Task {
			do {
				try await RemoteControlServer.shared.start(socketPath: socketPath, target: target)
				completion(nil)
			} catch {
				completion(error.localizedDescription)
			}
		}
	}

	@objc(stopWithCompletion:) public static func stop(completion: (@Sendable () -> Void)?) {
		Task {
			await RemoteControlServer.shared.stop()
			completion?()
		}
	}
}
