//
//  CueSheetDecoder.swift
//  CueSheet
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

import CogAudio
import Foundation

@objc(CueSheetDecoder)
class CueSheetDecoder: NSObject, CogDecoder {
	private static let observerContext = UnsafeMutableRawPointer.allocate(byteCount: 1, alignment: 1)

	private var sourceURL: URL?
	private var baseURL: URL?
	private var source: CogSource?
	// @objc so the feeder can look inside this wrapper with KVC, as it could
	// when the ivar was Objective-C's.
	@objc private var decoder: CogDecoder?

	private var cuesheet: CueSheet?
	private var track: CueSheetTrack?

	private var embedded = false
	private var noFragment = false
	private var observersAdded = false

	private var isDSD = false
	private var bytesPerFrame = 0

	private var trackStart = 0
	private var trackEnd = 0
	private var framePosition = 0
	private var seekedToStart = false

	@objc class func fileTypes() -> [Any] {
		return CueSheetContainer.fileTypes()
	}

	@objc class func mimeTypes() -> [Any] {
		return CueSheetContainer.mimeTypes()
	}

	@objc class func priority() -> Float {
		return 16.0
	}

	@objc class func fileTypeAssociations() -> [Any] {
		return [
			["CUE Sheet File", "cue.icns", "cue"]
		]
	}

	// The frame range of the track at index: its own start to the next
	// track's, or the file's end when the next track plays another file.
	class func trackRange(at index: Int, ofTracks tracks: [CueSheetTrack], embedded: Bool, fileProperties properties: [AnyHashable: Any]) -> (start: Int, end: Int) {
		let track = tracks[index]

		let nextTrack: CueSheetTrack? = index + 1 < tracks.count ? tracks[index + 1] : nil

		let sampleRate = (properties["sampleRate"] as? NSNumber)?.floatValue ?? 0

		var trackStart = track.time
		if !track.timeInSamples { trackStart *= Double(sampleRate) }

		var trackEnd: Double
		let nextURLString = nextTrack?.url?.absoluteString as NSString?
		let sameFile = nextURLString?.isEqual(track.url?.absoluteString) ?? false
		if let nextTrack, embedded || sameFile {
			trackEnd = nextTrack.time
			if !nextTrack.timeInSamples { trackEnd *= Double(sampleRate) }
		} else {
			trackEnd = (properties["totalFrames"] as? NSNumber)?.doubleValue ?? 0
		}

		return (Int(trackStart), Int(trackEnd))
	}

	class func properties(withFileProperties fileProperties: [AnyHashable: Any], start: Int, end: Int) -> [AnyHashable: Any] {
		var properties = fileProperties

		// Need to alter length
		properties["totalFrames"] = end - start

		return properties
	}

	@objc func properties() -> [AnyHashable: Any]? {
		if noFragment {
			return decoder?.properties() ?? [:]
		}

		return CueSheetDecoder.properties(withFileProperties: decoder?.properties() ?? [:], start: trackStart, end: trackEnd)
	}

	@objc func metadata() -> [AnyHashable: Any]? {
		var metadata: [AnyHashable: Any] = [:]
		if let track {
			metadata = CueSheetMetadataReader.processData(forTrack: track)
		}
		if let decoderMetadata = decoder?.metadata() {
			// Cue-sheet fields describe this logical track and take priority over
			// the metadata shared by the underlying audio file. Merge explicitly
			// to avoid Objective-C category selector collisions between plug-ins.
			var mergedMetadata = decoderMetadata
			for (key, value) in metadata {
				mergedMetadata[key] = value
			}
			return mergedMetadata
		}
		return metadata
	}

	func open(_ s: CogSource) -> Bool {
		guard let url = s.url() else { return false }
		if !url.isFileURL {
			return false
		}

		sourceURL = url

		embedded = false
		cuesheet = nil
		var fileMetadata: [AnyHashable: Any]?

		noFragment = false
		observersAdded = false

		let ext = url.pathExtension
		if ext.caseInsensitiveCompare("cue") != .orderedSame {
			// Embedded cuesheet check
			fileMetadata = AudioMetadataReader.metadata(for: url, skipCue: true)

			source = s

			decoder = AudioDecoder.audioDecoder(for: s, skipCue: true)

			registerObservers()

			guard let decoder, decoder.open(s) else {
				NSLog("Could not open cuesheet decoder")
				return false
			}

			let alsoMetadata = decoder.metadata()

			var sheetString = CueSheet.sheetString(in: fileMetadata)
			if sheetString == nil || sheetString!.isEmpty {
				sheetString = CueSheet.sheetString(in: alsoMetadata)
			}

			if let sheetString, !sheetString.isEmpty {
				cuesheet = CueSheet(string: sheetString, withFilename: url.path)
				embedded = true
			}

			baseURL = url

			let fragment = url.fragment
			if fragment == nil || fragment == "" {
				noFragment = true
			}
		} else {
			cuesheet = CueSheet(file: url.path)
		}

		if !noFragment {
			let tracks = cuesheet?.tracks ?? []
			for i in 0 ..< tracks.count {
				if tracks[i].track == url.fragment {
					track = tracks[i]

					if !embedded {
						source = track?.url.flatMap { AudioSource.audioSource(for: $0) }

						guard let source, let trackURL = track?.url, source.open(trackURL) else {
							NSLog("Could not open cuesheet source")
							return false
						}

						decoder = AudioDecoder.audioDecoder(for: source, skipCue: true)

						guard let decoder, decoder.open(source) else {
							NSLog("Could not open cuesheet decoder")
							return false
						}
					}

					guard let decoder else { return false }
					let properties = decoder.properties() ?? [:]
					let bitsPerSample = (properties["bitsPerSample"] as? NSNumber)?.intValue ?? 0
					let channels = (properties["channels"] as? NSNumber)?.intValue ?? 0

					isDSD = bitsPerSample == 1

					bytesPerFrame = ((bitsPerSample + 7) / 8) * channels

					let range = CueSheetDecoder.trackRange(at: i, ofTracks: tracks, embedded: embedded, fileProperties: properties)
					trackStart = range.start
					trackEnd = range.end

					seekedToStart = false

					// Note: Should register for observations of the decoder
					willChangeValue(forKey: "properties")
					didChangeValue(forKey: "properties")

					return true
				}
			}
		} else {
			// Fix for embedded cuesheet handler parsing non-embedded files,
			// or files that are already in the playlist without a fragment
			guard let decoder else { return false }
			let properties = decoder.properties() ?? [:]
			let bitsPerSample = (properties["bitsPerSample"] as? NSNumber)?.intValue ?? 0
			let channels = (properties["channels"] as? NSNumber)?.intValue ?? 0

			isDSD = bitsPerSample == 1

			bytesPerFrame = ((bitsPerSample + 7) / 8) * channels

			trackStart = 0

			trackEnd = Int((properties["totalFrames"] as? NSNumber)?.doubleValue ?? 0)

			seekedToStart = false

			return true
		}

		return false
	}

