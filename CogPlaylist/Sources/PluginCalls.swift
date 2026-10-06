//
//  PluginCalls.swift
//  CogPlaylist
//
//  Calls into the plugins, which can raise an Objective-C exception on a file
//  they cannot parse. Swift can neither catch one nor unwind through one
//  safely, so each call goes through CogCatchException, and what it raised
//  is reported (the macOS app sends it to Sentry) while the call counts as
//  having found nothing.
//

import CogAudio
import Foundation

@objc public final class PluginCalls: NSObject {
	/// Told of each exception a plugin raised, and the URL it was reading.
	@objc public static var exceptionHandler: ((NSException, URL?) -> Void)?

	/// `body`'s result, or nil if a plugin raised an exception in it.
	static func run<T>(for url: URL?, _ body: () -> T?) -> T? {
		var result: T?
		if let exception = CogCatchException({ result = body() }) {
			if let exceptionHandler {
				exceptionHandler(exception, url)
			} else {
				NSLog("A plugin raised %@ reading %@: %@", exception.name.rawValue, url?.absoluteString ?? "-", exception.reason ?? "")
			}
			return nil
		}
		return result
	}
}
