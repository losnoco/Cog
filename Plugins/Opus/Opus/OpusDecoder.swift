//
//  OpusDecoder.swift
//  Opus
//
//  Created by Christopher Snowhill on 10/4/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

import CogAudio
import Foundation

private let maxChannels = 8
private let chmap: [[Int]] = [
	[0], // mono
	[0, 1], // l, r
	[0, 2, 1], // l, c, r -> l, r, c
	[0, 1, 2, 3], // l, r, bl, br
	[0, 2, 1, 3, 4], // l, c, r, bl, br -> l, r, c, bl, br
	[0, 2, 1, 5, 3, 4], // l, c, r, bl, br, lfe -> l, r, c, lfe, bl, br
	[0, 2, 1, 6, 5, 3, 4], // l, c, r, sl, sr, bc, lfe -> l, r, c, lfe, bc, sl, sr
	[0, 2, 1, 7, 5, 6, 3, 4] // l, c, r, sl, sr, bl, br, lfe -> l, r, c, lfe, bl, br, sl, sr
]

private func sourceRead(_ stream: UnsafeMutableRawPointer?, _ ptr: UnsafeMutablePointer<UInt8>?, _ nbytes: Int32) -> Int32 {
	let source = Unmanaged<AnyObject>.fromOpaque(stream!).takeUnretainedValue() as! CogSource
	return Int32(source.read(ptr, amount: Int(nbytes)))
}

private func sourceSeek(_ stream: UnsafeMutableRawPointer?, _ offset: opus_int64, _ whence: Int32) -> Int32 {
	let source = Unmanaged<AnyObject>.fromOpaque(stream!).takeUnretainedValue() as! CogSource
	return source.seek(Int(offset), whence: whence) ? 0 : -1
}

private func sourceClose(_ stream: UnsafeMutableRawPointer?) -> Int32 {
	0
}

private func sourceTell(_ stream: UnsafeMutableRawPointer?) -> opus_int64 {
	let source = Unmanaged<AnyObject>.fromOpaque(stream!).takeUnretainedValue() as! CogSource
	return opus_int64(source.tell())
}

private func setDictionary(_ dict: NSMutableDictionary, _ tag: String, _ value: String) {
	let realKey = tag.replacingOccurrences(of: ".", with: "․")
	let array: NSMutableArray
	if let existing = dict[realKey] as? NSMutableArray {
		array = existing
	} else {
		array = NSMutableArray()
		dict[realKey] = array
	}
	array.add(value)
}

@objc(OpusFile)
class OpusFile: NSObject, CogDecoder {
	private var source: CogSource?

	private var opusRef: OpaquePointer?
	private var currentSection: Int32 = 0
	private var lastSection: Int32 = 0

	private var seekable = false
	private var bitrate: Int32 = 0
	private var channels: Int32 = 0
	private var totalFrames = 0
	private var frame = 0

	private var metadataUpdateInterval = 0
	private var metadataUpdateCount = 0

	private var metaDict: NSDictionary = [:]
	private var icyMetaDict: NSDictionary = [:]

	private var albumArt = Data()

	private var replayGainAlbumGain: Float = 0
	private var replayGainTrackGain: Float = 0

	override init() {
		super.init()
		opusRef = nil
	}

	func open(_ s: CogSource) -> Bool {
		source = s

		var callbacks = OpusFileCallbacks(read: sourceRead, seek: sourceSeek, tell: sourceTell, close: sourceClose)

		var error: Int32 = 0
		opusRef = op_open_callbacks(Unmanaged.passUnretained(s as AnyObject).toOpaque(), &callbacks, nil, 0, &error)

		guard let opusRef else {
			return false
		}

		currentSection = op_current_link(opusRef)
		lastSection = currentSection

		bitrate = Int32(Double(op_bitrate(opusRef, currentSection)) / 1000.0)
		channels = op_channel_count(opusRef, currentSection)

		seekable = op_seekable(opusRef) != 0

		totalFrames = Int(op_pcm_total(opusRef, -1))
		frame = 0

		let head = op_head(opusRef, -1)
		let tags = op_tags(opusRef, -1)

		var trackGain: Int32 = 0
		if let tags {
			opus_tags_get_track_gain(tags, &trackGain)
		}

		replayGainAlbumGain = Float(Double(head!.pointee.output_gain) / 256.0 + 5.0)
		replayGainTrackGain = Float(Double(trackGain) / 256.0) + replayGainAlbumGain

		op_set_gain_offset(opusRef, OP_ABSOLUTE_GAIN, 0)

		willChangeValue(forKey: "properties")
		didChangeValue(forKey: "properties")

		metaDict = [:]
		icyMetaDict = [:]
		albumArt = Data()

		updateMetadata()

		metadataUpdateInterval = 48000
		metadataUpdateCount = 0

		return true
	}

