//
//  FFMPEGDecoder.swift
//  FFMPEG
//
//  Created by Andre Reffhaug on 2/26/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

import CogAudio
import Foundation

private let stBuff = 2048

@objc(FFMPEGReader)
class FFMPEGReader: NSObject {
	let file: CogSource
	private var cachedSize = false
	private var cachedSizeValue: Int64 = 0

	init(file: CogSource) {
		self.file = file
		super.init()
	}

	var size: Int64 {
		if !cachedSize {
			if file.seekable() {
				let curOffset = file.tell()
				file.seek(0, whence: SEEK_END)
				cachedSizeValue = Int64(file.tell())
				file.seek(Int(curOffset), whence: SEEK_SET)
			} else {
				cachedSizeValue = -1
			}
			cachedSize = true
		}
		return cachedSizeValue
	}
}

func ffmpeg_read(_ opaque: UnsafeMutableRawPointer?, _ buf: UnsafeMutablePointer<UInt8>?, _ bufSize: Int32) -> Int32 {
	let source = Unmanaged<FFMPEGReader>.fromOpaque(opaque!).takeUnretainedValue()
	let sizeRead = source.file.read(buf, amount: Int(bufSize))
	if sizeRead == 0 { return FFMPEGErrorEOF }
	return Int32(sizeRead)
}

func ffmpeg_write(_ opaque: UnsafeMutableRawPointer?, _ buf: UnsafePointer<UInt8>?, _ bufSize: Int32) -> Int32 {
	-1
}

func ffmpeg_seek(_ opaque: UnsafeMutableRawPointer?, _ offset: Int64, _ whence: Int32) -> Int64 {
	let source = Unmanaged<FFMPEGReader>.fromOpaque(opaque!).takeUnretainedValue()
	var seekPos: Int64 = 0
	let file = source.file
	let size = file.seekable() ? source.size : Int64(file.tell())

	switch whence {
	case FFMPEGSeekSize:
		return source.size
	case SEEK_SET:
		seekPos = offset
	case SEEK_CUR:
		seekPos = Int64(file.tell()) + offset
	case SEEK_END:
		seekPos = source.size - offset
	default:
		return -1
	}

	if seekPos < 0 || seekPos > size {
		return -1
	}

	return file.seek(Int(seekPos), whence: SEEK_SET) ? Int64(file.tell()) : -1
}

private let reverseBits: [UInt8] = {
	var table = [UInt8](repeating: 0, count: 0x100)
	var j = 0
	for i in 0..<0x100 {
		table[i] = UInt8(j)
		// "reverse-increment" of j
		var bitmask = 0x80
		while true {
			j ^= bitmask
			if (j & bitmask) != 0 { break }
			if bitmask == 1 { break }
			bitmask >>= 1
		}
	}
	return table
}()

private let ffmpegLogSetup: Void = {
	av_log_set_flags(AV_LOG_SKIP_REPEATED)
	av_log_set_level(AV_LOG_ERROR)
}()

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

