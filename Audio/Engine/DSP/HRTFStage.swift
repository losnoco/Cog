//
//  HRTFStage.swift
//  CogAudio
//
//  Created by Christopher Snowhill on 9/30/26.
//

@_implementationOnly import CogAudioEngineInternal
import CoreMotion
import Foundation
import simd

/// Headphone virtualisation (as `DSPHRTFNode`): every input channel is
/// convolved with the SADIE D02 impulse set to binaural stereo, enabled by
/// `enableHrtf`, and optionally turned with the listener's head
/// (`enableHeadTracking`, macOS 14 and later).
///
/// `HeadphoneFilter` has no latency but needs its history primed: after each
/// start or seek the stage extrapolates the history backward from the first
/// block (LPC, as the node did) and runs it through the filter unheard.
final class HRTFStage: NSObject, DSPStage {
	/// The impulse set; the app bundle's unless a test supplies one.
	var impulseFile: URL? = Bundle.main.url(forResource: "SADIE_D02-96000", withExtension: "mhr")

	private let lock = UnfairLock()
	private var enabled: Bool
	private var headTracking: Bool
	private var pendingMatrix: simd_float4x4?

	// DSP thread only.
	private var filter: HeadphoneFilter?
	private var inputFormat: StreamFormat?
	private var outputFormat: StreamFormat?
	private var primed = false
	private var output: [Float] = []
	private var prefill: [Float] = []
	private var extrapolateBuffer: UnsafeMutableRawPointer?
	private var extrapolateBufferSize = 0

	static let outputConfig = UInt32(AudioChannelSideLeft | AudioChannelSideRight)

	override init() {
		let defaults = UserDefaults.standard
		enabled = defaults.bool(forKey: "enableHrtf")
		headTracking = defaults.bool(forKey: "enableHeadTracking")
		super.init()
		defaults.addObserver(self, forKeyPath: "enableHrtf", options: [], context: &Self.context)
		defaults.addObserver(self, forKeyPath: "enableHeadTracking", options: [], context: &Self.context)
		HeadTracker.shared.onRotation = { [weak self] matrix in
			self?.lock.withLock { self?.pendingMatrix = matrix }
		}
		updateTracking()
	}

	deinit {
		UserDefaults.standard.removeObserver(self, forKeyPath: "enableHrtf", context: &Self.context)
		UserDefaults.standard.removeObserver(self, forKeyPath: "enableHeadTracking", context: &Self.context)
		HeadTracker.shared.stop()
		free(extrapolateBuffer)
	}

	private static var context = 0

	override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		guard context == &Self.context else {
			super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
			return
		}
		let defaults = UserDefaults.standard
		let enabled = defaults.bool(forKey: "enableHrtf")
		let headTracking = defaults.bool(forKey: "enableHeadTracking")
		lock.withLock {
			self.enabled = enabled
			self.headTracking = headTracking
			if !headTracking {
				pendingMatrix = matrix_identity_float4x4
			}
		}
		updateTracking()
	}

	private func updateTracking() {
		let (enabled, headTracking) = lock.withLock { (self.enabled, self.headTracking) }
		if enabled && headTracking {
			HeadTracker.shared.start()
		} else {
			HeadTracker.shared.stop()
		}
	}

	var isActive: Bool {
		lock.withLock { enabled }
	}

	func configure(input: StreamFormat) -> StreamFormat {
		inputFormat = input
		primed = false
		guard let impulseFile else {
			filter = nil
			outputFormat = input
			return input
		}
		let config = input.channelConfig != 0 ? input.channelConfig : AudioChunk.guessChannelConfig(UInt32(input.channels))
		let matrix = HeadTracker.shared.currentMatrix
		filter = HeadphoneFilter(impulseFile: impulseFile, forSampleRate: input.sampleRate, withInputChannels: Int32(input.channels), withConfig: config, withMatrix: matrix)
		guard filter != nil else {
			outputFormat = input
			return input
		}
		let output = StreamFormat(sampleRate: input.sampleRate, channels: 2, channelConfig: Self.outputConfig)
		outputFormat = output
		return output
	}

	func process(_ buffer: DSPBuffer) {
		guard let filter, let inputFormat, let outputFormat, buffer.frames > 0 else {
			if filter != nil, let outputFormat {
				buffer.resize(frames: 0, format: outputFormat)
			}
			return
		}
		if let matrix = lock.withLock({ () -> simd_float4x4? in
			defer { pendingMatrix = nil }
			return pendingMatrix
		}) {
			filter.reload(withMatrix: matrix)
		}

		let channels = inputFormat.channels
		let frames = buffer.frames
		if !primed {
			prime(filter, from: buffer, channels: channels)
		}

		if output.count < frames * 2 {
			output = [Float](repeating: 0, count: frames * 2)
		}
		buffer.samples.withUnsafeBufferPointer { input in
			output.withUnsafeMutableBufferPointer { out in
				filter.process(input.baseAddress!, sampleCount: Int32(frames), toBuffer: out.baseAddress!)
			}
		}
		buffer.resize(frames: frames, format: outputFormat)
		for i in 0..<(frames * 2) {
			buffer.samples[i] = output[i]
		}
	}

	/// Fills the filter's history with a backward extrapolation of the start
	/// of `buffer`, so the first output is not the impulse response of a
	/// step out of silence.
	private func prime(_ filter: HeadphoneFilter, from buffer: DSPBuffer, channels: Int) {
		primed = true
		let history = filter.needPrefill()
		guard history > 0 else { return }
		let frames = buffer.frames
		let basis = min(frames, 4096)
		prefill = [Float](repeating: 0, count: (history + basis) * channels)
		for i in 0..<(basis * channels) {
			prefill[history * channels + i] = buffer.samples[i]
		}
		prefill.withUnsafeMutableBufferPointer { samples in
			lpc_extrapolate_bkwd(samples.baseAddress! + history * channels, basis, basis, Int32(channels), Int32(LPC_ORDER), history, &extrapolateBuffer, &extrapolateBufferSize)
		}
		var discard = [Float](repeating: 0, count: history * 2)
		prefill.withUnsafeBufferPointer { samples in
			discard.withUnsafeMutableBufferPointer { out in
				filter.process(samples.baseAddress!, sampleCount: Int32(history), toBuffer: out.baseAddress!)
			}
		}
	}

	func reset() {
		filter?.reset()
		primed = false
	}
}

