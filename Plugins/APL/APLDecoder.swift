//
//  APLDecoder.swift
//  APL
//
//  Plays the block range of the audio file an APL link points at, through
//  the decoder that file would use on its own.
//

import CogAudio
import Foundation

@objc(APLDecoder)
class APLDecoder: NSObject, CogDecoder {
	private var source: CogSource?
	private var decoder: CogDecoder?

	private var bytesPerFrame = 0 // Number of bytes per frame, ie channels * (bitsPerSample/8)
	private var framePosition = 0 // current position in frames

	private var trackStart = 0
	private var trackEnd = 0 // frames until end of track.
	private var trackLength = 0 // track len in frames

	private var isDSD = false

	private var apl: APLFile?

	@objc class func fileTypes() -> [Any] {
		return ["apl"]
	}

	@objc class func mimeTypes() -> [Any] {
		return ["application/x-apl"]
	}

	@objc class func priority() -> Float {
		return 1.0
	}

	@objc class func fileTypeAssociations() -> [Any] {
		return [
			["APL Link File", "song.icns", "apl"]
		]
	}

	@objc func properties() -> [AnyHashable: Any]? {
		guard let decoder else { return [:] }
		var properties = decoder.properties() ?? [:]

		// Need to alter length
		properties["totalFrames"] = trackLength
		return properties
	}

	@objc func metadata() -> [AnyHashable: Any]? {
		return [:]
	}

	func open(_ s: CogSource) -> Bool {
		guard let url = s.url() else { return false }
		if !url.isFileURL {
			return false
		}

		apl = APLFile(file: url.path)

		let aplURL = apl?.file
		source = aplURL.flatMap { AudioSource.audioSource(for: $0) }

		guard let source, let aplURL, source.open(aplURL) else {
			NSLog("Could not open source for file '\(apl?.file?.description ?? "(null)")' referenced in apl")
			return false
		}
		decoder = AudioDecoder.audioDecoder(for: source, skipCue: true)

		guard let decoder, decoder.open(source) else {
			NSLog("Could not open decoder for source for apl")
			return false
		}

		let properties = decoder.properties() ?? [:]
		let bitsPerSample = (properties["bitsPerSample"] as? NSNumber)?.intValue ?? 0
		let channels = (properties["channels"] as? NSNumber)?.intValue ?? 0

		isDSD = bitsPerSample == 1

		bytesPerFrame = ((bitsPerSample + 7) / 8) * channels

		if let apl, apl.endBlock > apl.startBlock {
			trackEnd = apl.endBlock
		} else {
			trackEnd = Int((properties["totalFrames"] as? NSNumber)?.doubleValue ?? 0)
		}

		trackStart = apl?.startBlock ?? 0
		trackLength = trackEnd - trackStart

		seek(0)

		// Note: Should register for observations of the decoder, but laziness consumes all.
		willChangeValue(forKey: "properties")
		didChangeValue(forKey: "properties")
		return true
	}

	func close() {
		if let decoder {
			decoder.close()
			self.decoder = nil
		}

		source = nil
		apl = nil
	}

	deinit {
		close()
	}

	func seek(_ frame: Int) -> Int {
		if frame > trackEnd - trackStart {
			// need a better way of returning fail.
			return -1
		}

		framePosition = decoder?.seek(frame + trackStart) ?? 0

		return framePosition
	}

	func readAudio() -> AudioChunk? {
		var maxFrames = Int(Int32.max)

		if framePosition + maxFrames > trackEnd {
			maxFrames = Int(Int32(truncatingIfNeeded: trackEnd - framePosition))
		}

		if maxFrames == 0 {
			#if DEBUG
				NSLog("APL readAudio Returning 0")
			#endif
			return nil
		}

		let frameScale = isDSD ? 8 : 1

		guard let chunk = decoder?.readAudio() else {
			return nil
		}
		if chunk.frameCount() * frameScale > maxFrames {
			chunk.setFrameCount(maxFrames / frameScale)
		}

		framePosition += chunk.frameCount() * frameScale
		return chunk
	}

	func isSilence() -> Bool {
		return decoder?.isSilence?() ?? false
	}
}