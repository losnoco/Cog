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

	private static var sharedVisualizationController: VisualizationController = {
		let visualizationController = VisualizationController()
		return visualizationController
	}()

	@objc
	class func sharedController() -> VisualizationController {
		return sharedVisualizationController
	}

	@objc
	func postLatency(_ latency: Double) {
		self.latency = latency
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
			for i in 0..<amount {
				let x = bufferPointer[i]
				self.visAudio[j] = x
				j += 1; if j >= k { j = 0 }
			}
			self.visAudioCursor = j
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
			if(outPCM != nil) {
				let pcmPointer = UnsafeMutableBufferPointer<Float>(start: outPCM, count: 4096)
				for i in 0...4095 {
					pcmPointer[i] = 0.0
				}
			}
			if(visFFT != nil) {
				let fftPointer = UnsafeMutableBufferPointer<Float>(start: visFFT, count: 2048)
				for i in 0...2047 {
					fftPointer[i] = 0.0
				}
			}
			return
		}

		var outPCMCopy = Array<Float>(repeating: 0.0, count: 4096)

		serialQueue.sync {
			let latencySamples = (Int)(self.latency * self.sampleRate)
			var j = self.visAudioCursor - latencySamples
			let k = self.visAudioSize
			if j < 0 { j += k }
			for i in 0...4095 {
				let x = self.visAudio[j]
				outPCMCopy[i] = x
				j += 1; if j >= k { j = 0 }
			}
		}

		if(outPCM != nil) {
			let pcmPointer = UnsafeMutableBufferPointer<Float>(start: outPCM, count: 4096)
			for i in 0...4095 {
				let x = outPCMCopy[i]
				pcmPointer[i] = x
			}
		}

		if(visFFT != nil) {
			serialQueue.sync {
				fft_calculate(outPCMCopy, visFFT, 2048)
			}
		}
	}
}
