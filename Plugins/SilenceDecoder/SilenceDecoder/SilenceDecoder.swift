//
//  SilenceDecoder.swift
//  Cog
//
//  Created by Christopher Snowhill on 2/8/15.
//  Copyright 2015 __NoWork, LLC__. All rights reserved.
//

import CogAudio
import Foundation

@objc(SilenceDecoder)
class SilenceDecoder: NSObject, CogDecoder {
	private enum Constants {
		static let sampleRate = 44100
		static let channels = 2
	}

	private var sourceStorage: CogSource?

	private var length = 0
	private var remain = 0

	private var seconds = 0.0

	// Ten seconds of silence stand in for an unreadable file; the buffer
	// stays zeroed for as long as it is read.
	private let buffer = [Float](repeating: 0, count: 1024 * Constants.channels)

	@objc class func fileTypes() -> [Any] {
		return []
	}

	@objc class func mimeTypes() -> [Any] {
		return ["audio/x-silence"]
	}

	@objc class func priority() -> Float {
		return 1.0
	}

	@objc class func fileTypeAssociations() -> [Any] {
		return []
	}

	func open(_ s: CogSource) -> Bool {
		sourceStorage = s

		let path = String(s.url()?.relativeString.dropFirst(10) ?? "")

		var secs = (path as NSString).intValue
		if secs == 0 { secs = 10 }

		length = Int(secs) * Constants.sampleRate
		remain = length

		willChangeValue(forKey: "properties")
		didChangeValue(forKey: "properties")

		return true
	}

	func properties() -> [AnyHashable: Any]? {
		return ["bitrate": 0,
		        "sampleRate": Constants.sampleRate,
		        "totalFrames": length,
		        "bitsPerSample": 32,
		        "floatingPoint": true,
		        "channels": Constants.channels,
		        "seekable": true,
		        "endian": "host",
		        "encoding": "synthesized"]
	}

	func metadata() -> [AnyHashable: Any]? {
		return [:]
	}

	func readAudio() -> AudioChunk? {
		var frames = 1024

		// As PlaylistControllerEnums.h's IsRepeatOneSet: repeat one plays forever.
		if UserDefaults.standard.integer(forKey: "repeat") != 1 {
			if frames > remain {
				frames = Int(Int32(truncatingIfNeeded: remain))
			}

			remain -= frames
		}

		let chunk = AudioChunk(properties: properties() ?? [:])
		chunk.streamTimestamp = seconds
		buffer.withUnsafeBytes {
			chunk.assignSamples($0.baseAddress!, frameCount: frames)
		}

		seconds += chunk.duration()

		return chunk
	}

	func seek(_ frame: Int) -> Int {
		var frame = frame
		if frame > length {
			frame = length
		}

		remain = length - frame

		seconds = Double(frame) / Double(Constants.sampleRate)

		return frame
	}

	func close() {
	}

	func isSilence() -> Bool {
		return true
	}

	@objc func setSource(_ s: CogSource?) {
		sourceStorage = s
	}

	@objc func source() -> CogSource? {
		return sourceStorage
	}
}