private let lossyCodecs: Set<AVCodecID> = [
	AV_CODEC_ID_MP2, AV_CODEC_ID_MP3, AV_CODEC_ID_AAC, AV_CODEC_ID_AC3,
	AV_CODEC_ID_VORBIS, AV_CODEC_ID_DVAUDIO, AV_CODEC_ID_WMAV1, AV_CODEC_ID_WMAV2,
	AV_CODEC_ID_MACE3, AV_CODEC_ID_MACE6, AV_CODEC_ID_VMDAUDIO, AV_CODEC_ID_MP3ADU,
	AV_CODEC_ID_MP3ON4, AV_CODEC_ID_WESTWOOD_SND1, AV_CODEC_ID_GSM, AV_CODEC_ID_QDM2,
	AV_CODEC_ID_COOK, AV_CODEC_ID_TRUESPEECH, AV_CODEC_ID_SMACKAUDIO, AV_CODEC_ID_QCELP,
	AV_CODEC_ID_DSICINAUDIO, AV_CODEC_ID_IMC, AV_CODEC_ID_MUSEPACK7, AV_CODEC_ID_MLP,
	AV_CODEC_ID_GSM_MS, AV_CODEC_ID_ATRAC3, AV_CODEC_ID_NELLYMOSER, AV_CODEC_ID_MUSEPACK8,
	AV_CODEC_ID_SPEEX, AV_CODEC_ID_WMAVOICE, AV_CODEC_ID_WMAPRO, AV_CODEC_ID_ATRAC3P,
	AV_CODEC_ID_EAC3, AV_CODEC_ID_SIPR, AV_CODEC_ID_MP1, AV_CODEC_ID_TWINVQ,
	AV_CODEC_ID_MP4ALS, AV_CODEC_ID_ATRAC1, AV_CODEC_ID_BINKAUDIO_RDFT, AV_CODEC_ID_BINKAUDIO_DCT,
	AV_CODEC_ID_AAC_LATM, AV_CODEC_ID_QDMC, AV_CODEC_ID_CELT, AV_CODEC_ID_G723_1,
	AV_CODEC_ID_G729, AV_CODEC_ID_8SVX_EXP, AV_CODEC_ID_8SVX_FIB, AV_CODEC_ID_BMV_AUDIO,
	AV_CODEC_ID_RALF, AV_CODEC_ID_IAC, AV_CODEC_ID_ILBC, AV_CODEC_ID_OPUS,
	AV_CODEC_ID_COMFORT_NOISE, AV_CODEC_ID_METASOUND, AV_CODEC_ID_PAF_AUDIO, AV_CODEC_ID_ON2AVC,
	AV_CODEC_ID_DSS_SP, AV_CODEC_ID_CODEC2, AV_CODEC_ID_FFWAVESYNTH, AV_CODEC_ID_SONIC,
	AV_CODEC_ID_SONIC_LS, AV_CODEC_ID_EVRC, AV_CODEC_ID_SMV, AV_CODEC_ID_4GV,
	AV_CODEC_ID_INTERPLAY_ACM, AV_CODEC_ID_XMA1, AV_CODEC_ID_XMA2, AV_CODEC_ID_ATRAC3AL,
	AV_CODEC_ID_ATRAC3PAL, AV_CODEC_ID_DOLBY_E, AV_CODEC_ID_APTX, AV_CODEC_ID_SBC,
	AV_CODEC_ID_ATRAC9, AV_CODEC_ID_HCOM, AV_CODEC_ID_ACELP_KELVIN, AV_CODEC_ID_MPEGH_3D_AUDIO,
	AV_CODEC_ID_SIREN, AV_CODEC_ID_HCA, AV_CODEC_ID_FASTAUDIO
]

@objc(FFMPEGDecoder)
class FFMPEGDecoder: NSObject, CogDecoder {
	private var source: CogSource?
	private var reader: FFMPEGReader?
	private var seekable = false
	private var channels: Int32 = 0
	private var channelConfig: UInt32 = 0
	private var bitsPerSample: Int32 = 0
	private var floatingPoint = false
	private var lossy = false
	private var frequency: Float = 0
	private var totalFrames = 0
	private var framesRead = 0
	private var bitrate: Int32 = 0
	private var subsong: Int32 = 0
	private var startTime: Int64 = 0
	private var endTime: Int64 = 0
	private var seekedToStart = false

	private var buffer: UnsafeMutablePointer<UInt8>?
	private var ioCtx: UnsafeMutablePointer<AVIOContext>?
	private var streamIndex: Int32 = 0
	private var formatCtx: UnsafeMutablePointer<AVFormatContext>?
	private var formatCtxLent = false
	private var codecCtx: UnsafeMutablePointer<AVCodecContext>?
	private var lastDecodedFrame: UnsafeMutablePointer<AVFrame>?
	private var lastReadPacket: UnsafeMutablePointer<AVPacket>?

	private var sampleBufferSize = 0
	private var sampleBuffer: UnsafeMutablePointer<UInt8>?

	private var metadataUpdateInterval = 0
	private var metadataUpdateCount = 0

	private var metadataUpdated = false

	private var prebufferedChunk: AudioChunk?

	private var rawDSD = false
	private var rawDSDReverseBits = false
	private var rawDSDPlanar = false

	private var bytesConsumedFromDecodedFrame = 0
	private var readNextPacket = false
	private var seekFrame: Int64 = 0
	private var skipSamples: Int64 = 0
	private var endOfStream = false
	private var endOfAudio = false

	private var metadataIndex: Int32 = 0
	private var attachedPicIndex: Int32 = 0

	private var albumArt = Data()
	private var metaDict: NSDictionary = [:]
	private var id3Metadata: NSDictionary = [:]

	override init() {
		super.init()
		_ = ffmpegLogSetup
		_ = reverseBits
		lastReadPacket = nil
		lastDecodedFrame = nil
		codecCtx = nil
		formatCtx = nil
		ioCtx = nil
		buffer = nil
	}