	private func updateMetadata() {
		guard let opusRef, let tags = op_tags(opusRef, -1) else { return }
		let newMetaDict = NSMutableDictionary()
		var newAlbumArt = albumArt

		for i in 0..<Int(tags.pointee.comments) {
			var entry = FLAC__StreamMetadata_VorbisComment_Entry()
			entry.entry = UnsafeMutablePointer<FLAC__byte>(OpaquePointer(tags.pointee.user_comments![i]!))
			entry.length = UInt32(tags.pointee.comment_lengths![i])
			var name: UnsafeMutablePointer<CChar>?
			var value: UnsafeMutablePointer<CChar>?
			if FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair(entry, &name, &value) != 0 {
				autoreleasepool {
					var tagName = (guess_encoding_of_string(name) as String?) ?? ""
					free(name)
					tagName = tagName.lowercased()

					if tagName == "metadata_block_picture" {
						var pic = OpusPictureTag()
						if opus_picture_tag_parse(&pic, value!) >= 0 {
							if pic.format == OP_PIC_FORMAT_PNG ||
								pic.format == OP_PIC_FORMAT_JPEG ||
								pic.format == OP_PIC_FORMAT_GIF ||
								pic.format == OP_PIC_FORMAT_UNKNOWN {
								newAlbumArt = Data(bytes: pic.data, count: Int(pic.data_length))
							}
						}
						opus_picture_tag_clear(&pic)
					} else if tagName == "unsynced lyrics" ||
								tagName == "lyrics" {
						setDictionary(newMetaDict, "unsyncedlyrics", (guess_encoding_of_string(value) as String?) ?? "")
					} else if tagName == "comments:itunnorm" {
						setDictionary(newMetaDict, "soundcheck", (guess_encoding_of_string(value) as String?) ?? "")
					} else {
						setDictionary(newMetaDict, tagName, (guess_encoding_of_string(value) as String?) ?? "")
					}

					free(value)
				}
			}
		}

		if newAlbumArt != albumArt || !(newMetaDict as NSDictionary).isEqual(metaDict) {
			metaDict = newMetaDict
			albumArt = newAlbumArt

			if source?.seekable() != true {
				willChangeValue(forKey: "metadata")
				didChangeValue(forKey: "metadata")
			}
		}
	}

	private func updateIcyMetadata() {
		if source?.seekable() == true { return }

		let newIcyMetaDict = NSMutableDictionary()

		if let httpSource = source as? CogHTTPSource {
			if httpSource.hasMetadata() {
				autoreleasepool {
					let metadata = httpSource.metadata() ?? [:]
					let genre = metadata["genre"] as? String
					let album = metadata["album"] as? String
					let artist = metadata["artist"] as? String
					let title = metadata["title"] as? String

					if let genre, !genre.isEmpty {
						setDictionary(newIcyMetaDict, "genre", genre)
					}
					if let album, !album.isEmpty {
						setDictionary(newIcyMetaDict, "album", album)
					}
					if let artist, !artist.isEmpty {
						setDictionary(newIcyMetaDict, "artist", artist)
					}
					if let title, !title.isEmpty {
						setDictionary(newIcyMetaDict, "title", title)
					}
				}
			}
		}

		if !(newIcyMetaDict as NSDictionary).isEqual(icyMetaDict) {
			icyMetaDict = newIcyMetaDict
			willChangeValue(forKey: "metadata")
			didChangeValue(forKey: "metadata")
		}
	}

