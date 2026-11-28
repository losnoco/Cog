//
//  VorbisDecoder.swift
//  Vorbis
//
//  Created by Vincent Spader on 1/22/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
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

private func sourceRead(_ buf: UnsafeMutableRawPointer?, _ size: Int, _ nmemb: Int, _ datasource: UnsafeMutableRawPointer?) -> Int {
	let source = Unmanaged<AnyObject>.fromOpaque(datasource!).takeUnretainedValue() as! CogSource
	return source.read(buf, amount: size * nmemb)
}

private func sourceSeek(_ datasource: UnsafeMutableRawPointer?, _ offset: ogg_int64_t, _ whence: Int32) -> Int32 {
	let source = Unmanaged<AnyObject>.fromOpaque(datasource!).takeUnretainedValue() as! CogSource
	return source.seek(Int(offset), whence: whence) ? 0 : -1
}

private func sourceClose(_ datasource: UnsafeMutableRawPointer?) -> Int32 {
	0
}

private func sourceTell(_ datasource: UnsafeMutableRawPointer?) -> Int {
	let source = Unmanaged<AnyObject>.fromOpaque(datasource!).takeUnretainedValue() as! CogSource
	return source.tell()
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

@objc(VorbisDecoder)
class VorbisDecoder: NSObject, CogDecoder {
	private var source: CogSource?

	private var vorbisRef = OggVorbis_File()
	private var currentSection: Int32 = 0
	private var lastSection: Int32 = 0

	private var seekable = false
	private var bitrate: Int32 = 0
	private var channels: Int32 = 0
	private var frequency: Float = 0
	private var totalFrames = 0
	private var frame = 0

	private var metadataUpdateInterval = 0
	private var metadataUpdateCount = 0

	private var metaDict: NSDictionary = [:]
	private var icyMetaDict: NSDictionary = [:]

	private var albumArt = Data()

	func open(_ s: CogSource) -> Bool {
		source = s

		let callbacks = ov_callbacks(read_func: sourceRead, seek_func: sourceSeek, close_func: sourceClose, tell_func: sourceTell)

		if ov_open_callbacks(Unmanaged.passUnretained(s as AnyObject).toOpaque(), &vorbisRef, nil, 0, callbacks) != 0 {
			return false
		}

		guard let vi = ov_info(&vorbisRef, -1) else {
			return false
		}

		bitrate = Int32(Double(vi.pointee.bitrate_nominal) / 1000.0)
		channels = vi.pointee.channels
		frequency = Float(vi.pointee.rate)

		seekable = ov_seekable(&vorbisRef) != 0

		totalFrames = Int(ov_pcm_total(&vorbisRef, -1))
		frame = 0

		willChangeValue(forKey: "properties")
		didChangeValue(forKey: "properties")

		metaDict = [:]
		icyMetaDict = [:]
		albumArt = Data()

		updateMetadata()

		metadataUpdateInterval = Int(frequency)
		metadataUpdateCount = 0

		return true
	}

	private func updateMetadata() {
		guard let tags = ov_comment(&vorbisRef, -1) else { return }
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
						if let picture = flac_picture_parse_from_base64(value) {
							if let binary = picture.pointee.binary, picture.pointee.binary_length > 0 {
								newAlbumArt = Data(bytes: binary, count: picture.pointee.binary_length)
							}
							flac_picture_free(picture)
						}
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
		var numread: Int32 = 0
		var total = 0
		let frames = 1024

		let streamTimestamp = Double(frame) / Double(frequency)

		if currentSection != lastSection {
			if let vi = ov_info(&vorbisRef, -1) {
				bitrate = Int32(Double(vi.pointee.bitrate_nominal) / 1000.0)
				channels = vi.pointee.channels
				frequency = Float(vi.pointee.rate)
			}

			metadataUpdateInterval = Int(frequency)
			metadataUpdateCount = 0

			willChangeValue(forKey: "properties")
			didChangeValue(forKey: "properties")

			updateMetadata()
		}

		let chunk = AudioChunk(properties: properties()!)

		let channelCount = Int(channels)
		let buffer = UnsafeMutablePointer<Float>.allocate(capacity: frames * channelCount)
		defer { buffer.deallocate() }

		repeat {
			lastSection = currentSection
			var pcm: UnsafeMutablePointer<UnsafeMutablePointer<Float>?>?
			numread = Int32(ov_read_float(&vorbisRef, &pcm, Int32(frames - total), &currentSection))
			if numread > 0 {
				if channels <= maxChannels {
					for i in 0..<channelCount {
						for j in 0..<Int(numread) {
							buffer[(total + j) * channelCount + i] = pcm![chmap[channelCount - 1][i]]![j]
						}
					}
				} else {
					for i in 0..<channelCount {
						for j in 0..<Int(numread) {
							buffer[(total + j) * channelCount + i] = pcm![i]![j]
						}
					}
				}
				total += Int(numread)
			}

			if currentSection != lastSection {
				break
			}
		} while total != frames && numread != 0

		metadataUpdateCount += total
		if metadataUpdateCount >= metadataUpdateInterval {
			metadataUpdateCount -= metadataUpdateInterval
			updateIcyMetadata()
		}

		chunk.streamTimestamp = streamTimestamp
		chunk.assignSamples(buffer, frameCount: total)

		return chunk
	}

	func close() {
		ov_clear(&vorbisRef)
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
		if ov_pcm_seek(&vorbisRef, ogg_int64_t(frame)) != 0 {
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
		        "sampleRate": frequency,
		        "totalFrames": totalFrames,
		        "bitrate": bitrate,
		        "seekable": (source?.seekable() ?? false) && seekable,
		        "codec": "Ogg Vorbis",
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
		return ["ogg"]
	}

	@objc class func mimeTypes() -> [Any] {
		return ["application/ogg", "application/x-ogg", "audio/ogg", "audio/x-vorbis+ogg"]
	}

	@objc class func priority() -> Float {
		// Ahead of the CoreAudio plugin, which also takes Ogg Vorbis now that
		// macOS decodes it: Apple's decoder gets the ends of the stream wrong,
		// fading out the last ~20 ms and misplacing the start, so a looped
		// track pops at the join. At equal priority, which of the two opened a
		// file was down to chance.
		return 2.0
	}

	@objc class func fileTypeAssociations() -> [Any] {
		return [
			["Ogg Vorbis File", "ogg.icns", "ogg"]
		]
	}
}