	func open(_ s: CogSource) -> Bool {
		source = s

		formatCtx = nil
		totalFrames = 0
		framesRead = 0
		rawDSD = false

		let fragment = s.url().fragment ?? ""
		subsong = fragment.isEmpty ? 0 : Int32(Int(fragment) ?? 0)

		guard let newBuffer = av_malloc(32 * 1024)?.assumingMemoryBound(to: UInt8.self) else {
			return false
		}
		buffer = newBuffer

		let newReader = FFMPEGReader(file: s)
		reader = newReader
		seekable = s.seekable()

		let seekFunc: (@convention(c) (UnsafeMutableRawPointer?, Int64, Int32) -> Int64)? = seekable ? ffmpeg_seek : nil
		ioCtx = avio_alloc_context(newBuffer, 32 * 1024, 0, Unmanaged.passUnretained(newReader).toOpaque(), ffmpeg_read, ffmpeg_write, seekFunc)
		guard let ioCtx else {
			return false
		}

		formatCtx = avformat_alloc_context()
		guard let formatCtx else {
			return false
		}

		formatCtx.pointee.pb = ioCtx

		var errcode = avformat_open_input(&self.formatCtx, "", nil, nil)
		if errcode < 0 {
			return false
		}

		errcode = avformat_find_stream_info(self.formatCtx, nil)
		if errcode < 0 {
			return false
		}

		return openStreams()
	}

	// Opens subsong `track` of a seekable file whose format context the caller
	// has opened and probed. The context stays the caller's, and outlives the
	// decoder's use.
	@objc func openTrack(_ track: Int32, ofFormatContext ctx: UnsafeMutablePointer<AVFormatContext>) -> Bool {
		formatCtx = ctx
		formatCtxLent = true
		seekable = true
		subsong = track
		totalFrames = 0
		framesRead = 0
		rawDSD = false

		return openStreams()
	}

