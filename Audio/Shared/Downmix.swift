//
//  Downmix.swift
//  Cog
//
//  Created by Christopher Snowhill on 2/05/22.
//  Copyright 2022 __LoSnoCo__. All rights reserved.
//

import Accelerate
import CoreAudio
import Foundation

private func downmixToStereo(_ inBuffer: UnsafePointer<Float>, channels: Int, config: UInt32, outBuffer: UnsafeMutablePointer<Float>, count: Int) {
	var frontRatios: (Float, Float) = (0.0, 0.0)
	var frontCenterRatio: Float = 0.0
	var lfeRatio: Float = 0.0
	var backRatios: (Float, Float) = (0.0, 0.0)
	var backCenterRatio: Float = 0.0
	var sideRatios: (Float, Float) = (0.0, 0.0)

	let frontLeft = UInt32(AudioChannelFrontLeft)
	let frontRight = UInt32(AudioChannelFrontRight)
	let frontCenter = UInt32(AudioChannelFrontCenter)
	let backLeft = UInt32(AudioChannelBackLeft)
	let backRight = UInt32(AudioChannelBackRight)
	let lfe = UInt32(AudioChannelLFE)
	let backCenter = UInt32(AudioChannelBackCenter)
	let sideLeft = UInt32(AudioChannelSideLeft)
	let sideRight = UInt32(AudioChannelSideRight)

	if config & (frontLeft | frontRight) != 0 {
		frontRatios.0 = 1.0
	}
	if config & frontCenter != 0 {
		frontRatios.0 = 0.5858
		frontCenterRatio = 0.4142
	}
	if config & (backLeft | backRight) != 0 {
		if config & frontCenter != 0 {
			frontRatios.0 = 0.651
			frontCenterRatio = 0.46
			backRatios.0 = 0.5636
			backRatios.1 = 0.3254
		} else {
			frontRatios.0 = 0.4226
			backRatios.0 = 0.366
			backRatios.1 = 0.2114
		}
	}
	if config & lfe != 0 {
		frontRatios.0 *= 0.8
		frontCenterRatio *= 0.8
		lfeRatio = frontCenterRatio
		backRatios.0 *= 0.8
		backRatios.1 *= 0.8
	}
	if config & backCenter != 0 {
		frontRatios.0 *= 0.86
		frontCenterRatio *= 0.86
		lfeRatio *= 0.86
		backRatios.0 *= 0.86
		backRatios.1 *= 0.86
		backCenterRatio = frontCenterRatio * 0.86
	}
	if config & (sideLeft | sideRight) != 0 {
		var ratio: Float = 0.73
		if config & backCenter != 0 { ratio = 0.85 }
		frontRatios.0 *= ratio
		frontCenterRatio *= ratio
		lfeRatio *= ratio
		backRatios.0 *= ratio
		backRatios.1 *= ratio
		backCenterRatio *= ratio
		sideRatios.0 = 0.463882352941176 * ratio
		sideRatios.1 = 0.267882352941176 * ratio
	}

	var channelIndexes = [Int32](repeating: 0, count: channels)
	for i in 0..<channels {
		channelIndexes[i] = Int32(AudioChunk.findChannelIndex(AudioChunk.extractChannelFlag(UInt32(i), fromConfig: config)))
	}

	vDSP_vclr(outBuffer, 1, vDSP_Length(count * 2))

	let tempBuffer = UnsafeMutablePointer<Float>.allocate(capacity: count * 2)
	defer { tempBuffer.deallocate() }

	for i in 0..<channels {
		var leftRatio: Float = 0.0
		var rightRatio: Float = 0.0
		switch channelIndexes[i] {
		case 0:
			leftRatio = frontRatios.0
			rightRatio = frontRatios.1
		case 1:
			leftRatio = frontRatios.1
			rightRatio = frontRatios.0
		case 2:
			leftRatio = frontCenterRatio
			rightRatio = frontCenterRatio
		case 3:
			leftRatio = lfeRatio
			rightRatio = lfeRatio
		case 4:
			leftRatio = backRatios.0
			rightRatio = backRatios.1
		case 5:
			leftRatio = backRatios.1
			rightRatio = backRatios.0
		case 8:
			leftRatio = backCenterRatio
			rightRatio = backCenterRatio
		case 9:
			leftRatio = sideRatios.0
			rightRatio = sideRatios.1
		case 10:
			leftRatio = sideRatios.1
			rightRatio = sideRatios.0
		default:
			break
		}
		vDSP_vsmul(inBuffer + i, vDSP_Stride(channels), &leftRatio, tempBuffer, 1, vDSP_Length(count))
		vDSP_vsmul(inBuffer + i, vDSP_Stride(channels), &rightRatio, tempBuffer + count, 1, vDSP_Length(count))
		vDSP_vadd(outBuffer, 2, tempBuffer, 1, outBuffer, 2, vDSP_Length(count))
		vDSP_vadd(outBuffer + 1, 2, tempBuffer + count, 1, outBuffer + 1, 2, vDSP_Length(count))
	}
}

