//
//  VisualizationController.swift
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 6/30/22.
//

import Foundation

@objc(VisualizationController)
class VisualizationController : NSObject {
	var serialQueue = DispatchQueue(label: "Visualization Queue")
	var sampleRate = 0.0
	var latency = 0.0
	var visAudio: [Float] = Array(repeating: 0.0, count: 44100 * 45)
	var visAudioCursor = 0
	var visAudioSize = 0
	var visSamplesPosted: UInt64 = 0

	private static var sharedVisualizationController: VisualizationController = {
		let visualizationController = VisualizationController()
		return visualizationController
	}()

	@objc
	class func sharedController() -> VisualizationController {
		return sharedVisualizationController
	}
	
	@objc
	func reset() {
		serialQueue.sync {
			self.latency = 0;
			let amount = self.visAudioSize
			for i in 0..<amount {
				self.visAudio[i] = 0
			}
			self.visSamplesPosted = 0;
		}
	}

	@objc
	func postLatency(_ latency: Double) {
		self.latency = latency
	}
	
	@objc
	func samplesPosted() -> UInt64 {
		return self.visSamplesPosted
	}

	@objc
	func postSampleRate(_ sampleRate: Double) {
		serialQueue.sync {
			if(self.sampleRate != sampleRate) {
				self.sampleRate = sampleRate
				visAudioSize = (Int)(sampleRate * 45.0)
				visAudio = Array(repeating: 0.0, count: visAudioSize)
				visAudioCursor = 0
			}
		}
	}

	@objc
	func postVisPCM(_ inPCM: UnsafePointer<Float>?, amount: Int) {
		serialQueue.sync {
			let bufferPointer = UnsafeBufferPointer<Float>(start: inPCM, count: amount)
			var j = self.visAudioCursor
			let k = self.visAudioSize
			if(j + amount <= k) {
				let endIndex = j + amount;
				self.visAudio.replaceSubrange(j..<endIndex, with: bufferPointer)
				j += amount
				if(j >= k) { j = 0 }
			} else {
				let inEndIndex = k - j
				let remainder = amount - inEndIndex
				self.visAudio.replaceSubrange(j..<k, with: bufferPointer.prefix(inEndIndex))
				self.visAudio.replaceSubrange(0..<remainder, with: bufferPointer.suffix(remainder))
				j = remainder
			}
			self.visAudioCursor = j
			self.visSamplesPosted += UInt64(amount);
		}
	}

	@objc
	func readSampleRate() -> Double {
		serialQueue.sync {
			return self.sampleRate
		}
	}

	@objc
	func copyVisPCM(_ outPCM: UnsafeMutablePointer<Float>?, visFFT: UnsafeMutablePointer<Float>?, latencyOffset: Double) {
		if(self.visAudioSize == 0) {
			outPCM?.update(repeating: 0.0, count: 4096)
			visFFT?.update(repeating: 0.0, count: 2048)
			return
		}

		var outPCMCopy = Array<Float>(repeating: 0.0, count: 4096)

		serialQueue.sync {
			// Offset latency so the target sample is in the center of the window
			let latencySamples = (Int)((self.latency + latencyOffset) * self.sampleRate) + 2048
			var samplesToDo = 4096;
			if(latencySamples < 0) {
				return;
			}
			if(latencySamples < 4096) {
				// Latency can sometimes dip below this threshold
				samplesToDo = latencySamples;
			}
			var j = self.visAudioCursor - latencySamples
			let k = self.visAudioSize
			if j < 0 { j += k }
			if(j + samplesToDo <= k) {
				outPCMCopy.replaceSubrange(0..<samplesToDo, with: self.visAudio.suffix(from: j).prefix(samplesToDo))
			} else {
				let outEndIndex = k - j
				let remainder = samplesToDo - outEndIndex
				outPCMCopy.replaceSubrange(0..<outEndIndex, with: self.visAudio.suffix(from: j))
				outPCMCopy.replaceSubrange(outEndIndex..<samplesToDo, with: self.visAudio.prefix(remainder))
			}
		}

		outPCM?.update(from: outPCMCopy, count: 4096)

		if(visFFT != nil) {
			serialQueue.sync {
				fft_calculate(outPCMCopy, visFFT, 2048)
			}
		}
	}
}