	private func openStreams() -> Bool {
		guard let formatCtx else { return false }

		streamIndex = -1
		metadataIndex = -1
		attachedPicIndex = -1

		var pictures: [Int] = []

		for i in 0..<Int(formatCtx.pointee.nb_streams) {
			guard let stream = formatCtx.pointee.streams![i] else { continue }
			let codecPar = stream.pointee.codecpar!
			if streamIndex < 0 && codecPar.pointee.codec_type == AVMEDIA_TYPE_AUDIO {
				streamIndex = Int32(i)
			} else if codecPar.pointee.codec_id == AV_CODEC_ID_TIMED_ID3 {
				metadataIndex = Int32(i)
			} else if (stream.pointee.disposition & AV_DISPOSITION_ATTACHED_PIC) != 0 {
				pictures.append(i)
			} else {
				stream.pointee.discard = AVDISCARD_ALL
			}
		}

		if streamIndex < 0 {
			return false
		}

		if !pictures.isEmpty {
			if pictures.count == 1 {
				attachedPicIndex = Int32(pictures[0])
			} else {
				// Find the first attached picture with "front" or "cover" in its filename
				for picture in pictures {
					let stream = formatCtx.pointee.streams![picture]!
					let metadata = stream.pointee.metadata
					if let filename = av_dict_get(metadata, "filename", nil, 0) {
						if strcasestr(filename.pointee.value, "front") != nil ||
							strcasestr(filename.pointee.value, "cover") != nil {
							attachedPicIndex = Int32(picture)
							break
						}
					}
				}
				if attachedPicIndex < 0 {
					attachedPicIndex = Int32(pictures[0])
				}
			}
		}

		let stream = formatCtx.pointee.streams![Int(streamIndex)]!
		let codecPar = stream.pointee.codecpar!

		codecCtx = avcodec_alloc_context3(nil)
		guard let codecCtx else {
			return false
		}

		var errcode = avcodec_parameters_to_context(codecCtx, codecPar)
		if errcode < 0 {
			return false
		}

		let codecID = codecCtx.pointee.codec_id
		var codec: UnsafePointer<AVCodec>?
		var dict: OpaquePointer?

		switch codecID {
		case AV_CODEC_ID_DSD_LSBF, AV_CODEC_ID_DSD_MSBF, AV_CODEC_ID_DSD_LSBF_PLANAR, AV_CODEC_ID_DSD_MSBF_PLANAR:
			rawDSD = true
			rawDSDReverseBits = codecID == AV_CODEC_ID_DSD_LSBF || codecID == AV_CODEC_ID_DSD_LSBF_PLANAR
			rawDSDPlanar = codecID == AV_CODEC_ID_DSD_LSBF_PLANAR || codecID == AV_CODEC_ID_DSD_MSBF_PLANAR
		case AV_CODEC_ID_MP3:
			codec = avcodec_find_decoder_by_name("mp3float")
		case AV_CODEC_ID_MP2:
			codec = avcodec_find_decoder_by_name("mp2float")
		case AV_CODEC_ID_MP1:
			codec = avcodec_find_decoder_by_name("mp1float")
		case AV_CODEC_ID_AAC:
			codec = avcodec_find_decoder_by_name("libfdk_aac")
			av_dict_set_int(&dict, "drc_level", -2, 0) // disable DRC
			av_dict_set_int(&dict, "level_limit", 0, 0) // disable peak limiting
		case AV_CODEC_ID_ALAC:
			codec = avcodec_find_decoder_by_name("alac")
		case AV_CODEC_ID_AC3:
			codec = avcodec_find_decoder_by_name("ac3")
		case AV_CODEC_ID_EAC3:
			codec = avcodec_find_decoder_by_name("eac3")
		default:
			break
		}

		if codec == nil && !rawDSD {
			codec = avcodec_find_decoder(codecID)
		}

		if codec == nil && !rawDSD {
			av_dict_free(&dict)
			return false
		}

		if !rawDSD {
			errcode = avcodec_open2(codecCtx, codec, &dict)
			if errcode < 0 {
				av_dict_free(&dict)
				return false
			}
		}

		av_dict_free(&dict)

		// Bah, their skipping is broken
		if !rawDSD {
			codecCtx.pointee.flags2 |= AV_CODEC_FLAG2_SKIP_MANUAL
		}

		lastDecodedFrame = av_frame_alloc()
		av_frame_unref(lastDecodedFrame)
		lastReadPacket = UnsafeMutablePointer<AVPacket>.allocate(capacity: 1)
		av_new_packet(lastReadPacket, 0)
		readNextPacket = true
		bytesConsumedFromDecodedFrame = Int(Int32.max)
		seekFrame = -1

		if !rawDSD {
			frequency = Float(codecCtx.pointee.sample_rate)
			channels = codecCtx.pointee.ch_layout.nb_channels
			channelConfig = UInt32(codecCtx.pointee.ch_layout.u.mask)
			floatingPoint = false

			switch codecCtx.pointee.sample_fmt {
			case AV_SAMPLE_FMT_U8, AV_SAMPLE_FMT_U8P:
				bitsPerSample = 8
			case AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_S16P:
				bitsPerSample = 16
			case AV_SAMPLE_FMT_S32, AV_SAMPLE_FMT_S32P:
				bitsPerSample = 32
			case AV_SAMPLE_FMT_FLT, AV_SAMPLE_FMT_FLTP:
				bitsPerSample = 32
				floatingPoint = true
			case AV_SAMPLE_FMT_DBL, AV_SAMPLE_FMT_DBLP:
				bitsPerSample = 64
				floatingPoint = true
			default:
				return false
			}
		} else {
			frequency = Float(codecPar.pointee.sample_rate * 8)
			channels = codecCtx.pointee.ch_layout.nb_channels
			channelConfig = UInt32(codecCtx.pointee.ch_layout.u.mask)
			bitsPerSample = 1
			floatingPoint = false
		}

		lossy = floatingPoint
		if !floatingPoint && lossyCodecs.contains(codecID) {
			lossy = true
		}

		let tb = AVRational(num: 1, den: codecCtx.pointee.sample_rate)
		totalFrames = seekable ? Int(av_rescale_q(formatCtx.pointee.duration, FFMPEGTimeBaseQ, tb)) : 0
		bitrate = Int32(codecCtx.pointee.bit_rate / 1000)
		framesRead = 0
		endOfStream = false
		endOfAudio = false

		sampleBufferSize = max(Int(codecCtx.pointee.sample_rate) / 200, 1024)
		sampleBufferSize *= rawDSD ? Int(channels) : Int(channels) * Int(bitsPerSample / 8)
		sampleBuffer = av_malloc(sampleBufferSize)?.assumingMemoryBound(to: UInt8.self)
		guard let sampleBuffer else {
			return false
		}

		metadataUpdateInterval = Int(codecCtx.pointee.sample_rate)
		metadataUpdateCount = 0

		if rawDSD {
			totalFrames *= 8
		}

		if seekable {
			if stream.pointee.start_time != 0 && stream.pointee.start_time != FFMPEGNoPtsValue {
				skipSamples = av_rescale_q(stream.pointee.start_time, stream.pointee.time_base, tb)
			}
			if skipSamples < 0 {
				skipSamples = 0
			}
		} else {
			skipSamples = 0
		}

		if subsong < formatCtx.pointee.nb_chapters {
			let chapter = formatCtx.pointee.chapters![Int(subsong)]!
			startTime = av_rescale_q(chapter.pointee.start, chapter.pointee.time_base, tb)
			endTime = av_rescale_q(chapter.pointee.end, chapter.pointee.time_base, tb)
			skipSamples = startTime
			totalFrames = Int(endTime - startTime)
		}

		seekFrame = skipSamples // Skip preroll if necessary

		if totalFrames < 0 {
			totalFrames = 0
		}

		seekedToStart = !seekable

		id3Metadata = [:]
		metaDict = [:]
		albumArt = Data()
		metadataUpdated = false
		updateMetadata()

		prebufferedChunk = nil

		if attachedPicIndex >= 0 {
			prebufferedChunk = readAudio()
		}

		return true
	}