private func downmixToMono(_ inBuffer: UnsafePointer<Float>, channels: Int, config: UInt32, outBuffer: UnsafeMutablePointer<Float>, count: Int) {
	var input = inBuffer
	let tempBuffer = UnsafeMutablePointer<Float>.allocate(capacity: count * 2)
	defer { tempBuffer.deallocate() }
	if channels > 2 || config != UInt32(AudioConfigStereo) {
		downmixToStereo(inBuffer, channels: channels, config: config, outBuffer: tempBuffer, count: count)
		input = UnsafePointer(tempBuffer)
	}
	cblas_scopy(Int32(count), input, 2, outBuffer, 1)
	vDSP_vadd(outBuffer, 1, input + 1, 2, outBuffer, 1, vDSP_Length(count))
	var scale: Float = 0.5
	vDSP_vsmul(outBuffer, 1, &scale, outBuffer, 1, vDSP_Length(count))
}

private func upmix(_ inBuffer: UnsafePointer<Float>, inchannels: Int, inconfig: UInt32, outBuffer: UnsafeMutablePointer<Float>, outchannels: Int, outconfig: UInt32, monoToCenter: Bool, count: Int) {
	let frontLeft = UInt32(AudioChannelFrontLeft)
	let frontRight = UInt32(AudioChannelFrontRight)
	let frontCenter = UInt32(AudioChannelFrontCenter)
	let backLeft = UInt32(AudioChannelBackLeft)
	let backRight = UInt32(AudioChannelBackRight)
	let lfe = UInt32(AudioChannelLFE)
	let backCenter = UInt32(AudioChannelBackCenter)
	let sideLeft = UInt32(AudioChannelSideLeft)
	let sideRight = UInt32(AudioChannelSideRight)
	let notFound = UInt32.max

	if inconfig == UInt32(AudioConfigMono) && monoToCenter && (outconfig & frontCenter) != 0 {
		let cIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: frontCenter))
		cblas_scopy(Int32(count), inBuffer, 1, outBuffer + cIndex, Int32(outchannels))
		for i in 0..<cIndex {
			vDSP_vclr(outBuffer + i, vDSP_Stride(outchannels), vDSP_Length(count))
		}
		for i in (cIndex + 1)..<outchannels {
			vDSP_vclr(outBuffer + i, vDSP_Stride(outchannels), vDSP_Length(count))
		}
	} else if inconfig == UInt32(AudioConfigMono) && (outconfig & frontLeft) != 0 && (outconfig & frontRight) != 0 {
		/* A device's channel count says nothing of what is connected to it: on
		 * an interface taken for 7.1, only the first two may be in use. */
		let flIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: frontLeft))
		let frIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: frontRight))
		vDSP_vclr(outBuffer, 1, vDSP_Length(count * outchannels))
		cblas_scopy(Int32(count), inBuffer, 1, outBuffer + flIndex, Int32(outchannels))
		cblas_scopy(Int32(count), inBuffer, 1, outBuffer + frIndex, Int32(outchannels))
	} else if inconfig == UInt32(AudioConfig4Point0) && outchannels >= 5 {
		let flIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: frontLeft))
		let frIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: frontRight))
		let blIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: backLeft))
		let brIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: backRight))
		vDSP_vclr(outBuffer, 1, vDSP_Length(count * outchannels))
		if flIndex != Int(notFound) { cblas_scopy(Int32(count), inBuffer + 0, 4, outBuffer + flIndex, Int32(outchannels)) }
		if frIndex != Int(notFound) { cblas_scopy(Int32(count), inBuffer + 1, 4, outBuffer + frIndex, Int32(outchannels)) }
		if blIndex != Int(notFound) { cblas_scopy(Int32(count), inBuffer + 2, 4, outBuffer + blIndex, Int32(outchannels)) }
		if brIndex != Int(notFound) { cblas_scopy(Int32(count), inBuffer + 3, 4, outBuffer + brIndex, Int32(outchannels)) }
	} else if inconfig == UInt32(AudioConfig5Point0) && outchannels >= 6 {
		let flIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: frontLeft))
		let frIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: frontRight))
		let cIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: frontCenter))
		let blIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: backLeft))
		let brIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: backRight))
		vDSP_vclr(outBuffer, 1, vDSP_Length(count * outchannels))
		if flIndex != Int(notFound) { cblas_scopy(Int32(count), inBuffer + 0, 5, outBuffer + flIndex, Int32(outchannels)) }
		if frIndex != Int(notFound) { cblas_scopy(Int32(count), inBuffer + 1, 5, outBuffer + frIndex, Int32(outchannels)) }
		if cIndex != Int(notFound) { cblas_scopy(Int32(count), inBuffer + 2, 5, outBuffer + cIndex, Int32(outchannels)) }
		if blIndex != Int(notFound) { cblas_scopy(Int32(count), inBuffer + 3, 5, outBuffer + blIndex, Int32(outchannels)) }
		if brIndex != Int(notFound) { cblas_scopy(Int32(count), inBuffer + 4, 5, outBuffer + brIndex, Int32(outchannels)) }
	} else if inconfig == UInt32(AudioConfig6Point1) && outchannels >= 8 {
		let flIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: frontLeft))
		let frIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: frontRight))
		let cIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: frontCenter))
		let lfeIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: lfe))
		let blIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: backLeft))
		let brIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: backRight))
		let bcIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: backCenter))
		let slIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: sideLeft))
		let srIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: sideRight))
		vDSP_vclr(outBuffer, 1, vDSP_Length(count * outchannels))
		if flIndex != Int(notFound) { cblas_scopy(Int32(count), inBuffer + 0, 7, outBuffer + flIndex, Int32(outchannels)) }
		if frIndex != Int(notFound) { cblas_scopy(Int32(count), inBuffer + 1, 7, outBuffer + frIndex, Int32(outchannels)) }
		if cIndex != Int(notFound) { cblas_scopy(Int32(count), inBuffer + 2, 7, outBuffer + cIndex, Int32(outchannels)) }
		if lfeIndex != Int(notFound) { cblas_scopy(Int32(count), inBuffer + 3, 7, outBuffer + lfeIndex, Int32(outchannels)) }
		if slIndex != Int(notFound) { cblas_scopy(Int32(count), inBuffer + 4, 7, outBuffer + slIndex, Int32(outchannels)) }
		if srIndex != Int(notFound) { cblas_scopy(Int32(count), inBuffer + 5, 7, outBuffer + srIndex, Int32(outchannels)) }
		if bcIndex != Int(notFound) {
			cblas_scopy(Int32(count), inBuffer + 6, 7, outBuffer + bcIndex, Int32(outchannels))
		} else {
			if blIndex != Int(notFound) { cblas_scopy(Int32(count), inBuffer + 6, 7, outBuffer + blIndex, Int32(outchannels)) }
			if brIndex != Int(notFound) { cblas_scopy(Int32(count), inBuffer + 6, 7, outBuffer + brIndex, Int32(outchannels)) }
		}
	} else {
		vDSP_vclr(outBuffer, 1, vDSP_Length(count * outchannels))
		for i in 0..<inchannels {
			let channelFlag = AudioChunk.extractChannelFlag(UInt32(i), fromConfig: inconfig)
			let outIndex = Int(AudioChunk.channelIndex(fromConfig: outconfig, forFlag: channelFlag))
			if outIndex != Int(notFound) {
				cblas_scopy(Int32(count), inBuffer + i, Int32(inchannels), outBuffer + outIndex, Int32(outchannels))
			}
		}
	}
}

