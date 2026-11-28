/*
 *  CoreAudioDecoder.swift
 *
 *  Copyright (C) 2006 Stephen F. Booth <me@sbooth.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

import AudioToolbox
import CogAudio
import Foundation

private func ffatGetChannelID(_ label: AudioChannelLabel) -> Int {
	if label == 0 {
		return -1
	} else if label <= kAudioChannelLabel_LFEScreen {
		return Int(label) - 1
	} else if label <= kAudioChannelLabel_RightSurround {
		return Int(label) + 4
	} else if label <= kAudioChannelLabel_CenterSurround {
		return Int(label) + 1
	} else if label <= kAudioChannelLabel_RightSurroundDirect {
		return Int(label) + 23
	} else if label <= kAudioChannelLabel_TopBackRight {
		return Int(label) - 1
	} else if label < kAudioChannelLabel_RearSurroundLeft {
		return -1
	} else if label <= kAudioChannelLabel_RearSurroundRight {
		return Int(label) - 29
	} else if label <= kAudioChannelLabel_RightWide {
		return Int(label) - 4
	} else if label == kAudioChannelLabel_LFE2 {
		return -1
	} else if label == kAudioChannelLabel_Mono {
		return 2 // Front center
	} else {
		return -1
	}
}

private func readProc(_ clientData: UnsafeMutableRawPointer, _ position: Int64, _ requestCount: UInt32, _ buffer: UnsafeMutableRawPointer, _ actualCount: UnsafeMutablePointer<UInt32>) -> OSStatus {
	let decoder = Unmanaged<CoreAudioDecoder>.fromOpaque(clientData).takeUnretainedValue()
	guard let source = decoder.audioSource else { return noErr }

	if position != decoder.lastPosition {
		source.seek(Int(position), whence: SEEK_SET)
	}

	var bytesRead = 0
	if requestCount != 0 {
		bytesRead = source.read(buffer, amount: Int(requestCount))
	}

	decoder.lastPosition = position + Int64(bytesRead)

	actualCount.pointee = UInt32(bytesRead)

	return noErr
}

private func getSizeProc(_ clientData: UnsafeMutableRawPointer) -> Int64 {
	let decoder = Unmanaged<CoreAudioDecoder>.fromOpaque(clientData).takeUnretainedValue()
	return decoder.fileSize
}

@objc(CoreAudioDecoder)
class CoreAudioDecoder: NSObject, CogDecoder {
	fileprivate var lastPosition: Int64 = 0
	fileprivate var fileSize: Int64 = 0
	fileprivate var audioSource: CogSource?
	private var audioFile: AudioFileID?
	private var inRef: ExtAudioFileRef?

	private var audioFileOpened = false
	private var inOpened = false
	private var audioFileIsLossy = false

	private var bitrate: Int32 = 0
	private var bitsPerSample: Int32 = 0
	private var floatingPoint = false
	private var channels: Int32 = 0
	private var channelConfig: UInt32 = 0
	private var frequency: Float = 0
	private var totalFrames = 0
	private var frame = 0

	private var codec = ""

	func close() {
		if inOpened {
			ExtAudioFileDispose(inRef!)
			inOpened = false
		}

		if audioFileOpened {
			AudioFileClose(audioFile!)
			audioFileOpened = false
		}

		audioSource = nil
	}

	deinit {
		close()
	}

	func open(_ source: CogSource) -> Bool {
		audioFileOpened = false
		inOpened = false

		if !source.seekable() {
			return false
		}

		audioSource = source
		lastPosition = Int64(source.tell())
		source.seek(0, whence: SEEK_END)
		fileSize = Int64(source.tell())
		source.seek(Int(lastPosition), whence: SEEK_SET)

		var err = AudioFileOpenWithCallbacks(Unmanaged.passUnretained(self).toOpaque(), readProc, nil, getSizeProc, nil, 0, &audioFile)
		if err != noErr {
			return false
		}

		audioFileOpened = true

		err = ExtAudioFileWrapAudioFileID(audioFile!, false, &inRef)
		if err != noErr {
			return false
		}

		inOpened = true

		frame = 0

		return readInfoFromExtAudioFileRef()
	}

	private func readInfoFromExtAudioFileRef() -> Bool {
		guard let inRef else { return false }

		var asbd = AudioStreamBasicDescription()
		var asbdSize = UInt32(MemoryLayout<AudioStreamBasicDescription>.size)
		var err = ExtAudioFileGetProperty(inRef, kExtAudioFileProperty_FileDataFormat, &asbdSize, &asbd)
		if err != noErr {
			ExtAudioFileDispose(inRef)
			return false
		}

		var total: Int64 = 0
		var size = UInt32(MemoryLayout<Int64>.size)
		err = ExtAudioFileGetProperty(inRef, kExtAudioFileProperty_FileLengthFrames, &size, &total)
		if err != noErr {
			ExtAudioFileDispose(inRef)
			return false
		}
		totalFrames = Int(total)

		var afi: AudioFileID?
		size = UInt32(MemoryLayout<AudioFileID?>.size)
		err = ExtAudioFileGetProperty(inRef, kExtAudioFileProperty_AudioFile, &size, &afi)
		if err != noErr {
			ExtAudioFileDispose(inRef)
			return false
		}

		var formatBitsPerSample: Int32 = 0
		size = UInt32(MemoryLayout<Int32>.size)
		err = AudioFileGetProperty(afi!, kAudioFilePropertySourceBitDepth, &size, &formatBitsPerSample)
		if err != noErr {
			if err == kAudioFileUnsupportedPropertyError {
				formatBitsPerSample = 0 // floating point formats apparently don't return this any more
			} else {
				ExtAudioFileDispose(inRef)
				return false
			}
		}

		var bitrateValue: UInt32 = 0
		size = UInt32(MemoryLayout<UInt32>.size)
		err = AudioFileGetProperty(afi!, kAudioFilePropertyBitRate, &size, &bitrateValue)
		if err != noErr {
			ExtAudioFileDispose(inRef)
			return false
		}

		err = AudioFileGetPropertyInfo(afi!, kAudioFilePropertyChannelLayout, &size, nil)
		if err != noErr || size == 0 {
			// No layout in the file (a plain WAV or AIFF): the usual one for its
			// channel count.
			channelConfig = AudioChunk.guessChannelConfig(asbd.mChannelsPerFrame)
		} else {
			let acl = UnsafeMutablePointer<AudioChannelLayout>.allocate(capacity: Int(size))
			defer { acl.deallocate() }
			err = AudioFileGetProperty(afi!, kAudioFilePropertyChannelLayout, &size, acl)
			if err != noErr {
				ExtAudioFileDispose(inRef)
				return false
			}

			var config: UInt32 = 0
			var ok = true
			withUnsafeMutablePointer(to: &acl.pointee.mChannelDescriptions) { descBase in
				let descriptions = UnsafeMutableRawPointer(descBase).assumingMemoryBound(to: AudioChannelDescription.self)
				for i in 0..<Int(acl.pointee.mNumberChannelDescriptions) {
					let channelNumber = ffatGetChannelID(descriptions[i].mChannelLabel)
					if channelNumber >= 0 && channelNumber <= 31 {
						if config & (1 << UInt32(channelNumber)) != 0 {
							ok = false
							return
						}
						config |= 1 << UInt32(channelNumber)
					} else {
						ok = false
						return
					}
				}
			}
			if !ok {
				ExtAudioFileDispose(inRef)
				return false
			}

			channelConfig = config
		}

		bitrate = Int32((bitrateValue + 500) / 1000)

		var formatName: Unmanaged<CFString>?
		size = UInt32(MemoryLayout<Unmanaged<CFString>?>.size)
		err = AudioFormatGetProperty(kAudioFormatProperty_FormatName, asbdSize, &asbd, &size, &formatName)
		if err != noErr {
			ExtAudioFileDispose(inRef)
			return false
		}

		codec = formatName?.takeRetainedValue() as String? ?? ""

		if let range = codec.range(of: ",") {
			codec = String(codec[..<range.lowerBound])
		}

		// Set our properties
		bitsPerSample = formatBitsPerSample
		channels = Int32(asbd.mChannelsPerFrame)
		frequency = Float(asbd.mSampleRate)
		floatingPoint = false

		// mBitsPerChannel will only be set for lpcm formats
		if bitsPerSample <= 0 {
			bitsPerSample = 32
			floatingPoint = true
		}

		audioFileIsLossy = false

		let lowerCodec = codec.lowercased()
		if floatingPoint || lowerCodec.contains("adpcm") || lowerCodec.contains("gsm") {
			audioFileIsLossy = true
		}

		// Set output format
		var result = AudioStreamBasicDescription()

		result.mFormatID = kAudioFormatLinearPCM
		if floatingPoint {
			result.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked
		} else {
			result.mFormatFlags = kAudioFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsBigEndian
		}

		result.mSampleRate = Double(frequency)
		result.mChannelsPerFrame = UInt32(channels)
		result.mBitsPerChannel = UInt32(bitsPerSample)

		result.mBytesPerPacket = UInt32(channels * (bitsPerSample / 8))
		result.mFramesPerPacket = 1
		result.mBytesPerFrame = UInt32(channels * (bitsPerSample / 8))

		err = ExtAudioFileSetProperty(inRef, kExtAudioFileProperty_ClientDataFormat, UInt32(MemoryLayout<AudioStreamBasicDescription>.size), &result)
		if err != noErr {
			ExtAudioFileDispose(inRef)
			return false
		}

		willChangeValue(forKey: "properties")
		didChangeValue(forKey: "properties")

		return true
	}

	func readAudio() -> AudioChunk? {
		guard let inRef else { return nil }

		let frames = 1024
		let bytesPerFrame = Int(channels * (bitsPerSample / 8))
		let buffer = UnsafeMutablePointer<UInt8>.allocate(capacity: frames * bytesPerFrame)
		defer { buffer.deallocate() }

		// Set up the AudioBufferList
		var bufferList = AudioBufferList()
		bufferList.mNumberBuffers = 1
		bufferList.mBuffers.mNumberChannels = UInt32(channels)
		bufferList.mBuffers.mData = UnsafeMutableRawPointer(buffer)
		bufferList.mBuffers.mDataByteSize = UInt32(frames * Int(channels) * Int(bitsPerSample / 8))

		// Read a chunk of PCM input (converted from whatever format)
		var frameCount = UInt32(frames)
		let err = ExtAudioFileRead(inRef, &frameCount, &bufferList)
		if err != noErr {
			return nil
		}

		let chunk = AudioChunk(properties: properties() ?? [:])

		let streamTimestamp = Double(frame) / Double(frequency)
		chunk.streamTimestamp = streamTimestamp

		chunk.assignSamples(buffer, frameCount: Int(frameCount))

		return chunk
	}

	func seek(_ frame: Int) -> Int {
		guard let inRef else { return -1 }
		let err = ExtAudioFileSeek(inRef, Int64(frame))
		if err != noErr {
			return -1
		}

		self.frame = frame

		return frame
	}

	@objc class func fileTypes() -> [Any] {
		var size = UInt32(MemoryLayout<Unmanaged<CFArray>?>.size)
		var extensions: Unmanaged<CFArray>?
		let err = AudioFileGetGlobalInfo(kAudioFileGlobalInfo_AllExtensions, 0, nil, &size, &extensions)
		if err != noErr {
			return []
		}
		return extensions?.takeRetainedValue() as? [Any] ?? []
	}

	@objc class func mimeTypes() -> [Any] {
		var size = UInt32(MemoryLayout<Unmanaged<CFArray>?>.size)
		var mimeTypes: Unmanaged<CFArray>?
		let err = AudioFileGetGlobalInfo(kAudioFileGlobalInfo_AllMIMETypes, 0, nil, &size, &mimeTypes)
		if err != noErr {
			return []
		}
		return mimeTypes?.takeRetainedValue() as? [Any] ?? []
	}

	@objc class func priority() -> Float {
		return 1.0
	}

	@objc class func fileTypeAssociations() -> [Any] {
		return [
			["WAVE File", "wav.icns", "wav", "w64"],
			["AIFF File", "aiff.icns", "aif", "aiff", "aifc"],
			["CAF File", "song.icns", "caf"],
			["AU File", "song.icns", "au"],
			["MPEG Audio File", "mp3.icns", "mp3", "mp2", "mp1", "m2a", "mpa"],
			["MPEG Stream File", "song.icns", "mpeg"],
			["MPEG-4 Audio File", "m4a.icns", "m4a", "mp4", "m4b", "m4r"],
			["MPEG-4 AAC Audio File", "song.icns", "aac", "adts"],
			["AMR Audio File", "song.icns", "amr"],
			["USAC Audio File", "song.icns", "xhe"],
			["AC-3 Audio File", "song.icns", "ac3"],
			["FLAC Audio File", "flac.icns", "flac"],
			["SND Audio File", "song.icns", "snd"]
		]
	}

	@objc func properties() -> [AnyHashable: Any]? {
		return ["channels": channels,
		        "channelConfig": channelConfig,
		        "bitsPerSample": bitsPerSample,
		        "floatingPoint": floatingPoint,
		        "bitrate": bitrate,
		        "sampleRate": frequency,
		        "totalFrames": totalFrames,
		        "seekable": true,
		        "codec": codec,
		        "endian": floatingPoint ? "host" : "big",
		        "encoding": audioFileIsLossy ? "lossy" : "lossless"]
	}

	@objc func metadata() -> [AnyHashable: Any]? {
		return [:]
	}
}
