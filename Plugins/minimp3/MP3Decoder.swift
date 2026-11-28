//
//  MP3Decoder.swift
//  minimp3
//
//  Created by Vincent Spader on 6/17/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

import CogAudio
import Foundation

private func mp3ReadCallback(_ buf: UnsafeMutableRawPointer?, _ size: Int, _ userData: UnsafeMutableRawPointer?) -> Int {
	let file = Unmanaged<AnyObject>.fromOpaque(userData!).takeUnretainedValue() as! CogSource
	return file.read(buf, amount: size)
}

private func mp3SeekCallback(_ position: UInt64, _ userData: UnsafeMutableRawPointer?) -> Int32 {
	let file = Unmanaged<AnyObject>.fromOpaque(userData!).takeUnretainedValue() as! CogSource
	return file.seek(Int(position), whence: SEEK_SET) ? 0 : -1
}

// A chapter fragment is a plain decimal index, the whole of it: "2", not
// "2x" or "-1". The digit cap keeps an absurd fragment from overflowing the
// parse; any real chapter count is far below it.
private func chapterIndexFromFragment(_ fragment: String?) -> Int? {
	guard let fragment, !fragment.isEmpty, fragment.count <= 9 else { return nil }
	for c in fragment.unicodeScalars where c < "0" || c > "9" {
		return nil
	}
	return Int(fragment)
}

@objc(MP3Decoder)
class MP3Decoder: NSObject, CogDecoder {
	private var seekable = false
	private let decoderBuffer = UnsafeMutablePointer<UInt8>.allocate(capacity: Int(MP3DecoderBufferSize))
	private var decoderBufferFilled = 0

	private var decoderEx = mp3dec_ex_t()
	private var decoderIO = mp3dec_io_t()
	private var decoderInfo = mp3dec_frame_info_t()

	private var samplesFilled = 0
	private let outputBuffer = UnsafeMutablePointer<mp3d_sample_t>.allocate(capacity: Int(MP3DecoderMaxSamplesPerFrame))

	private var seconds: Double = 0

	private var source: CogSource?

	private var startPadding: UInt32 = 0
	private var endPadding: UInt32 = 0
	private var foundiTunSMPB = false

	private var fileSize = 0
	private var framesDecoded = 0
	private var foundID3v2 = false

	// The chapter of "file.mp3#N" being played: the ID3v2 chapters in the
	// order FFMPEGContainer numbers them, and the span's content frames.
	private var isChapter = false
	private var chapters: [NSDictionary]?
	private var chapterIndex = 0
	private var chapterStartFrame = 0
	private var fileTotalFrames = 0
	private var fileTitle = ""

	private var inputEOF = false

	private var channels: Int32 = 0
	private var sampleRate: Float = 0
	private var bitrate: Int32 = 0
	private var totalFrames = 0
	private var layer: Int32 = 0

	private var metadataUpdateInterval = 0
	private var metadataUpdateCount = 0

	private var genre: String?
	private var album: String?
	private var artist: String?
	private var title: String?

	deinit {
		decoderBuffer.deallocate()
		outputBuffer.deallocate()
	}