	func close() {
		if let lastReadPacket {
			av_packet_unref(lastReadPacket)
			lastReadPacket.deallocate()
			self.lastReadPacket = nil
		}

		if let lastDecodedFrame {
			av_frame_unref(lastDecodedFrame)
			av_freep(&self.lastDecodedFrame)
		}

		if codecCtx != nil {
			avcodec_free_context(&codecCtx)
		}

		if formatCtx != nil {
			if formatCtxLent {
				formatCtx = nil
			} else {
				avformat_close_input(&formatCtx)
			}
		}

		if let ioCtx {
			buffer = ioCtx.pointee.buffer
			avio_context_free(&self.ioCtx)
		}

		if sampleBuffer != nil {
			av_freep(&sampleBuffer)
		}

		if buffer != nil {
			av_freep(&buffer)
		}
	}

	func interrupt() {
		if let s = source as? NSObject, s.responds(to: Selector(("interrupt"))) {
			s.perform(Selector(("interrupt")))
		}
	}

	deinit {
		close()
	}

	private func updateMetadata() {
		guard let formatCtx else { return }
		let newMetaDict = NSMutableDictionary()

		for i in 0..<4 {
			var metadata: OpaquePointer?
			if i == 0 {
				metadata = formatCtx.pointee.metadata
				if metadata == nil { continue }
			} else if i == 1 {
				if formatCtx.pointee.nb_programs > 0 {
					guard let program = formatCtx.pointee.programs![0] else { continue }
					metadata = program.pointee.metadata
					if metadata == nil { continue }
				} else {
					continue
				}
			} else if i == 2 {
				let stream = formatCtx.pointee.streams![Int(streamIndex)]!
				metadata = stream.pointee.metadata
				if metadata == nil { continue }
			} else {
				if subsong < formatCtx.pointee.nb_chapters {
					metadata = formatCtx.pointee.chapters![Int(subsong)]!.pointee.metadata
					if metadata == nil { continue }
				} else {
					break
				}
			}

			var tag: UnsafePointer<AVDictionaryEntry>?
			while let next = av_dict_iterate(metadata, tag) {
				tag = next
				autoreleasepool {
					let key = String(cString: next.pointee.key)
					let value = (guess_encoding_of_string(next.pointee.value) as String?) ?? ""
					if key.caseInsensitiveCompare("streamtitle") == .orderedSame {
						let splitValues = value.components(separatedBy: " - ")
						if splitValues.count > 1 {
							setDictionary(newMetaDict, "artist", splitValues[0])
							setDictionary(newMetaDict, "title", splitValues[1])
						} else {
							setDictionary(newMetaDict, "title", splitValues[0])
						}
					} else if key.caseInsensitiveCompare("unsynced lyrics") == .orderedSame ||
								key.caseInsensitiveCompare("lyrics") == .orderedSame {
						setDictionary(newMetaDict, "unsyncedlyrics", value)
					} else if key.caseInsensitiveCompare("icy-url") == .orderedSame {
						setDictionary(newMetaDict, "album", value)
					} else if key.caseInsensitiveCompare("icy-genre") == .orderedSame {
						setDictionary(newMetaDict, "genre", value)
					} else if key.caseInsensitiveCompare("title") == .orderedSame {
						if i == 0 && formatCtx.pointee.nb_chapters > 1 {
							setDictionary(newMetaDict, "album", value)
						} else {
							setDictionary(newMetaDict, "title", value)
						}
					} else if key.caseInsensitiveCompare("variant_bitrate") == .orderedSame {
						let nbitrate = Int(value) ?? 0
						setDictionary(newMetaDict, "bitrate", String(nbitrate / 1000))
					} else if key.caseInsensitiveCompare("date_recorded") == .orderedSame {
						setDictionary(newMetaDict, "date", value)
					} else if key.caseInsensitiveCompare("replaygain_gain") == .orderedSame {
						setDictionary(newMetaDict, i == 0 ? "replaygain_album_gain" : "replaygain_track_gain", value)
					} else if key.caseInsensitiveCompare("replaygain_peak") == .orderedSame {
						setDictionary(newMetaDict, i == 0 ? "replaygain_album_peak" : "replaygain_track_peak", value)
					} else if key.caseInsensitiveCompare("iTunNORM") == .orderedSame {
						setDictionary(newMetaDict, "soundcheck", value)
					} else {
						let tagKey = ((guess_encoding_of_string(next.pointee.key) as String?) ?? "").lowercased()
						setDictionary(newMetaDict, tagKey, value)
					}
				}
			}
		}

		if let httpSource = source as? CogHTTPSource {
			if httpSource.hasMetadata() {
				let metadata = httpSource.metadata() ?? [:]
				if let genre = metadata["genre"] as? String, !genre.isEmpty {
					newMetaDict["genre"] = [genre]
				}
				if let album = metadata["album"] as? String, !album.isEmpty {
					newMetaDict["album"] = [album]
				}
				if let artist = metadata["artist"] as? String, !artist.isEmpty {
					newMetaDict["artist"] = [artist]
				}
				if let title = metadata["title"] as? String, !title.isEmpty {
					newMetaDict["title"] = [title]
				}
			}
		}

		/* Meh */
		let ignoreKey = "id3v2_priv․com․apple․streaming․transportstreamtimestamp"
		if newMetaDict[ignoreKey] != nil {
			newMetaDict.removeObject(forKey: ignoreKey)
		}

		if !(newMetaDict as NSDictionary).isEqual(metaDict) {
			metaDict = newMetaDict
			if !seekable {
				willChangeValue(forKey: "metadata")
				didChangeValue(forKey: "metadata")
			} else {
				metadataUpdated = true
			}
		}
	}