@objc(DownmixProcessor)
public class DownmixProcessor: NSObject {
	private var inputFormat: AudioStreamBasicDescription
	private var outputFormat: AudioStreamBasicDescription
	private var inConfig: UInt32
	private var outConfig: UInt32

	/* Fit mono to the output's center channel, where it has one, rather than to
	 * its front left and right. */
	@objc public var monoToCenter = false

	@objc public init?(inputFormat inf: AudioStreamBasicDescription, inputConfig iConfig: UInt32, andOutputFormat outf: AudioStreamBasicDescription, outputConfig oConfig: UInt32) {
		if inf.mFormatID != kAudioFormatLinearPCM ||
			(inf.mFormatFlags & kAudioFormatFlagsNativeFloatPacked) != kAudioFormatFlagsNativeFloatPacked ||
			inf.mBitsPerChannel != 32 ||
			inf.mBytesPerFrame != (4 * inf.mChannelsPerFrame) ||
			inf.mBytesPerPacket != inf.mFramesPerPacket * inf.mBytesPerFrame {
			return nil
		}

		if outf.mFormatID != kAudioFormatLinearPCM ||
			(outf.mFormatFlags & kAudioFormatFlagsNativeFloatPacked) != kAudioFormatFlagsNativeFloatPacked ||
			outf.mBitsPerChannel != 32 ||
			outf.mBytesPerFrame != (4 * outf.mChannelsPerFrame) ||
			outf.mBytesPerPacket != outf.mFramesPerPacket * outf.mBytesPerFrame {
			return nil
		}

		inputFormat = inf
		outputFormat = outf
		inConfig = iConfig
		outConfig = oConfig
		super.init()
	}