	func open(_ source: CogSource) -> Bool {
		// "file.mp3#2" is a chapter FFMPEGContainer listed. Its span comes from
		// the ID3v2 chapters, read below; a fragment that is not a decimal
		// index, or has no chapters to index, is refused, passing it to FFmpeg's
		// decoder, next in priority.
		let fragment = source.url().fragment
		var fragmentIndex = 0
		var wantsChapter = false
		if let fragment, !fragment.isEmpty {
			if !source.seekable() { return false }
			guard let idx = chapterIndexFromFragment(fragment) else { return false }
			fragmentIndex = idx
			wantsChapter = true
		}

		isChapter = false
		chapters = nil
		chapterIndex = 0
		chapterStartFrame = 0
		fileTotalFrames = 0
		fileTitle = ""

		self.source = source

		seekable = source.seekable()

		var id3Length = 0

		if seekable {
			// minimp3 already skips ID3v2, but we need to supplement it with
			// support for iTunes gapless info, stored in the ID3v2 tag.
			var buffer = [UInt8](repeating: 0, count: 10)
			let buflen = source.read(&buffer, amount: 10)

			source.seek(0, whence: SEEK_END)
			fileSize = source.tell()
			source.seek(10, whence: SEEK_SET)

			if buflen >= 10 && buffer[0] == 0x49 && buffer[1] == 0x44 && buffer[2] == 0x33 {
				id3Length = (Int(buffer[6] & 0x7F) << 21) | (Int(buffer[7] & 0x7F) << 14) |
					(Int(buffer[8] & 0x7F) << 7) | Int(buffer[9] & 0x7F)

				foundID3v2 = true

				// Add 10 bytes for ID3 header
				id3Length += 10

				let tagBuffer = UnsafeMutablePointer<UInt8>.allocate(capacity: id3Length)
				defer { tagBuffer.deallocate() }

				let initial = min(buflen, id3Length)
				tagBuffer.update(from: &buffer, count: initial)

				var bufleft = id3Length - buflen
				var tagRead = initial

				while bufleft > 0 {
					let bufRead = source.read(tagBuffer + tagRead, amount: bufleft)
					if bufRead <= 0 {
						return false
					}
					tagRead += bufRead
					if bufRead < bufleft {
						return false
					}
					bufleft -= bufRead
				}

				if let tag = id3_tag_parse(tagBuffer, id3_length_t(id3Length)) {
					for i in 0..<Int(tag.pointee.nframes) {
						guard let frame = tag.pointee.frames![i] else { continue }
						let idBytes = frame.pointee.id
						let frameID = String(cString: [idBytes.0, idBytes.1, idBytes.2, idBytes.3, idBytes.4])
						if frameID == "COMM" {
							let descriptionField = id3_frame_field(frame, 2)
							let description = id3_field_getstring(descriptionField)
							let valueField = id3_frame_field(frame, 3)
							let value = id3_field_getfullstring(valueField)

							if let description, let value {
								let description8 = id3_ucs4_utf8duplicate(description)
								if String(cString: description8!) == "iTunSMPB" {
									let value8 = id3_ucs4_utf8duplicate(value)
									parseiTunSMPB(String(cString: value8!), id3Length: id3Length)
									free(value8)
								}
								free(description8)
							}
						} else if frameID == "TIT2" {
							// The file's own title, which a chapter row carries
							// as its album (FFmpeg's rule). A text frame's string
							// is a string list, so take its first entry.
							let field = id3_frame_field(frame, 1)
							if id3_field_getnstrings(field) > 0 {
								if let value = id3_field_getstrings(field, 0) {
									let value8 = id3_ucs4_utf8duplicate(value)
									fileTitle = String(cString: value8!)
									free(value8)
								}
							}
						}
					}

					id3_tag_delete(tag)
				}

				if wantsChapter {
					guard let tagReader = NSClassFromString("TagLibID3v2Reader") as? NSObject.Type,
					      tagReader.responds(to: Selector(("chaptersForTag:"))) else {
						return false
					}
					let data = Data(bytes: tagBuffer, count: id3Length)
					let unsorted = tagReader.perform(Selector(("chaptersForTag:")), with: data)?.takeUnretainedValue() as? [NSDictionary] ?? []
					// FFMPEGContainer numbers the rows as FFmpeg lists them, by
					// start, so the fragment indexes the same order.
					chapters = unsorted.sorted { a, b in
						(a[CogChapterStart] as? NSNumber)?.compare(b[CogChapterStart] as? NSNumber ?? 0) == .orderedAscending
					}
					if fragmentIndex >= (chapters?.count ?? 0) {
						return false
					}
					chapterIndex = fragmentIndex
				}
			}

			if wantsChapter && (chapters?.count ?? 0) == 0 { return false }

			decoderIO.read = mp3ReadCallback
			decoderIO.read_data = Unmanaged.passUnretained(source as AnyObject).toOpaque()
			decoderIO.seek = mp3SeekCallback
			decoderIO.seek_data = Unmanaged.passUnretained(source as AnyObject).toOpaque()
			let error = mp3dec_ex_open_cb(&decoderEx, &decoderIO, MP3D_SEEK_TO_SAMPLE)
			if error != 0 {
				return false
			}
			if foundiTunSMPB {
				// start_delay is used for seeking, to_skip must be filled for the
				// first packet decoded and detected_samples will truncate the end padding
				decoderEx.start_delay = Int32(startPadding) * decoderEx.info.channels
				decoderEx.to_skip = decoderEx.start_delay
				decoderEx.detected_samples = UInt64(totalFrames) * UInt64(decoderEx.info.channels)
				decoderEx.samples = UInt64(totalFrames + Int(startPadding) + Int(endPadding)) * UInt64(decoderEx.info.channels)
			}
			if !foundiTunSMPB {
				var samples = decoderEx.detected_samples
				if samples == 0 {
					samples = decoderEx.samples
				}
				totalFrames = Int(samples) / Int(decoderEx.info.channels)
			}
			bitrate = Int32(Double((fileSize - id3Length) * 8) / (Double(totalFrames) / Double(decoderEx.info.hz)) / 1000.0)
			if wantsChapter {
				// The chapter's span, positioned at its start; the first frame
				// decoded below is the chapter's own.
				var startFrame = 0, endFrame = 0
				if !chapterSpan(at: chapterIndex, start: &startFrame, end: &endFrame, fileFrames: totalFrames) ||
					mp3dec_ex_seek(&decoderEx, UInt64(startFrame) * UInt64(decoderEx.info.channels)) < 0 {
					mp3dec_ex_close(&decoderEx)
					return false
				}
				fileTotalFrames = totalFrames
				chapterStartFrame = startFrame
				totalFrames = endFrame - startFrame
				isChapter = true
			}
			var samplePtr: UnsafeMutablePointer<mp3d_sample_t>?
			var samples = 0
			var retry = 10
			repeat {
				samples = mp3dec_ex_read_frame(&decoderEx, &samplePtr, &decoderInfo, Int(MP3DecoderMaxSamplesPerFrame))
			} while samples == 0 && { retry -= 1; return retry > 0 }()
			if samples > 0, let samplePtr {
				samplesFilled = samples / Int(decoderInfo.channels)
				outputBuffer.update(from: samplePtr, count: samples)
			} else {
				inputEOF = true
				mp3dec_ex_close(&decoderEx)
				return false
			}
			inputEOF = false
		} else {
			decoderBufferFilled = source.read(decoderBuffer, amount: Int(MP3DecoderBufferSize))
			inputEOF = decoderBufferFilled < Int(MP3DecoderBufferSize)
			mp3dec_init(&decoderEx.mp3d)
			var error = mp3dec_detect_buf(decoderBuffer, decoderBufferFilled)
			if error != 0 {
				return false
			}

			while true {
				error = mp3dec_decode_frame(&decoderEx.mp3d, decoderBuffer, Int32(decoderBufferFilled), outputBuffer, &decoderInfo)
				if Int(decoderInfo.frame_bytes) > decoderBufferFilled {
					break
				}
				decoderBufferFilled -= Int(decoderInfo.frame_bytes)
				decoderBuffer.update(from: decoderBuffer + Int(decoderInfo.frame_bytes), count: decoderBufferFilled)
				if error != 0 {
					break
				}
			}
			if error == 0 {
				return false
			}
			samplesFilled = Int(error)
			bitrate = decoderInfo.bitrate_kbps
		}

		syncFormat()

		seconds = 0.0

		genre = ""
		album = ""
		artist = ""
		title = ""
		if isChapter {
			title = (chapters?[chapterIndex][CogChapterTitle] as? String) ?? ""
			album = fileTitle
		}

		metadataUpdateInterval = Int(sampleRate)
		metadataUpdateCount = 0

		return true
	}

