//
//  FFMPEGContainer.swift
//  FFMPEG
//
//  Created by Christopher Snowhill on 6/25/22.
//  Copyright 2022 Christopher Snowhill. All rights reserved.
//

import CogAudio
import Foundation

@objc(FFMPEGContainer)
class FFMPEGContainer: NSObject, CogContainer {
	@objc class func fileTypes() -> [Any] {
		return FFMPEGDecoder.fileTypes()
	}

	@objc class func mimeTypes() -> [Any] {
		return FFMPEGDecoder.fileTypes()
	}

	@objc class func priority() -> Float {
		return FFMPEGDecoder.priority()
	}

	// Whether the loader reads each track of `url` from this plugin's decoder
	// alone: no properties or metadata reader for the file, and no decoder that
	// would be tried ahead of ours.
	private static func decoderAloneReadsURL(_ url: URL, source: CogSource) -> Bool {
		guard let pcClass = NSClassFromString("PluginController") as? CogPluginController.Type,
		      let pluginController = pcClass.shared() else {
			return false
		}
		let ext = url.pathExtension.lowercased()

		if pluginController.metadataReaders()[ext] != nil {
			return false
		}
		if pluginController.propertiesReadersByExtension()[ext] != nil {
			return false
		}
		if pluginController.propertiesReadersByMimeType()[source.mimeType().lowercased()] != nil {
			return false
		}

		let decoders = pluginController.decodersByExtension()[ext] as? [String] ?? []
		if !decoders.contains(NSStringFromClass(FFMPEGDecoder.self)) {
			return false
		}
		for classString in decoders {
			if let decoder = NSClassFromString(classString) as? CogDecoder.Type,
			   decoder != FFMPEGDecoder.self && decoder.priority() >= FFMPEGDecoder.priority() {
				return false
			}
		}

		return true
	}

	private static func hasContent(_ value: Any?) -> Bool {
		if let string = value as? String {
			return !string.isEmpty
		}
		if let array = value as? [Any] {
			for item in array {
				if let string = item as? String, !string.isEmpty {
					return true
				}
			}
		}
		return false
	}

	// Every track's properties and tags, as the decoder gives them opened on
	// each track's URL, from the format context the tracks were found in.
	private static func trackInfoForTracks(_ tracks: [URL], formatContext: UnsafeMutablePointer<AVFormatContext>) -> NSDictionary? {
		let info = NSMutableDictionary(capacity: tracks.count)
		var i = 0
		for track in tracks {
			let decoder = FFMPEGDecoder()
			if decoder.openTrack(Int32(i), ofFormatContext: formatContext) {
				let properties = decoder.properties() ?? [:]
				let entryInfo = ((properties as NSDictionary).merging(with: decoder.metadata() ?? [:]) ?? [:]) as NSDictionary

				// A cue sheet in the file's tags makes the loader read its
				// subsongs as cue sheet tracks, which merge differently.
				if i == 0 && tracks.count > 1 && hasContent(entryInfo["cuesheet"]) {
					decoder.close()
					return nil
				}

				// A later track of no length cannot seek to its start, so its
				// decoder reads on from where the last one stopped.
				if i == 0 || (properties["totalFrames"] as? NSNumber)?.intValue != 0 {
					info[track] = entryInfo
				}
			} else if i == 0 && tracks.count > 1 {
				// A first track that will not open cannot show whether the
				// file has a cue sheet either.
				decoder.close()
				return nil
			}
			decoder.close()
			i += 1
		}
		return info
	}

	@objc class func urls(forContainerURL url: URL) -> [Any] {
		return urls(forContainerURL: url, trackInfo: nil)
	}

	@objc class func urls(forContainerURL url: URL, trackInfo: AutoreleasingUnsafeMutablePointer<NSDictionary?>?) -> [Any] {
		var tracks: [URL] = []
		var formatCtx: UnsafeMutablePointer<AVFormatContext>?
		var ioCtx: UnsafeMutablePointer<AVIOContext>?
		var buffer: UnsafeMutablePointer<UInt8>?
		var source: CogSource?
		var reader: FFMPEGReader?

		defer {
			if formatCtx != nil {
				avformat_close_input(&formatCtx)
			}
			if ioCtx != nil {
				buffer = ioCtx!.pointee.buffer
				avio_context_free(&ioCtx)
			}
			if buffer != nil {
				av_freep(&buffer)
			}
			source?.close()
		}

		if url.fragment != nil {
			// input url already has fragment defined - no need to expand further
			return [url]
		}

		source = AudioSource.audioSource(for: url)

		if source == nil || !source!.open(url) {
			return []
		}

		let scheme = url.scheme ?? ""
		if (scheme == "http" || scheme == "https") && url.pathExtension == "m3u8" {
			source?.close()
			source = nil

			formatCtx = avformat_alloc_context()
			guard let fmt = formatCtx else {
				return []
			}

			if avformat_open_input(&formatCtx, url.absoluteString, nil, nil) < 0 {
				return []
			}
			_ = fmt
		} else {
			buffer = av_malloc(32 * 1024)?.assumingMemoryBound(to: UInt8.self)
			guard let buffer else {
				return []
			}

			reader = FFMPEGReader(file: source!)
			ioCtx = avio_alloc_context(buffer, 32 * 1024, 0, Unmanaged.passUnretained(reader!).toOpaque(), ffmpeg_read, ffmpeg_write, ffmpeg_seek)
			guard let ioCtx else {
				return []
			}

			formatCtx = avformat_alloc_context()
			guard let fmt = formatCtx else {
				return []
			}

			fmt.pointee.pb = ioCtx

			if avformat_open_input(&formatCtx, "", nil, nil) < 0 {
				return []
			}
		}

		guard let formatCtx else { return [] }

		if avformat_find_stream_info(formatCtx, nil) < 0 {
			return []
		}

		var streamIndex = -1
		for i in 0..<Int(formatCtx.pointee.nb_streams) {
			guard let stream = formatCtx.pointee.streams![i] else { continue }
			let codecPar = stream.pointee.codecpar!
			if streamIndex < 0 && codecPar.pointee.codec_type == AVMEDIA_TYPE_AUDIO {
				streamIndex = i
			} else if codecPar.pointee.codec_id == AV_CODEC_ID_TIMED_ID3 {
				// metadata stream
			} else if (stream.pointee.disposition & AV_DISPOSITION_ATTACHED_PIC) != 0 {
				// attached picture
			} else {
				stream.pointee.discard = AVDISCARD_ALL
			}
		}

		if streamIndex < 0 {
			return []
		}

		let subsongs = Int(formatCtx.pointee.nb_chapters)

		if subsongs <= 1 {
			tracks.append(url)
		} else {
			for i in 0..<subsongs {
				if let trackURL = URL(string: url.absoluteString + "#\(i)") {
					tracks.append(trackURL)
				}
			}
		}

		// Only from a local file opened as the decoder opens one: the loader
		// reads no properties from network streams.
		if let trackInfo, source?.seekable() == true, url.isFileURL, decoderAloneReadsURL(url, source: source!) {
			trackInfo.pointee = trackInfoForTracks(tracks, formatContext: formatCtx)
		}

		return tracks
	}
}