	private func updateID3Metadata() {
		guard let lastReadPacket else { return }
		let tag = Data(bytes: lastReadPacket.pointee.data, count: Int(lastReadPacket.pointee.size))
		if let tagReader = NSClassFromString("TagLibID3v2Reader") as? NSObject.Type,
		   tagReader.responds(to: Selector(("metadataForTag:"))) {
			let newID3 = tagReader.perform(Selector(("metadataForTag:")), with: tag)?.takeUnretainedValue() as? NSDictionary ?? [:]
			if !newID3.isEqual(id3Metadata) {
				id3Metadata = newID3
				willChangeValue(forKey: "metadata")
				didChangeValue(forKey: "metadata")
			}
		}
	}

	private func updateArtwork() {
		guard let lastReadPacket else { return }
		let newAlbumArt = Data(bytes: lastReadPacket.pointee.data, count: Int(lastReadPacket.pointee.size))
		if newAlbumArt != albumArt {
			albumArt = newAlbumArt
			if !seekable {
				willChangeValue(forKey: "metadata")
				didChangeValue(forKey: "metadata")
			} else {
				metadataUpdated = true
			}
		}
	}

	func readAudio() -> AudioChunk? {
		guard let codecCtx, let formatCtx, let lastDecodedFrame, let lastReadPacket, let sampleBuffer else { return nil }

		if !seekedToStart {
			_ = seek(0)
		}

		if let prebufferedChunk {
			// A bit of ignored read-ahead to support embedded artwork
			self.prebufferedChunk = nil
			return prebufferedChunk
		}

		if totalFrames != 0 && framesRead >= totalFrames {
			return nil
		}

		let frameSize = rawDSD ? Int(channels) : Int(channels) * Int(bitsPerSample / 8)
		let bytesToRead = sampleBufferSize
		var bytesRead = 0

		let buf = sampleBuffer
		var dataSize = 0
		var seekBytesSkip = 0
		var errcode: Int32 = 0

		memset(buf, 0, bytesToRead)

		while bytesRead < bytesToRead {
			var planeSize: Int32 = 0
			var planar: Int32 = 0
			if !rawDSD {
				planar = av_sample_fmt_is_planar(codecCtx.pointee.sample_fmt)
				dataSize = Int(av_samples_get_buffer_size(&planeSize, channels, lastDecodedFrame.pointee.nb_samples, codecCtx.pointee.sample_fmt, 1))
			} else {
				planar = 0
				dataSize = endOfStream ? 0 : Int(lastReadPacket.pointee.size)
			}

			if dataSize < 0 { dataSize = 0 }

			while readNextPacket && !endOfAudio {
				if !endOfStream {
					av_packet_unref(lastReadPacket)
					errcode = av_read_frame(formatCtx, lastReadPacket)
					if errcode < 0 {
						if errcode == FFMPEGErrorEOF {
							endOfStream = true
						}
						if let pb = formatCtx.pointee.pb, pb.pointee.error != 0 { break }
					}

					if lastReadPacket.pointee.stream_index == metadataIndex {
						updateID3Metadata()
						continue
					} else if lastReadPacket.pointee.stream_index == attachedPicIndex {
						updateArtwork()
						continue
					}

					if lastReadPacket.pointee.stream_index != streamIndex {
						continue
					}
				}

				if !rawDSD {
					errcode = avcodec_send_packet(codecCtx, endOfStream ? nil : lastReadPacket)
					if errcode < 0 && errcode == FFMPEGErrorEAGAIN {
						continue
					}
				}

				readNextPacket = false
			}

			if dataSize <= bytesConsumedFromDecodedFrame {
				if endOfStream && endOfAudio { break }

				bytesConsumedFromDecodedFrame = 0

				if !rawDSD {
					errcode = avcodec_receive_frame(codecCtx, lastDecodedFrame)
					if errcode < 0 {
						if errcode == FFMPEGErrorEOF {
							endOfAudio = true
							break
						} else if errcode == FFMPEGErrorEAGAIN {
							readNextPacket = true
							continue
						} else {
							return nil
						}
					}

					dataSize = Int(av_samples_get_buffer_size(&planeSize, channels, lastDecodedFrame.pointee.nb_samples, codecCtx.pointee.sample_fmt, 1))
				} else {
					dataSize = Int(lastReadPacket.pointee.size)
					if endOfStream {
						endOfAudio = true
						break
					} else if dataSize <= bytesConsumedFromDecodedFrame {
						readNextPacket = true
						continue
					}

					if rawDSDPlanar {
						let tempBuf = UnsafeMutablePointer<UInt8>.allocate(capacity: dataSize)
						defer { tempBuf.deallocate() }
						let samples = dataSize / Int(channels)
						let packetData = lastReadPacket.pointee.data!
						for i in 0..<samples {
							for j in 0..<Int(channels) {
								tempBuf[i * Int(channels) + j] = packetData[j * samples + i]
							}
						}
						memmove(packetData, tempBuf, dataSize)
					}

					if rawDSDReverseBits {
						let packetData = lastReadPacket.pointee.data!
						for i in 0..<dataSize {
							packetData[i] = reverseBits[Int(packetData[i])]
						}
					}
				}

				if dataSize < 0 { dataSize = 0 }

				// FFmpeg seeking by packet is usually inexact, so skip up to
				// target sample using packet timestamp
				if seekFrame >= 0 && errcode >= 0 {
					let tb = AVRational(num: 1, den: codecCtx.pointee.sample_rate)
					let packetBeginFrame = av_rescale_q(lastReadPacket.pointee.dts, formatCtx.pointee.streams![Int(streamIndex)]!.pointee.time_base, tb)
					if packetBeginFrame < seekFrame {
						seekBytesSkip += Int((seekFrame - packetBeginFrame) * Int64(frameSize))
					}
					seekFrame = -1
				}

				let minSkipped = min(dataSize, seekBytesSkip)
				bytesConsumedFromDecodedFrame += minSkipped
				seekBytesSkip -= minSkipped
			}

			if !rawDSD {
				let newChannels = codecCtx.pointee.ch_layout.nb_channels
				let newChannelConfig = UInt32(codecCtx.pointee.ch_layout.u.mask)
				let newFrequency = Float(codecCtx.pointee.sample_rate)

				if newChannels != channels || newChannelConfig != channelConfig || newFrequency != frequency {
					if bytesRead > 0 {
						break
					} else {
						channels = newChannels
						channelConfig = newChannelConfig
						frequency = newFrequency
						willChangeValue(forKey: "properties")
						didChangeValue(forKey: "properties")
					}
				}
			}

			let toConsume = min(dataSize - bytesConsumedFromDecodedFrame, bytesToRead - bytesRead)

			// copy decoded samples to Cog's buffer
			if rawDSD {
				memmove(buf + bytesRead, lastReadPacket.pointee.data! + bytesConsumedFromDecodedFrame, toConsume)
			} else if planar == 0 || channels == 1 {
				memmove(buf + bytesRead, lastDecodedFrame.pointee.data.0! + bytesConsumedFromDecodedFrame, toConsume)
			} else {
				var out = buf + bytesRead
				let bytesPerSample = Int(bitsPerSample / 8)
				let bytesConsumedPerPlane = bytesConsumedFromDecodedFrame / Int(channels)
				let toConsumePerPlane = toConsume / Int(channels)
				var s = 0
				while s < toConsumePerPlane {
					for ch in 0..<Int(channels) {
						memcpy(out, lastDecodedFrame.pointee.extended_data![ch]! + bytesConsumedPerPlane + s, bytesPerSample)
						out += bytesPerSample
					}
					s += bytesPerSample
				}
			}

			bytesConsumedFromDecodedFrame += toConsume
			bytesRead += toConsume

			if rawDSD && bytesConsumedFromDecodedFrame == dataSize {
				av_packet_unref(lastReadPacket)
			}
		}

		var framesReadNow = bytesRead / frameSize
		if totalFrames != 0 && framesRead + framesReadNow > totalFrames {
			framesReadNow = totalFrames - framesRead
		}

		let streamTimestamp = Double(framesRead * (rawDSD ? 8 : 1)) / Double(frequency)

		framesRead += framesReadNow

		metadataUpdateCount += framesReadNow
		if metadataUpdateCount >= metadataUpdateInterval {
			metadataUpdateCount -= metadataUpdateInterval
			updateMetadata()
		}

		let chunk = AudioChunk(properties: properties() ?? [:])
		chunk.streamTimestamp = streamTimestamp
		chunk.assignSamples(sampleBuffer, frameCount: framesReadNow)

		return chunk
	}