	private func parseiTunSMPB(_ value: String, id3Length: Int) {
		let parts = value.split(separator: " ").map(String.init)
		guard parts.count == 6,
		      let startPad = UInt32(parts[1], radix: 16),
		      let endPad = UInt32(parts[2], radix: 16),
		      let tempDuration = Int64(parts[3], radix: 16),
		      let lastOffset = UInt64(parts[5], radix: 16) else {
			return
		}
		guard tempDuration >= 0,
		      startPad <= 576 * 2 * 32,
		      endPad <= 576 * 2 * 64,
		      fileSize != 0, lastOffset < UInt64(fileSize - id3Length) else {
			return
		}
		if endPad >= 528 + 1 {
			startPadding = startPad + 528 + 1
			endPadding = endPad - (528 + 1)
			// iTunes encodes the original length of the file here
			totalFrames = Int(tempDuration)
			foundiTunSMPB = true
		}
	}

	@discardableResult
	private func syncFormat() -> Bool {
		let newSampleRate = Float(decoderInfo.hz)
		let newChannels = decoderInfo.channels
		let newLayer = decoderInfo.layer

		let changed = newSampleRate != sampleRate || newChannels != channels || newLayer != layer

		if changed {
			sampleRate = newSampleRate
			channels = newChannels
			layer = newLayer

			willChangeValue(forKey: "properties")
			didChangeValue(forKey: "properties")
		}

		return changed
	}

