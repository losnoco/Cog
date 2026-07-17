//
//  RemoteControlServerBridge.swift
//  CogRemoteControl
//
//  Created by Kevin López Brante on 2026-07-01.
//
//  The app finds this class by name at runtime (it can't link the framework:
//  the macOS 13 floor makes dyld loading fatal on the app's older systems)
//  and calls it through the CogRemoteControlServerBridging protocol in
//  CogRemoteControlTarget.h. Keep the class name and selectors stable.
//

import Foundation

@available(macOS 13.0, *)
@objc(CogRemoteControlServerBridge)
public final class RemoteControlServerBridge: NSObject, CogRemoteControlServerBridging {
	override private init() {}

	/// Starts the server on the given Unix domain socket path. The completion
	/// receives an error description, or nil on success.
	@objc(startWithSocketPath:target:completion:) public static func start(
		withSocketPath socketPath: String,
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
