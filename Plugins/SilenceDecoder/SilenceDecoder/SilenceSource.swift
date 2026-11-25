//
//  SilenceSource.swift
//  SilenceSource
//
//  Created by Christopher Snowhill on 2/8/15.
//  Copyright 2015 __NoWork, LLC__. All rights reserved.
//

import CogAudio
import Foundation

@objc(SilenceSource)
class SilenceSource: NSObject, CogSource {
	private var urlStorage: URL?

	@objc class func schemes() -> [Any] {
		return ["silence"]
	}

	func open(_ url: URL) -> Bool {
		urlStorage = url
		return true
	}

	func seekable() -> Bool {
		return true
	}

	func seek(_ position: Int, whence: Int32) -> Bool {
		return true
	}

	func tell() -> Int {
		return 0
	}

	func read(_ buffer: UnsafeMutableRawPointer, amount: Int) -> Int {
		buffer.initializeMemory(as: UInt8.self, repeating: 0, count: amount)
		return amount
	}

	func close() {
	}

	func url() -> URL? {
		return urlStorage
	}

	func mimeType() -> String? {
		return "audio/x-silence"
	}
}