	func readAudio() -> AudioChunk? {
		var chunk: AudioChunk?

		while true {
			var framesToCopy = samplesFilled

			if isChapter {
				// The chapter ends where the next one starts; the rest of a
				// frame that straddles the boundary belongs to the next track.
				let remaining = totalFrames - framesDecoded
				if remaining <= 0 {
					inputEOF = true
					break
				}
				if framesToCopy > remaining {
					framesToCopy = remaining
				}
			}

			if framesToCopy > 0 {
				let newChunk = AudioChunk(properties: properties() ?? [:])
				newChunk.streamTimestamp = seconds
				newChunk.assignSamples(outputBuffer, frameCount: framesToCopy)
				seconds += newChunk.duration()
				samplesFilled = 0
				framesDecoded += framesToCopy
				chunk = newChunk
				break
			}

			if seekable {
				var samplePtr: UnsafeMutablePointer<mp3d_sample_t>?
				let samples = mp3dec_ex_read_frame(&decoderEx, &samplePtr, &decoderInfo, Int(MP3DecoderMaxSamplesPerFrame))
				if samples > 0, let samplePtr {
					samplesFilled = samples / Int(decoderInfo.channels)
					outputBuffer.update(from: samplePtr, count: samples)
				} else {
					inputEOF = true
				}
			} else {
				let bytesRemain = Int(MP3DecoderBufferSize) - decoderBufferFilled
				let bytesRead = source?.read(decoderBuffer + decoderBufferFilled, amount: bytesRemain) ?? -1
				if bytesRead < 0 || bytesRead < bytesRemain {
					inputEOF = true
				}
				if bytesRead > 0 {
					decoderBufferFilled += bytesRead
				}
				let samples = mp3dec_decode_frame(&decoderEx.mp3d, decoderBuffer, Int32(decoderBufferFilled), outputBuffer, &decoderInfo)
				if samples > 0 {
					samplesFilled = Int(samples)
				}
				if Int(decoderInfo.frame_bytes) > decoderBufferFilled {
					inputEOF = true
				} else {
					decoderBufferFilled -= Int(decoderInfo.frame_bytes)
					decoderBuffer.update(from: decoderBuffer + Int(decoderInfo.frame_bytes), count: decoderBufferFilled)
				}
			}

			syncFormat()

			if inputEOF {
				break
			}
		}

		metadataUpdateCount += chunk.map { Int($0.frameCount()) } ?? 0
		if metadataUpdateCount >= metadataUpdateInterval {
			metadataUpdateCount -= metadataUpdateInterval
			updateMetadata()
		}

		return chunk
	}

	func close() {
		if seekable {
			mp3dec_ex_close(&decoderEx)
		}
		source = nil
	}

	func interrupt() {
		if let s = source as? NSObject, s.responds(to: Selector(("interrupt"))) {
			s.perform(Selector(("interrupt")))
		}
	}

	func seek(_ frame: Int) -> Int {
		var frame = frame
		if frame == framesDecoded {
			return frame
		}

		if frame > totalFrames {
			frame = totalFrames
		}

		if seekable {
			let absolute = isChapter ? chapterStartFrame + frame : frame
			let error = mp3dec_ex_seek(&decoderEx, UInt64(absolute) * UInt64(decoderEx.info.channels))
			if error < 0 {
				return -1
			}
			samplesFilled = 0
			inputEOF = false
		} else {
			return -1
		}

		seconds = Double(frame) / Double(decoderInfo.hz)
		framesDecoded = frame

		return frame
	}