	func seek(_ frame: Int) -> Int {
		var frame = frame
		guard let codecCtx, let formatCtx else { return -1 }
		if totalFrames == 0 {
			return -1
		}

		seekedToStart = true
		prebufferedChunk = nil

		if frame >= totalFrames {
			framesRead = totalFrames
			endOfStream = true
			endOfAudio = true
			return -1
		}
		if rawDSD { frame /= 8 }
		let tb = AVRational(num: 1, den: codecCtx.pointee.sample_rate)
		let ts = av_rescale_q(Int64(frame + Int(skipSamples)), tb, formatCtx.pointee.streams![Int(streamIndex)]!.pointee.time_base)
		let ret = avformat_seek_file(formatCtx, streamIndex, ts - 1000, ts, ts, 0)
		if !rawDSD {
			avcodec_flush_buffers(codecCtx)
		} else {
			av_packet_unref(lastReadPacket)
		}
		if ret < 0 {
			framesRead = totalFrames
			endOfStream = true
			endOfAudio = true
			return -1
		}
		readNextPacket = true
		bytesConsumedFromDecodedFrame = Int(Int32.max)
		framesRead = frame
		seekFrame = Int64(frame) + skipSamples
		endOfStream = false
		endOfAudio = false

		return rawDSD ? frame * 8 : frame
	}

