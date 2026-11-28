//
//  FlacDecoder.swift
//  Flac
//
//  Created by Vincent Spader on 1/25/05.
//  Copyright 2005 Vincent Spader. All rights reserved.
//

import CogAudio
import Foundation

// MARK: - libFLAC callbacks

private func readCallback(_ decoder: UnsafePointer<FLAC__StreamDecoder>?, _ buffer: UnsafeMutablePointer<FLAC__byte>?, _ bytes: UnsafeMutablePointer<Int>?, _ clientData: UnsafeMutableRawPointer?) -> FLAC__StreamDecoderReadStatus {
	let flac = Unmanaged<FlacDecoder>.fromOpaque(clientData!).takeUnretainedValue()
	let bytesRead = flac.source?.read(buffer, amount: bytes!.pointee) ?? 0

	if bytesRead < 0 {
		bytes!.pointee = 0
		return FLAC__STREAM_DECODER_READ_STATUS_ABORT
	} else if bytesRead == 0 {
		bytes!.pointee = 0
		flac.endOfStream = true
		return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM
	} else {
		bytes!.pointee = bytesRead
		return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE
	}
}

private func seekCallback(_ decoder: UnsafePointer<FLAC__StreamDecoder>?, _ absoluteByteOffset: FLAC__uint64, _ clientData: UnsafeMutableRawPointer?) -> FLAC__StreamDecoderSeekStatus {
	let flac = Unmanaged<FlacDecoder>.fromOpaque(clientData!).takeUnretainedValue()

	if flac.source?.seek(Int(absoluteByteOffset), whence: SEEK_SET) == true {
		flac.endOfStream = false
		return FLAC__STREAM_DECODER_SEEK_STATUS_OK
	}
	return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR
}

private func tellCallback(_ decoder: UnsafePointer<FLAC__StreamDecoder>?, _ absoluteByteOffset: UnsafeMutablePointer<FLAC__uint64>?, _ clientData: UnsafeMutableRawPointer?) -> FLAC__StreamDecoderTellStatus {
	let flac = Unmanaged<FlacDecoder>.fromOpaque(clientData!).takeUnretainedValue()

	let pos = flac.source?.tell() ?? -1
	if pos < 0 {
		return FLAC__STREAM_DECODER_TELL_STATUS_ERROR
	}
	absoluteByteOffset!.pointee = FLAC__uint64(pos)
	return FLAC__STREAM_DECODER_TELL_STATUS_OK
}

private func eofCallback(_ decoder: UnsafePointer<FLAC__StreamDecoder>?, _ clientData: UnsafeMutableRawPointer?) -> FLAC__bool {
	let flac = Unmanaged<FlacDecoder>.fromOpaque(clientData!).takeUnretainedValue()
	return flac.endOfStream ? 1 : 0
}

private func lengthCallback(_ decoder: UnsafePointer<FLAC__StreamDecoder>?, _ streamLength: UnsafeMutablePointer<FLAC__uint64>?, _ clientData: UnsafeMutableRawPointer?) -> FLAC__StreamDecoderLengthStatus {
	let flac = Unmanaged<FlacDecoder>.fromOpaque(clientData!).takeUnretainedValue()

	if flac.source?.seekable() == true {
		let currentPos = flac.source?.tell() ?? 0
		flac.source?.seek(0, whence: SEEK_END)
		streamLength!.pointee = FLAC__uint64(flac.source?.tell() ?? 0)
		flac.source?.seek(currentPos, whence: SEEK_SET)
		return FLAC__STREAM_DECODER_LENGTH_STATUS_OK
	}
	streamLength!.pointee = 0
	return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR
}