	// The chapter's span in content frames at the file's own rate, its end
	// clamped to the file's length. The tag keeps the times in milliseconds.
	private func chapterSpan(at index: Int, start: inout Int, end: inout Int, fileFrames: Int) -> Bool {
		guard let chapters, index < chapters.count else { return false }
		let chapter = chapters[index]
		let hz = UInt64(decoderEx.info.hz)
		if hz == 0 { return false }
		let startMs = (chapter[CogChapterStart] as? NSNumber)?.uint64Value ?? 0
		let endMs = (chapter[CogChapterEnd] as? NSNumber)?.uint64Value ?? 0
		var startFrame = Int((startMs * hz + 500) / 1000)
		var endFrame = Int((endMs * hz + 500) / 1000)
		if endFrame > fileFrames { endFrame = fileFrames }
		if startFrame >= endFrame { return false }
		start = startFrame
		end = endFrame
		return true
	}

	// The next chapter of the same file: the feeder carries on without
	// reopening. The seek is sample-accurate, and lands where the last chapter
	// ended when the two are contiguous.
	func setTrack(_ track: URL) -> Bool {
		if !isChapter { return false }
		if track.path != source?.url().path { return false }
		guard let index = chapterIndexFromFragment(track.fragment) else { return false }
		guard let chapters, index < chapters.count else { return false }
		var startFrame = 0, endFrame = 0
		if !chapterSpan(at: index, start: &startFrame, end: &endFrame, fileFrames: fileTotalFrames) { return false }
		if mp3dec_ex_seek(&decoderEx, UInt64(startFrame) * UInt64(decoderEx.info.channels)) < 0 { return false }

		samplesFilled = 0
		chapterIndex = index
		chapterStartFrame = startFrame
		totalFrames = endFrame - startFrame
		framesDecoded = 0
		seconds = 0.0
		inputEOF = false
		title = (chapters[chapterIndex][CogChapterTitle] as? String) ?? ""

		willChangeValue(forKey: "properties")
		didChangeValue(forKey: "properties")
		willChangeValue(forKey: "metadata")
		didChangeValue(forKey: "metadata")

		return true
	}

	private func updateMetadata() {
		// A chapter's title and album are fixed at open; only a stream's
		// metadata changes mid-play, and a chapter is never a stream.
		if isChapter { return }
		var newArtist = artist
		var newAlbum = album
		var newTitle = title
		var newGenre = genre

		if let httpSource = source as? CogHTTPSource {
			if httpSource.hasMetadata() {
				let metadata = httpSource.metadata() ?? [:]
				newGenre = metadata["genre"] as? String
				newAlbum = metadata["album"] as? String
				newArtist = metadata["artist"] as? String
				newTitle = metadata["title"] as? String
			}
		}

		if newArtist != artist || newAlbum != album || newTitle != title || newGenre != genre {
			artist = newArtist
			album = newAlbum
			title = newTitle
			genre = newGenre
			if source?.seekable() != true {
				willChangeValue(forKey: "metadata")
				didChangeValue(forKey: "metadata")
			}
		}
	}

	@objc func properties() -> [AnyHashable: Any]? {
		if layer < 1 || layer > 3 { return nil }
		let layers = ["MP1", "MP2", "MP3"]
		return ["channels": channels,
		        "bitsPerSample": 32,
		        "sampleRate": sampleRate,
		        "floatingPoint": true,
		        "bitrate": bitrate,
		        "totalFrames": totalFrames,
		        "seekable": seekable,
		        "codec": layers[Int(layer) - 1],
		        "endian": "host",
		        "encoding": "lossy"]
	}

	@objc func metadata() -> [AnyHashable: Any]? {
		var dict: [String: String] = [:]
		if let artist { dict["artist"] = artist }
		if let album { dict["album"] = album }
		if let title { dict["title"] = title }
		if let genre { dict["genre"] = genre }
		return dict
	}

	@objc class func fileTypes() -> [Any] {
		return ["mp3", "m2a", "mpa"]
	}

	@objc class func mimeTypes() -> [Any] {
		return ["audio/mpeg", "audio/x-mp3"]
	}

	@objc class func fileTypeAssociations() -> [Any] {
		return [["MPEG Audio File", "mp3.icns", "mp3", "m2a", "mpa"]]
	}

	@objc class func priority() -> Float {
		return 2.0
	}
}