	private func registerObservers() {
		if !observersAdded {
			#if DEBUG
				NSLog("REGISTERING OBSERVERS")
			#endif
			decoder?.addObserver?(self, forKeyPath: "properties", options: .new, context: Self.observerContext)
			decoder?.addObserver?(self, forKeyPath: "metadata", options: .new, context: Self.observerContext)

			observersAdded = true
		}
	}

	private func removeObservers() {
		if observersAdded {
			decoder?.removeObserver?(self, forKeyPath: "properties", context: Self.observerContext)
			decoder?.removeObserver?(self, forKeyPath: "metadata", context: Self.observerContext)
			observersAdded = false
		}
	}

	override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		if context == Self.observerContext {
			if let keyPath {
				willChangeValue(forKey: keyPath)
				didChangeValue(forKey: keyPath)
			}
		} else {
			super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
		}
	}

	func close() {
		if let decoder {
			removeObservers()
			decoder.close()
			self.decoder = nil
		}

		source = nil
		cuesheet = nil
		track = nil
	}

	func interrupt() {
		if let decoder, decoder.responds(to: #selector(CogDecoder.interrupt)) {
			decoder.interrupt?()
			return
		}

		source?.interrupt?()
	}

	deinit {
		close()
	}

	func setTrack(_ url: URL) -> Bool {
		// handling the file directly
		if noFragment {
			return false
		}

		var pathsAreFiles = false

		if url.isFileURL && sourceURL?.isFileURL == true {
			pathsAreFiles = true
		}

		// Same file, just next track...this may be unnecessary since frame-based decoding is done now...
		let samePath = sourceURL?.path == url.path
		let sameHost = (sourceURL?.host as NSString?)?.isEqual(url.host) ?? false
		if embedded || (samePath && (pathsAreFiles || sameHost)) {
			let tracks = cuesheet?.tracks ?? []

			for i in 0 ..< tracks.count {
				if tracks[i].track == url.fragment {
					let newTrack = tracks[i]

					if !((newTrack.url as NSURL?)?.isEqual(track?.url) ?? false) {
						return false
					}

					track = newTrack

					let range = CueSheetDecoder.trackRange(at: i, ofTracks: tracks, embedded: embedded, fileProperties: decoder?.properties() ?? [:])
					trackStart = range.start
					trackEnd = range.end

					seekedToStart = false

					#if DEBUG
						NSLog("CHANGING TRACK!")
					#endif
					return true
				}
			}
		}

		return false
	}

	func seek(_ frame: Int) -> Int {
		if !noFragment && frame > trackEnd - trackStart {
			// need a better way of returning fail.
			return -1
		}

		seekedToStart = true

		framePosition = decoder?.seek(frame + trackStart) ?? 0

		return framePosition - trackStart
	}

	func readAudio() -> AudioChunk? {
		if !seekedToStart {
			seek(0)
		}

		var frames = Int(Int32.max)

		if !noFragment && framePosition + frames > trackEnd {
			frames = Int(Int32(bitPattern: UInt32(truncatingIfNeeded: trackEnd - framePosition)))
		}

		if frames == 0 {
			#if DEBUG
				NSLog("Returning 0")
			#endif
			return nil
		}

		let frameScale = isDSD ? 8 : 1

		guard let chunk = decoder?.readAudio() else {
			return nil
		}

		let n = chunk.frameCount() * frameScale
		if n > frames {
			chunk.setFrameCount(frames / frameScale)
		}

		let streamTimestamp = Double(framePosition - trackStart) / chunk.format.mSampleRate
		chunk.streamTimestamp = streamTimestamp

		framePosition += chunk.frameCount() * frameScale

		return chunk
	}

	func isSilence() -> Bool {
		return decoder?.isSilence?() ?? false
	}
}