private func writeCallback(_ decoder: UnsafePointer<FLAC__StreamDecoder>?, _ frame: UnsafePointer<FLAC__Frame>?, _ sampleBuffer: UnsafePointer<UnsafePointer<FLAC__int32>?>?, _ clientData: UnsafeMutableRawPointer?) -> FLAC__StreamDecoderWriteStatus {
	let flac = Unmanaged<FlacDecoder>.fromOpaque(clientData!).takeUnretainedValue()

	if flac.abortFlag {
		return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT
	}

	let f = frame!.pointee
	let channels = Int(f.header.channels)
	let bitsPerSample = Int(f.header.bits_per_sample)
	let frequency = Int(f.header.sample_rate)

	if channels != flac.channels ||
		bitsPerSample != flac.bitsPerSample ||
		frequency != Int(flac.frequency) {
		if channels != flac.channels {
			flac.channelConfig = 0
		}
		flac.channels = channels
		flac.bitsPerSample = bitsPerSample
		flac.frequency = Float(frequency)
		flac.willChangeValue(forKey: "properties")
		flac.didChangeValue(forKey: "properties")
	}

	let blockBuffer = flac.blockBuffer!
	let blocksize = Int(f.header.blocksize)

	switch f.header.bits_per_sample {
	case 8:
		// Interleave the audio (no need for byte swapping)
		let out = blockBuffer.assumingMemoryBound(to: Int8.self)
		var o = 0
		for sample in 0..<blocksize {
			for channel in 0..<channels {
				out[o] = Int8(truncatingIfNeeded: sampleBuffer![channel]![sample])
				o += 1
			}
		}

	case 16:
		// Interleave the audio, converting to big endian byte order
		let out = blockBuffer.assumingMemoryBound(to: Int16.self)
		var o = 0
		for sample in 0..<blocksize {
			for channel in 0..<channels {
				out[o] = Int16(bitPattern: CFSwapInt16HostToBig(UInt16(truncatingIfNeeded: sampleBuffer![channel]![sample])))
				o += 1
			}
		}

	case 24:
		// Interleave the audio (no need for byte swapping)
		let out = blockBuffer.assumingMemoryBound(to: Int8.self)
		var o = 0
		for sample in 0..<blocksize {
			for channel in 0..<channels {
				let audioSample = sampleBuffer![channel]![sample]
				out[o] = Int8(truncatingIfNeeded: audioSample >> 16)
				o += 1
				out[o] = Int8(truncatingIfNeeded: audioSample >> 8)
				o += 1
				out[o] = Int8(truncatingIfNeeded: audioSample)
				o += 1
			}
		}

	case 32:
		// Interleave the audio, converting to big endian byte order
		let out = blockBuffer.assumingMemoryBound(to: Int32.self)
		var o = 0
		for sample in 0..<blocksize {
			for channel in 0..<channels {
				out[o] = Int32(bitPattern: CFSwapInt32HostToBig(UInt32(bitPattern: sampleBuffer![channel]![sample])))
				o += 1
			}
		}
		fallthrough

	default:
		// Time for some nearest byte padding up to 32
		let out = blockBuffer.assumingMemoryBound(to: UInt8.self)
		var o = 0
		let sampleSize = Int(f.header.bits_per_sample)
		for sample in 0..<blocksize {
			for channel in 0..<channels {
				let sampleExtended = sampleBuffer![channel]![sample]
				var sampleBit = sampleSize - 8
				while sampleBit >= -8 {
					if sampleBit >= 0 {
						out[o] = UInt8(truncatingIfNeeded: (sampleExtended >> sampleBit) & 0xFF)
					} else {
						out[o] = UInt8(truncatingIfNeeded: (sampleExtended << -sampleBit) & 0xFF)
					}
					o += 1
					sampleBit -= 8
				}
			}
		}
	}

	flac.blockBufferFrames = Int32(blocksize)

	return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE
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

// This callback is only called for STREAMINFO blocks
private func metadataCallback(_ decoder: UnsafePointer<FLAC__StreamDecoder>?, _ metadata: UnsafePointer<FLAC__StreamMetadata>?, _ clientData: UnsafeMutableRawPointer?) {
	// Some flacs observed in the wild have multiple STREAMINFO metadata blocks,
	// of which only first one has sane values, so only use values from the first STREAMINFO
	// to determine stream format (this seems to be consistent with flac spec: http://flac.sourceforge.net/format.html)
	let flac = Unmanaged<FlacDecoder>.fromOpaque(clientData!).takeUnretainedValue()
	let md = metadata!.pointee

	if !flac.hasStreamInfo && md.type == FLAC__METADATA_TYPE_STREAMINFO {
		flac.channels = Int(md.data.stream_info.channels)
		flac.channelConfig = 0
		flac.frequency = Float(md.data.stream_info.sample_rate)
		flac.bitsPerSample = Int(md.data.stream_info.bits_per_sample)
		flac.totalFrames = Int(md.data.stream_info.total_samples)
		flac.hasStreamInfo = true
	}

	if md.type == FLAC__METADATA_TYPE_CUESHEET && !flac.cuesheetFound {
		flac.cuesheetFound = true

		autoreleasepool {
			var emitted: NSString?
			let fileName = (flac.source?.url()?.path as NSString?)?.lastPathComponent ?? ""
			grabbag__cuesheet_emit(&emitted, metadata, "\"\(fileName)\"")
			let newCuesheet = emitted as String?

			if newCuesheet != flac.cuesheet {
				flac.cuesheet = newCuesheet
				if flac.source?.seekable() != true {
					flac.willChangeValue(forKey: "metadata")
					flac.didChangeValue(forKey: "metadata")
				}
			}
		}
	}

	if md.type == FLAC__METADATA_TYPE_PICTURE {
		let albumArt = Data(bytes: md.data.picture.data, count: Int(md.data.picture.data_length))
		if albumArt != flac.albumArt {
			flac.albumArt = albumArt
			if flac.source?.seekable() != true {
				flac.willChangeValue(forKey: "metadata")
				flac.didChangeValue(forKey: "metadata")
			}
		}
	}

	if md.type == FLAC__METADATA_TYPE_VORBIS_COMMENT {
		let newMetaDict = NSMutableDictionary()
		var newCuesheet = flac.cuesheet
		let vorbisComment = md.data.vorbis_comment
		for i in 0..<Int(vorbisComment.num_comments) {
			var name: UnsafeMutablePointer<CChar>?
			var value: UnsafeMutablePointer<CChar>?
			if FLAC__metadata_object_vorbiscomment_entry_to_name_value_pair(vorbisComment.comments![i], &name, &value) != 0 {
				autoreleasepool {
					var nameString = (guess_encoding_of_string(name) as String?) ?? ""
					var valueString = (guess_encoding_of_string(value) as String?) ?? ""
					free(name)
					free(value)
					nameString = nameString.lowercased()
					nameString = flac.dataStore?.coalesceString(nameString) ?? nameString
					valueString = flac.dataStore?.coalesceString(valueString) ?? valueString
					if nameString == "cuesheet" {
						newCuesheet = valueString
						flac.cuesheetFound = true
					} else if nameString == "waveformatextensible_channel_mask" {
						if valueString.hasPrefix("0x") {
							flac.channelConfig = UInt32(strtoul(String(valueString.dropFirst(2)), nil, 16))
						}
					} else if nameString == "unsynced lyrics" ||
								nameString == "lyrics" {
						setDictionary(newMetaDict, "unsyncedlyrics", valueString)
					} else if nameString == "comments:itunnorm" {
						setDictionary(newMetaDict, "soundcheck", valueString)
					} else {
						setDictionary(newMetaDict, nameString, valueString)
					}
				}
			}
		}

		flac.hasVorbisComment = true

		let newMeta = newMetaDict as NSDictionary
		if !newMeta.isEqual(flac.metaDict) || newCuesheet != flac.cuesheet {
			flac.metaDict = newMeta
			flac.cuesheet = newCuesheet

			if flac.source?.seekable() != true {
				flac.willChangeValue(forKey: "metadata")
				flac.didChangeValue(forKey: "metadata")
			}
		}
	}
}

private func errorCallback(_ decoder: UnsafePointer<FLAC__StreamDecoder>?, _ status: FLAC__StreamDecoderErrorStatus, _ clientData: UnsafeMutableRawPointer?) {
	let flac = Unmanaged<FlacDecoder>.fromOpaque(clientData!).takeUnretainedValue()
	if status != FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC {
		flac.abortFlag = true
	}
}

// MARK: - Decoder

@objc(FlacDecoder)
class FlacDecoder: NSObject, CogDecoder {
	fileprivate var decoder: UnsafeMutablePointer<FLAC__StreamDecoder>?
	fileprivate var blockBuffer: UnsafeMutableRawPointer?
	fileprivate var blockBufferFrames: Int32 = 0

	fileprivate var source: CogSource?

	fileprivate var endOfStream = false

	fileprivate var bitsPerSample = 0
	fileprivate var channels = 0
	fileprivate var channelConfig: UInt32 = 0
	fileprivate var frequency: Float = 0
	fileprivate var totalFrames = 0
	fileprivate var frame = 0
	fileprivate var seconds: Double = 0

	fileprivate var fileSize = 0

	fileprivate var hasStreamInfo = false
	fileprivate var abortFlag = false
	fileprivate var hasVorbisComment = false

	fileprivate var metaDict: NSDictionary = [:]
	fileprivate var icyMetaDict: NSDictionary = [:]

	fileprivate var albumArt = Data()

	fileprivate var cuesheetFound = false
	fileprivate var cuesheet: String? = ""

	fileprivate var dataStore: RedundantPlaylistDataStore?

	func open(_ s: CogSource) -> Bool {
		source = s
		fileSize = 0

		if s.seekable() {
			s.seek(0, whence: SEEK_END)
			fileSize = s.tell()
			s.seek(0, whence: SEEK_SET)
		}

		// Must peek at stream! HTTP reader supports seeking within its buffer
		var buffer = [UInt8](repeating: 0, count: 4)
		s.read(&buffer, amount: 4)
		s.seek(0, whence: SEEK_SET)
		let isOggFlac = buffer == [0x4F, 0x67, 0x67, 0x53] // "OggS"

		metaDict = [:]
		icyMetaDict = [:]
		albumArt = Data()
		cuesheetFound = false
		cuesheet = ""

		dataStore = RedundantPlaylistDataStore()

		guard let newDecoder = FLAC__stream_decoder_new() else {
			return false
		}
		decoder = newDecoder

		if !s.seekable() {
			FLAC__stream_decoder_set_md5_checking(newDecoder, 0)
		}

		FLAC__stream_decoder_set_metadata_ignore_all(newDecoder)
		FLAC__stream_decoder_set_metadata_respond(newDecoder, FLAC__METADATA_TYPE_STREAMINFO)
		FLAC__stream_decoder_set_metadata_respond(newDecoder, FLAC__METADATA_TYPE_VORBIS_COMMENT)
		FLAC__stream_decoder_set_metadata_respond(newDecoder, FLAC__METADATA_TYPE_PICTURE)
		FLAC__stream_decoder_set_metadata_respond(newDecoder, FLAC__METADATA_TYPE_CUESHEET)

		abortFlag = false

		let seekable = s.seekable()
		let clientData = Unmanaged.passUnretained(self).toOpaque()
		let seekCB: FLAC__StreamDecoderSeekCallback? = seekable ? seekCallback : nil
		let tellCB: FLAC__StreamDecoderTellCallback? = seekable ? tellCallback : nil
		let lengthCB: FLAC__StreamDecoderLengthCallback? = seekable ? lengthCallback : nil
		let eofCB: FLAC__StreamDecoderEofCallback? = seekable ? eofCallback : nil
		let ret: FLAC__StreamDecoderInitStatus

		if isOggFlac {
			FLAC__stream_decoder_set_decode_chained_stream(newDecoder, 1)

			ret = FLAC__stream_decoder_init_ogg_stream(newDecoder,
			                                           readCallback,
			                                           seekCB,
			                                           tellCB,
			                                           lengthCB,
			                                           eofCB,
			                                           writeCallback,
			                                           metadataCallback,
			                                           errorCallback,
			                                           clientData)
		} else {
			ret = FLAC__stream_decoder_init_stream(newDecoder,
			                                       readCallback,
			                                       seekCB,
			                                       tellCB,
			                                       lengthCB,
			                                       eofCB,
			                                       writeCallback,
			                                       metadataCallback,
			                                       errorCallback,
			                                       clientData)
		}

		if ret != FLAC__STREAM_DECODER_INIT_STATUS_OK {
			return false
		}

		FLAC__stream_decoder_process_until_end_of_metadata(newDecoder)

		if hasStreamInfo {
			willChangeValue(forKey: "properties")
			didChangeValue(forKey: "properties")
		}

		blockBuffer = malloc(FlacBlockBufferSize)

		frame = 0
		seconds = 0.0

		return true
	}

	func readAudio() -> AudioChunk? {
		var chunk: AudioChunk?

		while blockBufferFrames <= 0 {
			let state = FLAC__stream_decoder_get_state(decoder)
			if state == FLAC__STREAM_DECODER_END_OF_STREAM {
				return nil
			} else if state == FLAC__STREAM_DECODER_END_OF_LINK {
				if FLAC__stream_decoder_finish_link(decoder) == 0 {
					return nil
				}
			}

			if FLAC__stream_decoder_process_single(decoder) == 0 {
				return nil
			}
		}

		if blockBufferFrames > 0 {
			let newChunk = AudioChunk(properties: properties()!)

			frame += Int(blockBufferFrames)
			newChunk.streamTimestamp = seconds

			newChunk.assignSamples(blockBuffer!, frameCount: Int(blockBufferFrames))

			seconds += newChunk.duration()

			blockBufferFrames = 0
			chunk = newChunk
		}

		if source?.seekable() != true && !hasVorbisComment {
			if let httpSource = source as? CogHTTPSource {
				let newIcyMetaDict = NSMutableDictionary()
				if httpSource.hasMetadata() {
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
					let newIcy = newIcyMetaDict as NSDictionary
					if !newIcy.isEqual(icyMetaDict) {
						icyMetaDict = newIcy
						willChangeValue(forKey: "metadata")
						didChangeValue(forKey: "metadata")
					}
				}
			}
		}

		return chunk
	}

	func close() {
		if let decoder {
			FLAC__stream_decoder_finish(decoder)
			FLAC__stream_decoder_delete(decoder)
		}
		if let blockBuffer {
			free(blockBuffer)
		}

		decoder = nil
		blockBuffer = nil
	}

	func interrupt() {
		if let s = source as? NSObject, s.responds(to: Selector(("interrupt"))) {
			s.perform(Selector(("interrupt")))
		}
	}

	deinit {
		close()
	}

	func seek(_ sample: Int) -> Int {
		guard sample >= 0, FLAC__stream_decoder_seek_absolute(decoder, FLAC__uint64(sample)) != 0 else {
			return -1
		}

		frame = sample
		seconds = Double(sample) / Double(frequency)

		return sample
	}

	@objc func properties() -> [AnyHashable: Any]? {
		let bitrate: Float = fileSize != 0 ? Float(fileSize) * 8 / ((Float(totalFrames) + frequency / 2) / frequency) / 1000 : 0
		return ["channels": channels,
		        "channelConfig": channelConfig,
		        "bitsPerSample": bitsPerSample,
		        "sampleRate": frequency,
		        "totalFrames": totalFrames,
		        "seekable": source?.seekable() ?? false,
		        "bitrate": bitrate,
		        "codec": "FLAC",
		        "endian": "big",
		        "encoding": "lossless"]
	}

	@objc func metadata() -> [AnyHashable: Any]? {
		let dict1: NSDictionary = ["albumArt": albumArt, "cuesheet": cuesheet ?? ""]
		let dict2 = (dict1.merging(with: (metaDict as? [AnyHashable: Any]) ?? [:]) ?? [:]) as NSDictionary
		let dict3 = dict2.merging(with: (icyMetaDict as? [AnyHashable: Any]) ?? [:]) ?? [:]
		return dict3
	}

	@objc class func fileTypes() -> [Any] {
		return ["flac"]
	}

	@objc class func mimeTypes() -> [Any] {
		return ["audio/x-flac", "application/ogg", "audio/ogg"]
	}

	@objc class func priority() -> Float {
		return 2.0
	}

	@objc class func fileTypeAssociations() -> [Any] {
		return [
			["FLAC Audio File", "flac.icns", "flac"]
		]
	}
}