	func readAudio() -> AudioChunk? {
		guard let opusRef else { return nil }
		var numread: Int32 = 0
		var total = 0

		if currentSection != lastSection {
			bitrate = Int32(Double(op_bitrate(opusRef, currentSection)) / 1000.0)
			channels = op_channel_count(opusRef, currentSection)

			willChangeValue(forKey: "properties")
			didChangeValue(forKey: "properties")

			updateMetadata()
		}

		let frames = 1024
		let channelCount = Int(channels)
		let size = frames * channelCount
		let buffer = UnsafeMutablePointer<Float>.allocate(capacity: size)
		defer { buffer.deallocate() }
		let tempbuf = UnsafeMutablePointer<Float>.allocate(capacity: 512 * channelCount)
		defer { tempbuf.deallocate() }

		repeat {
			let out = buffer + total
			lastSection = currentSection
			var toread = size - total
			if toread > 512 { toread = 512 }
			numread = op_read_float(opusRef, channels < maxChannels ? tempbuf : out, Int32(toread), nil)
			if numread > 0 && channels <= maxChannels {
				for i in 0..<Int(numread) {
					for j in 0..<channelCount {
						out[i * channelCount + j] = tempbuf[i * channelCount + chmap[channelCount - 1][j]]
					}
				}
			}
			currentSection = op_current_link(opusRef)
			if numread > 0 {
				total += Int(numread) * channelCount
			}

			if currentSection != lastSection {
				break
			}
		} while total != size && numread != 0

		metadataUpdateCount += total / channelCount
		if metadataUpdateCount >= metadataUpdateInterval {
			metadataUpdateCount -= metadataUpdateInterval
			updateIcyMetadata()
		}

		let streamTimestamp = Double(frame) / 48000.0
		frame += total / channelCount

		let chunk = AudioChunk(properties: properties()!)
		chunk.streamTimestamp = streamTimestamp
		chunk.assignSamples(buffer, frameCount: total / channelCount)

		return chunk
	}

	func close() {
		op_free(opusRef)
		opusRef = nil
	}

	func interrupt() {
		if let s = source as? NSObject, s.responds(to: Selector(("interrupt"))) {
			s.perform(Selector(("interrupt")))
		}
	}

	deinit {
		close()
	}

	func seek(_ frame: Int) -> Int {
		guard let opusRef else { return -1 }
		if op_pcm_seek(opusRef, opus_int64(frame)) != 0 {
			// Refused (an unseekable stream, or a read error): the decoder is
			// still where it was, so say so rather than claim the new position.
			return -1
		}

		self.frame = frame

		return frame
	}

	@objc func properties() -> [AnyHashable: Any]? {
		return ["channels": channels,
		        "bitsPerSample": 32,
		        "floatingPoint": true,
		        "sampleRate": 48000,
		        "totalFrames": totalFrames,
		        "bitrate": bitrate,
		        "seekable": (source?.seekable() ?? false) && seekable,
		        "replaygain_album_gain": replayGainAlbumGain,
		        "replaygain_track_gain": replayGainTrackGain,
		        "codec": "Opus",
		        "endian": "host",
		        "encoding": "lossy"]
	}

	@objc func metadata() -> [AnyHashable: Any]? {
		let dict1: NSDictionary = ["albumArt": albumArt]
		let dict2 = (dict1.merging(with: (metaDict as? [AnyHashable: Any]) ?? [:]) ?? [:]) as NSDictionary
		let dict3 = dict2.merging(with: (icyMetaDict as? [AnyHashable: Any]) ?? [:]) ?? [:]
		return dict3
	}

	@objc class func fileTypes() -> [Any] {
		return ["opus", "ogg"]
	}

	@objc class func mimeTypes() -> [Any] {
		return ["audio/x-opus+ogg", "application/ogg"]
	}

	// We want this to take priority over both Core Audio and FFmpeg
	@objc class func priority() -> Float {
		return 2.0
	}

	@objc class func fileTypeAssociations() -> [Any] {
		return [
			["Opus Audio File", "ogg.icns", "opus"],
			["Ogg Audio File", "ogg.icns", "ogg"]
		]
	}
}
