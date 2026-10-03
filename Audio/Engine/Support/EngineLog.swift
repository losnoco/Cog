//
//  EngineLog.swift
//  CogAudio
//
//  Created by Christopher Snowhill on 9/29/26.
//

import Foundation
import os

/// Engine diagnostics. Watch with:
///
///     log stream --level debug --predicate 'subsystem == "org.cogx.cog" && category == "Engine"'
enum EngineLog {
	static let logger = Logger(subsystem: "org.cogx.cog", category: "Engine")

	/// A worker pass taking longer than this is reported.
	static let slowPass: TimeInterval = 0.02

	/// A slow feeder pass is a warning only with less than this decoded
	/// ahead in the deep ring; otherwise it is logged at debug level.
	static let shallowDeepRing: TimeInterval = 0.5

	static func now() -> UInt64 {
		DispatchTime.now().uptimeNanoseconds
	}

	static func milliseconds(since start: UInt64) -> Double {
		Double(DispatchTime.now().uptimeNanoseconds - start) / 1_000_000
	}
}