	@objc public func process(_ inBuffer: UnsafeRawPointer, frameCount frames: Int, output outBuffer: UnsafeMutableRawPointer) {
		let input = inBuffer.assumingMemoryBound(to: Float.self)
		let output = outBuffer.assumingMemoryBound(to: Float.self)
		let inChannels = Int(inputFormat.mChannelsPerFrame)
		let outChannels = Int(outputFormat.mChannelsPerFrame)

		if inChannels > 2 && outConfig == UInt32(AudioConfigStereo) {
			downmixToStereo(input, channels: inChannels, config: inConfig, outBuffer: output, count: frames)
		} else if inChannels > 1 && outConfig == UInt32(AudioConfigMono) {
			downmixToMono(input, channels: inChannels, config: inConfig, outBuffer: output, count: frames)
		} else if inChannels > outChannels {
			/* More channels in than the device takes, and the device is neither
			 * stereo nor mono: downmix to stereo, then fit that to the device's
			 * layout. Without this the chain below matches nothing and leaves the
			 * caller's buffer unwritten, which reaches the device as silence. */
			let tempBuffer = UnsafeMutablePointer<Float>.allocate(capacity: frames * 2)
			defer { tempBuffer.deallocate() }
			downmixToStereo(input, channels: inChannels, config: inConfig, outBuffer: tempBuffer, count: frames)
			upmix(UnsafePointer(tempBuffer), inchannels: 2, inconfig: UInt32(AudioConfigStereo), outBuffer: output, outchannels: outChannels, outconfig: outConfig, monoToCenter: monoToCenter, count: frames)
		} else if inChannels < outChannels {
			upmix(input, inchannels: inChannels, inconfig: inConfig, outBuffer: output, outchannels: outChannels, outconfig: outConfig, monoToCenter: monoToCenter, count: frames)
		} else if inConfig == outConfig {
			/* Identical layouts: the sample order already matches the device, so a
			 * raw copy is all that is needed. */
			memcpy(outBuffer, inBuffer, frames * Int(outputFormat.mBytesPerPacket))
		} else {
			/* The same number of channels but a different layout (say a 2.1 input
			 * and a 3.0 device): copying the bytes positionally would send the LFE
			 * sample to the centre speaker. Map each channel to the position its
			 * flag names and clear any position that has no input to fill it, so
			 * nothing lands on the wrong output. */
			upmix(input, inchannels: inChannels, inconfig: inConfig, outBuffer: output, outchannels: outChannels, outconfig: outConfig, monoToCenter: monoToCenter, count: frames)
		}
	}
}
