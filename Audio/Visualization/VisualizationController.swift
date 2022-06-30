//
//  VisualizationController.swift
//  CogAudio Framework
//
//  Created by Christopher Snowhill on 6/30/22.
//

import Foundation

@objc(VisualizationController)
class VisualizationController {
	var serialQueue = DispatchQueue(label: "Visualization Queue")
	var sampleRate = 44100.0
	var latency = 0.0
	var visAudio: [Float] = Array(repeating: 0.0, count: 44100 * 45)
	var visAudioCursor = 0
	var visAudioSize = 0

	private static var sharedController: VisualizationController = {
		let visualizationController = VisualizationController()
		return visualizationController
	}()
	
	class func sharedVisualizationController() -> VisualizationController {
		return sharedController
	}

	func postLatency(_ latency: Double) {
		self.latency = latency
	}

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

	func postVisPCM(_ inPCM: UnsafePointer<Float>?, amount: Int) {
		serialQueue.sync {
			let bufferPointer = UnsafeBufferPointer(start: inPCM, count: amount)
			if let bptr = bufferPointer {
				let dataArray = bptr.assumingMemoryBound(to: Float.self)
				var j = self.visAudioCursor
				var k = self.visAudioSize
				for i in 0..<amount {
					let x = Float(dataArray[i])
					self.visAudio[j] = x
					j++; if j >= k { j = 0 }
				}
				self.visAudioCursor = j
			}
		}
	}
	
	func readSampleRate() -> Double {
		serialQueue.sync {
			return self.sampleRate
		}
	}
	
	func copyVisPCM(_ outPCM: UnsafeMutablePointer<Float>?, visFFT: UnsafeMutablePointer<Float>?, latencyoffset: Double) {
		let outPCMCopy = Array<Float>(repeating: 0.0, count: 4096)

		serialQueue.sync {
			var latencySamples = (Int)(self.latency * self.sampleRate)
			var j = self.visAudioCursor - latencySamples
			var k = self.visAudioSize
			if j < 0 { j += k }
			for i in 0..4095 {
				let x = self.visAudio[j]
				outPCMCopy[i] = x
				j++; if j >= k { j = 0 }
			}
		}
		
		let pcmPointer = UnsafeMutableBufferPointer(start: outPCM, count: 4096)
		if let bptr = pcmPointer {
			let dataArray = bptr.assumingMemoryBound(to: Float.self)
			for i in 0..4095 {
				let x = outPCMCopy[i]
				dataArray[i] = x
			}
		}
		
		let fftPointer = UnsafeMutablePointer(start: visFFT, count: 2048)
		if let bptr = fftPointer {
			serialQueue.sync {
				fft_calculate(outPCMCopy, bptr, 2048)
			}
		}
	}
}