/// The listener's head orientation from AirPods and the like
/// (`CMHeadphoneMotionManager`, macOS 14 and later), as a rotation matrix
/// relative to where the head was when tracking started or was last reset
/// (`CogPlaybackDidResetHeadTracking`).
final class HeadTracker {
	static let shared = HeadTracker()

	/// Called on the main thread with each new matrix for the filter.
	var onRotation: ((simd_float4x4) -> Void)?

	private let lock = UnfairLock()
	private var reference: simd_float4x4?
	private var matrix = matrix_identity_float4x4
	private var running = false
	private var manager: AnyObject?

	private static let mirror = simd_float4x4(rows: [
		SIMD4(-1, 0, 0, 0),
		SIMD4(0, 1, 0, 0),
		SIMD4(0, 0, 1, 0),
		SIMD4(0, 0, 0, 1),
	])

	private init() {
		NotificationCenter.default.addObserver(forName: Notification.Name("CogPlaybackDidResetHeadTracking"), object: nil, queue: .main) { [weak self] _ in
			self?.lock.withLock { self?.reference = nil }
		}
	}

	/// The matrix for the filter right now.
	var currentMatrix: simd_float4x4 {
		lock.withLock { matrix }
	}

	func start() {
		guard #available(macOS 14, *) else { return }
		DispatchQueue.main.async { [self] in
			guard !running else { return }
			let motion = (manager as? CMHeadphoneMotionManager) ?? CMHeadphoneMotionManager()
			manager = motion
			guard motion.isDeviceMotionAvailable else { return }
			running = true
			lock.withLock { reference = nil }
			motion.startDeviceMotionUpdates(to: .main) { [weak self] motion, _ in
				guard let self, let motion else { return }
				self.report(Self.convert(motion.attitude.rotationMatrix))
			}
		}
	}

	func stop() {
		guard #available(macOS 14, *) else { return }
		DispatchQueue.main.async { [self] in
			guard running else { return }
			running = false
			(manager as? CMHeadphoneMotionManager)?.stopDeviceMotionUpdates()
			lock.withLock {
				reference = nil
				matrix = matrix_identity_float4x4
			}
			onRotation?(matrix_identity_float4x4)
		}
	}

	private func report(_ rotation: simd_float4x4) {
		let result: simd_float4x4 = lock.withLock {
			if reference == nil {
				reference = simd_inverse(rotation)
			}
			matrix = simd_mul(simd_mul(Self.mirror, rotation), reference!)
			return matrix
		}
		onRotation?(result)
	}

	/// The node's conversion from CoreMotion's frame to the filter's.
	private static func convert(_ r: CMRotationMatrix) -> simd_float4x4 {
		simd_float4x4(columns: (
			SIMD4(Float(r.m33), Float(-r.m31), Float(r.m32), 0),
			SIMD4(Float(r.m13), Float(-r.m11), Float(r.m12), 0),
			SIMD4(Float(r.m23), Float(-r.m21), Float(r.m22), 0),
			SIMD4(0, 0, 0, 1)
		))
	}
}