	@objc func properties() -> [AnyHashable: Any]? {
		guard let codecCtx else { return nil }
		return ["channels": channels,
		        "channelConfig": channelConfig,
		        "bitsPerSample": bitsPerSample,
		        "unSigned": bitsPerSample == 8,
		        "sampleRate": frequency,
		        "floatingPoint": floatingPoint,
		        "totalFrames": totalFrames,
		        "bitrate": bitrate,
		        "seekable": seekable,
		        "codec": (guess_encoding_of_string(avcodec_get_name(codecCtx.pointee.codec_id)) as String?) ?? "",
		        "dsdDoPReverseBits": false,
		        "endian": "host",
		        "encoding": lossy ? "lossy" : "lossless"]
	}

	@objc func metadata() -> [AnyHashable: Any]? {
		let dict1: NSDictionary = ["albumArt": albumArt]
		let dict2 = (dict1.merging(with: (metaDict as? [AnyHashable: Any]) ?? [:]) ?? [:]) as NSDictionary
		let dict3 = dict2.merging(with: (id3Metadata as? [AnyHashable: Any]) ?? [:]) ?? [:]
		return dict3
	}

	@objc class func fileTypes() -> [Any] {
		return ["wma", "asf", "tak", "mp4", "m4a", "m4b", "m4r", "aac", "mp3", "mp2", "m2a", "mpa", "ape", "ac3", "dts", "dtshd", "wav", "tta", "vqf", "vqe", "vql", "ra", "rm", "rmj", "mka", "mkv", "weba", "webm", "dsf", "dff", "iff", "dsdiff", "wsd", "aiff", "aif"]
	}

	@objc class func mimeTypes() -> [Any] {
		return ["application/wma", "application/x-wma", "audio/x-wma", "audio/x-ms-wma", "audio/x-tak", "application/ogg", "audio/aac", "audio/aacp", "audio/mpeg", "audio/mp4", "audio/x-mp3", "audio/x-mp2", "audio/x-matroska", "audio/x-ape", "audio/x-ac3", "audio/x-dts", "audio/x-dtshd", "audio/x-at3", "audio/wav", "audio/tta", "audio/x-tta", "audio/x-twinvq", "application/vnd.apple.mpegurl", "audio/mpegurl"]
	}

	@objc class func fileTypeAssociations() -> [Any] {
		return [
			["Windows Media Audio File", "song.icns", "wma", "asf"],
			["TAK Audio File", "song.icns", "tak"],
			["MPEG-4 Audio File", "m4a.icns", "mp4", "m4a", "m4b", "m4r"],
			["MPEG-4 AAC Audio File", "song.icns", "aac"],
			["MPEG Audio File", "mp3.icns", "mp3", "m2a", "mpa"],
			["Monkey's Audio File", "ape.icns", "ape"],
			["AC-3 Audio File", "song.icns", "ac3"],
			["DTS Audio File", "song.icns", "dts"],
			["DTS-HD MA Audio File", "song.icns", "dtshd"],
			["True Audio File", "song.icns", "tta"],
			["TrueVQ Audio File", "song.icns", "vqf", "vqe", "vql"],
			["Real Audio File", "song.icns", "ra", "rm", "rmj"],
			["Matroska Audio File", "song.icns", "mka"],
			["Matroska Video File", "song.icns", "mkv"],
			["WebM Audio File", "song.icns", "weba"],
			["WebM Media File", "song.icns", "webm"],
			["DSD Stream File", "song.icns", "dsf"],
			["Interchange File Format", "song.icns", "iff", "dsdiff"],
			["Wideband Single-bit Data", "song.icns", "wsd"],
			["Audio Interchange File Format", "aiff.icns", "aiff", "aif"]
		]
	}

	@objc class func priority() -> Float {
		return 1.5
	}
}
