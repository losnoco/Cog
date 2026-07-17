//
//  RemoteControlObjCBootstrap.swift
//  Cog
//
//  Created by Kevin López Brante on 2026-07-01.
//
//  The one remote-control symbol callable unconditionally from Objective-C.
//  Everything else is reached only behind #available(macOS 13.0, *), so the
//  runtime-loaded CogRemoteControl framework is never touched on older systems.
//

import Foundation

@objc(CogRemoteControlBootstrap)
@MainActor final class RemoteControlObjCBootstrap: NSObject {
	@objc static func bootstrap() {
		if #available(macOS 13.0, *) {
			RemoteControlBootstrap.shared.autostartIfNeeded()
		}
	}

	@objc static func shutdown() {
		if #available(macOS 13.0, *) {
			RemoteControlBootstrap.shared.shutdown()
		}
	}
}
