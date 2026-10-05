//
//  TimeStretchStage.swift
//  CogAudio
//
//  Created by Christopher Snowhill on 9/30/26.
//

@_implementationOnly import CogAudioEngineInternal
import Foundation

/// Tempo and pitch (as `DSPRubberbandNode` and `DSPSignalsmithStretchNode`),
/// with the engine chosen by `rubberbandEngine`: `varispeed` (resampling,
/// so the pitch follows the tempo, as when a record player runs fast), `faster` (Rubber
/// Band R2), `finer` (R3), `signalsmith`, or `disabled`.
///
/// Unlike those nodes, the stage runs only while `tempo` or `pitch` is off
/// 1: at unity nothing touches the audio. Varispeed is the exception once it
/// has run: it glides through 1 rather than restarting, and goes back to
/// bit-exact at the next reset. The stage counts input in output frames, so
/// it can say how many frames it still owes (for placing presentation
/// events) and trim its drain to exactly the stretched length.
final class TimeStretchStage: NSObject, DSPStage {
	private static let keys = ["rubberbandEngine", "tempo", "pitch", "rubberbandTransients", "rubberbandDetector", "rubberbandPhase",
	                           "rubberbandWindow", "rubberbandSmoothing", "rubberbandFormant", "rubberbandPitch", "rubberbandChannels"]

	private struct Settings: Equatable {
		var engine: String
		var tempo: Double
		var pitch: Double
		/// `RubberBandOptions`, spelled as its underlying type: a stored
		/// property may not use a type from the implementation-only import.
		var rubberBandOptions: Int32

		static func current() -> Settings {
			let defaults = UserDefaults.standard
			let engine = defaults.string(forKey: "rubberbandEngine") ?? "disabled"
			let tempo = defaults.double(forKey: "tempo")
			// Varispeed's pitch is its tempo; a pitch left over from another
			// engine must not keep the stage running or restart it.
			let pitch = engine == "varispeed" ? 1 : defaults.double(forKey: "pitch")
			return Settings(engine: engine,
			                tempo: tempo > 0 ? tempo : 1,
			                pitch: pitch > 0 ? pitch : 1,
			                rubberBandOptions: RubberBandBackend.options(from: defaults))
		}
	}

	private let lock = UnfairLock()
	private var settings = Settings.current()

	// DSP thread only.
	private var backend: StretchBackend?
	private var applied: Settings?
	private var format: StreamFormat?
	private var output: [Float] = []
	private var countIn = 0.0 // input consumed, in output frames
	private var countOut = 0
	/// Varispeed has had audio through it since the last start.
	private var warm = false

	override init() {
		super.init()
		for key in Self.keys {
			UserDefaults.standard.addObserver(self, forKeyPath: key, options: [], context: &Self.context)
		}
	}

	deinit {
		for key in Self.keys {
			UserDefaults.standard.removeObserver(self, forKeyPath: key, context: &Self.context)
		}
	}

	private static var context = 0

	override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		guard context == &Self.context else {
			super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
			return
		}
		let current = Settings.current()
		lock.withLock { settings = current }
	}

	private var currentSettings: Settings {
		lock.withLock { settings }
	}

	// MARK: - DSPStage

	var isActive: Bool {
		let settings = currentSettings
		if settings.engine == "varispeed" && warm {
			return true
		}
		return settings.engine != "disabled" && (abs(settings.tempo - 1) > 1e-6 || abs(settings.pitch - 1) > 1e-6)
	}

	var timeRatio: Double {
		applied?.tempo ?? 1
	}

	var pendingFrames: Int {
		max(0, Int(countIn.rounded()) - countOut)
	}

	var inspection: StageInspection? {
		applied.map { .timeStretch(engine: $0.engine, tempo: $0.tempo, pitch: $0.pitch) }
	}

	func configure(input: StreamFormat) -> StreamFormat {
		format = input
		start(with: currentSettings)
		return input
	}

	private func start(with settings: Settings) {
		guard let format else { return }
		applied = settings
		countIn = 0
		countOut = 0
		warm = false
		switch settings.engine {
		case "varispeed":
			backend = VarispeedBackend(format: format, tempo: settings.tempo)
		case "signalsmith":
			backend = SignalsmithBackend(format: format, tempo: settings.tempo, pitch: settings.pitch)
		default:
			backend = RubberBandBackend(format: format, options: settings.rubberBandOptions, tempo: settings.tempo, pitch: settings.pitch)
		}
	}

	/// Picks up setting changes: tempo, pitch and most Rubber Band options
	/// apply in place; another engine or an option Rubber Band cannot change
	/// while running means a fresh start.
	private func applySettings() {
		let settings = currentSettings
		guard let applied, settings != applied else { return }
		if settings.engine != applied.engine || RubberBandBackend.needsRestart(from: applied.rubberBandOptions, to: settings.rubberBandOptions, engine: settings.engine) {
			start(with: settings)
			return
		}
		backend?.set(tempo: settings.tempo, pitch: settings.pitch, options: settings.rubberBandOptions)
		self.applied = settings
	}

	func process(_ buffer: DSPBuffer) {
		applySettings()
		guard let backend, let format else { return }
		warm = applied?.engine == "varispeed"
		output.removeAll(keepingCapacity: true)
		countIn += Double(buffer.frames) / (applied?.tempo ?? 1)
		buffer.samples.withUnsafeBufferPointer { input in
			backend.process(UnsafeBufferPointer(rebasing: input[0..<(buffer.frames * format.channels)]), into: &output)
		}
		give(to: buffer)
	}

	func drain(_ buffer: DSPBuffer) {
		guard let backend, let format else { return }
		output.removeAll(keepingCapacity: true)
		countIn += Double(buffer.frames) / (applied?.tempo ?? 1)
		buffer.samples.withUnsafeBufferPointer { input in
			backend.process(UnsafeBufferPointer(rebasing: input[0..<(buffer.frames * format.channels)]), into: &output)
		}
		backend.drain(into: &output)
		// The stretchers flush more than the input amounts to; keep exactly
		// the stretched length, as the nodes did.
		let wanted = max(0, Int(countIn.rounded()) - countOut)
		if output.count > wanted * format.channels {
			output.removeLast(output.count - wanted * format.channels)
		}
		give(to: buffer)
		if let applied {
			start(with: applied)
		}
	}

	func reset() {
		if let applied {
			start(with: applied)
		}
	}

	private func give(to buffer: DSPBuffer) {
		guard let format else { return }
		let frames = output.count / format.channels
		buffer.resize(frames: frames, format: format)
		for i in 0..<(frames * format.channels) {
			buffer.samples[i] = output[i]
		}
		countOut += frames
	}
}

// MARK: - Backends

/// Rubber Band's option flags as the `RubberBandOptions` bit set.
private func rb(_ option: RubberBandOption) -> RubberBandOptions {
	RubberBandOptions(bitPattern: option.rawValue)
}

/// A stretcher working on interleaved float in the stage's format.
private protocol StretchBackend: AnyObject {
	func set(tempo: Double, pitch: Double, options: RubberBandOptions)
	/// Feeds `input` and appends whatever output is ready to `output`.
	func process(_ input: UnsafeBufferPointer<Float>, into output: inout [Float])
	/// Appends everything still inside after the last input.
	func drain(into output: inout [Float])
}

/// Splits and joins interleaved audio for the stretchers, which work on
/// separate channel buffers.
private final class ChannelBuffers {
	let channels: Int
	let capacity: Int
	private(set) var storage: [UnsafeMutablePointer<Float>]

	init(channels: Int, capacity: Int) {
		self.channels = channels
		self.capacity = capacity
		storage = (0..<channels).map { _ in UnsafeMutablePointer<Float>.allocate(capacity: capacity) }
		for pointer in storage {
			pointer.initialize(repeating: 0, count: capacity)
		}
	}

	deinit {
		for pointer in storage {
			pointer.deallocate()
		}
	}

	func clear(_ frames: Int) {
		for pointer in storage {
			pointer.update(repeating: 0, count: frames)
		}
	}

	func split(_ input: UnsafeBufferPointer<Float>, from frame: Int, count: Int) {
		for channel in 0..<channels {
			let target = storage[channel]
			for i in 0..<count {
				target[i] = input[(frame + i) * channels + channel]
			}
		}
	}

	func join(_ frames: Int, into output: inout [Float]) {
		let start = output.count
		output.append(contentsOf: repeatElement(0, count: frames * channels))
		for channel in 0..<channels {
			let source = storage[channel]
			for i in 0..<frames {
				output[start + i * channels + channel] = source[i]
			}
		}
	}

	func withPointers<Result>(_ body: (UnsafePointer<UnsafeMutablePointer<Float>>) -> Result) -> Result {
		storage.withUnsafeBufferPointer { body($0.baseAddress!) }
	}
}

private final class RubberBandBackend: StretchBackend {
	private let state: RubberBandState
	private var options: RubberBandOptions
	private let blockSize: Int
	private var toDrop: Int
	private let buffers: ChannelBuffers

	init?(format: StreamFormat, options: RubberBandOptions, tempo: Double, pitch: Double) {
		guard let state = rubberband_new(UInt32(format.sampleRate), UInt32(format.channels), options, 1 / tempo, pitch) else { return nil }
		self.state = state
		self.options = options
		blockSize = min(Int(rubberband_get_process_size_limit(state)), 4096)
		rubberband_set_max_process_size(state, UInt32(blockSize))
		// The start delay is in input frames (it equals the start pad for both
		// engines); what is dropped is output, so convert it. Dropping it
		// as-is cut or kept too much at any tempo other than 1.
		toDrop = Int((Double(rubberband_get_start_delay(state)) / tempo).rounded())
		buffers = ChannelBuffers(channels: format.channels, capacity: blockSize)

		// Prime with silence as the library prefers, so the start is clean.
		var pad = Int(rubberband_get_preferred_start_pad(state))
		buffers.clear(blockSize)
		while pad > 0 {
			let count = min(pad, blockSize)
			buffers.withPointers { pointers in
				pointers.withMemoryRebound(to: UnsafePointer<Float>?.self, capacity: format.channels) {
					rubberband_process(state, $0, UInt32(count), 0)
				}
			}
			pad -= count
		}
	}

	deinit {
		rubberband_delete(state)
	}

	func set(tempo: Double, pitch: Double, options newOptions: RubberBandOptions) {
		rubberband_set_time_ratio(state, 1 / tempo)
		rubberband_set_pitch_scale(state, pitch)
		let changed = options ^ newOptions
		guard changed != 0 else { return }
		options = newOptions
		let engineR3 = newOptions & rb(RubberBandOptionEngineFiner) != 0
		let transients = rb(RubberBandOptionTransientsCrisp) | rb(RubberBandOptionTransientsMixed) | rb(RubberBandOptionTransientsSmooth)
		let detector = rb(RubberBandOptionDetectorCompound) | rb(RubberBandOptionDetectorPercussive) | rb(RubberBandOptionDetectorSoft)
		let phase = rb(RubberBandOptionPhaseLaminar) | rb(RubberBandOptionPhaseIndependent)
		let formant = rb(RubberBandOptionFormantShifted) | rb(RubberBandOptionFormantPreserved)
		let pitchMask = rb(RubberBandOptionPitchHighSpeed) | rb(RubberBandOptionPitchHighQuality) | rb(RubberBandOptionPitchHighConsistency)
		if changed & transients != 0 { rubberband_set_transients_option(state, newOptions & transients) }
		if !engineR3 {
			if changed & detector != 0 { rubberband_set_detector_option(state, newOptions & detector) }
			if changed & phase != 0 { rubberband_set_phase_option(state, newOptions & phase) }
			if changed & pitchMask != 0 { rubberband_set_pitch_option(state, newOptions & pitchMask) }
		}
		if changed & formant != 0 { rubberband_set_formant_option(state, newOptions & formant) }
	}

	func process(_ input: UnsafeBufferPointer<Float>, into output: inout [Float]) {
		let channels = buffers.channels
		let frames = input.count / channels
		var offset = 0
		while offset < frames {
			let count = min(blockSize, frames - offset)
			buffers.split(input, from: offset, count: count)
			feed(count, final: false)
			offset += count
			retrieve(into: &output)
		}
	}

	func drain(into output: inout [Float]) {
		// In real-time mode the tail keeps coming for a while after the final
		// block: available() says 0 until more is ready and -1 once all of it
		// is out, so keep going until -1.
		buffers.clear(blockSize)
		for _ in 0..<256 {
			feed(0, final: true)
			retrieve(into: &output)
			if rubberband_available(state) < 0 { return }
		}
	}

	private func feed(_ frames: Int, final: Bool) {
		buffers.withPointers { pointers in
			pointers.withMemoryRebound(to: UnsafePointer<Float>?.self, capacity: buffers.channels) {
				rubberband_process(state, $0, UInt32(frames), final ? 1 : 0)
			}
		}
	}

	private func retrieve(into output: inout [Float]) {
		while true {
			let available = Int(rubberband_available(state))
			guard available > 0 else { return }
			let count = min(available, blockSize)
			let got = buffers.withPointers { pointers in
				pointers.withMemoryRebound(to: UnsafeMutablePointer<Float>?.self, capacity: buffers.channels) {
					Int(rubberband_retrieve(state, $0, UInt32(count)))
				}
			}
			if toDrop > 0 {
				// The start delay: output from before the first input frame.
				let drop = min(toDrop, got)
				toDrop -= drop
				if drop == got { continue }
				var kept: [Float] = []
				buffers.join(got, into: &kept)
				output.append(contentsOf: kept[(drop * buffers.channels)...])
				continue
			}
			buffers.join(got, into: &output)
		}
	}

	/// The node's options mapping from the rubberband* settings.
	static func options(from defaults: UserDefaults) -> RubberBandOptions {
		var options = rb(RubberBandOptionProcessRealTime)
		let engine = defaults.string(forKey: "rubberbandEngine")
		let engineR3 = engine == "finer"
		if engine == "faster" { options |= rb(RubberBandOptionEngineFaster) }
		if engineR3 { options |= rb(RubberBandOptionEngineFiner) }
		func pick(_ key: String, _ map: [String: RubberBandOption]) {
			if let value = defaults.string(forKey: key), let flag = map[value] {
				options |= rb(flag)
			}
		}
		if !engineR3 {
			pick("rubberbandTransients", ["crisp": RubberBandOptionTransientsCrisp, "mixed": RubberBandOptionTransientsMixed, "smooth": RubberBandOptionTransientsSmooth])
			pick("rubberbandDetector", ["compound": RubberBandOptionDetectorCompound, "percussive": RubberBandOptionDetectorPercussive, "soft": RubberBandOptionDetectorSoft])
			pick("rubberbandPhase", ["laminar": RubberBandOptionPhaseLaminar, "independent": RubberBandOptionPhaseIndependent])
		}
		pick("rubberbandWindow", ["standard": RubberBandOptionWindowStandard, "short": RubberBandOptionWindowShort,
		                          "long": engineR3 ? RubberBandOptionWindowStandard : RubberBandOptionWindowLong])
		if !engineR3 {
			pick("rubberbandSmoothing", ["off": RubberBandOptionSmoothingOff, "on": RubberBandOptionSmoothingOn])
		}
		pick("rubberbandFormant", ["shifted": RubberBandOptionFormantShifted, "preserved": RubberBandOptionFormantPreserved])
		pick("rubberbandPitch", ["highspeed": RubberBandOptionPitchHighSpeed, "highquality": RubberBandOptionPitchHighQuality, "highconsistency": RubberBandOptionPitchHighConsistency])
		pick("rubberbandChannels", ["apart": RubberBandOptionChannelsApart, "together": RubberBandOptionChannelsTogether])
		return options
	}

	/// Options Rubber Band only takes when it is created.
	static func needsRestart(from old: RubberBandOptions, to new: RubberBandOptions, engine: String) -> Bool {
		guard engine != "signalsmith", old != new else { return false }
		let engineR3 = new & rb(RubberBandOptionEngineFiner) != 0
		var mustRestart = rb(RubberBandOptionEngineFaster) | rb(RubberBandOptionEngineFiner) | rb(RubberBandOptionWindowStandard) | rb(RubberBandOptionWindowShort) |
			rb(RubberBandOptionWindowLong) | rb(RubberBandOptionSmoothingOff) | rb(RubberBandOptionSmoothingOn) | rb(RubberBandOptionChannelsApart) | rb(RubberBandOptionChannelsTogether)
		if engineR3 {
			mustRestart |= rb(RubberBandOptionPitchHighSpeed) | rb(RubberBandOptionPitchHighQuality) | rb(RubberBandOptionPitchHighConsistency)
		}
		return (old ^ new) & mustRestart != 0
	}
}

private final class SignalsmithBackend: StretchBackend {
	private let stretch: OpaquePointer
	private let sampleRate: Double
	private let channels: Int
	private var tempo: Double
	private var seekPending: [Float] = []
	private var seekLength: Int
	private var seeked = false
	private var owedOutput = 0.0
	private var input: ChannelBuffers
	private var output: ChannelBuffers

	init?(format: StreamFormat, tempo: Double, pitch: Double) {
		guard let stretch = cog_signalsmith_create(Int32(format.channels), Float(format.sampleRate)) else { return nil }
		self.stretch = stretch
		sampleRate = format.sampleRate
		channels = format.channels
		self.tempo = tempo
		cog_signalsmith_set_transpose(stretch, Float(pitch), Float(format.sampleRate))
		seekLength = min(Int(cog_signalsmith_output_seek_length(stretch, Float(tempo))), 65536)
		input = ChannelBuffers(channels: channels, capacity: 65536)
		output = ChannelBuffers(channels: channels, capacity: 65536)
	}

	deinit {
		cog_signalsmith_destroy(stretch)
	}

	func set(tempo: Double, pitch: Double, options: RubberBandOptions) {
		self.tempo = tempo
		cog_signalsmith_set_transpose(stretch, Float(pitch), Float(sampleRate))
	}

	func process(_ samples: UnsafeBufferPointer<Float>, into result: inout [Float]) {
		var frames = samples.count / channels
		var offset = 0
		if !seeked {
			// Align the output with the input, as the node did: the first
			// seekLength frames prime the stretcher through outputSeek.
			let take = min(frames, seekLength - seekPending.count / channels)
			seekPending.append(contentsOf: samples[0..<(take * channels)])
			offset = take
			frames -= take
			if seekPending.count / channels < seekLength { return }
			let seekFrames = seekPending.count / channels
			seekPending.withUnsafeBufferPointer { input.split($0, from: 0, count: seekFrames) }
			input.withPointers { pointers in
				pointers.withMemoryRebound(to: UnsafePointer<Float>.self, capacity: channels) {
					cog_signalsmith_output_seek(stretch, $0, Int32(seekFrames))
				}
			}
			seekPending.removeAll()
			seeked = true
		}
		while frames > 0 {
			let count = min(frames, Int(Double(output.capacity) * tempo) - 1, input.capacity)
			input.split(samples, from: offset, count: count)
			owedOutput += Double(count) / tempo
			let outCount = Int(owedOutput)
			owedOutput -= Double(outCount)
			input.withPointers { inPointers in
				output.withPointers { outPointers in
					inPointers.withMemoryRebound(to: UnsafePointer<Float>.self, capacity: channels) { inputs in
						cog_signalsmith_process(stretch, inputs, Int32(count), outPointers, Int32(outCount))
					}
				}
			}
			output.join(outCount, into: &result)
			offset += count
			frames -= count
		}
	}

	func drain(into result: inout [Float]) {
		if !seeked && !seekPending.isEmpty {
			// Shorter than the seek length: seek with what there is.
			let seekFrames = seekPending.count / channels
			seekPending.withUnsafeBufferPointer { input.split($0, from: 0, count: seekFrames) }
			input.withPointers { pointers in
				pointers.withMemoryRebound(to: UnsafePointer<Float>.self, capacity: channels) {
					cog_signalsmith_output_seek(stretch, $0, Int32(seekFrames))
				}
			}
			seekPending.removeAll()
			seeked = true
		}
		// Everything still inside comes out in one flush; the stage trims it
		// to the stretched length.
		let latency = Int(cog_signalsmith_output_latency(stretch)) + Int((Double(cog_signalsmith_input_latency(stretch)) / tempo).rounded())
		guard latency > 0 else { return }
		if latency > output.capacity {
			output = ChannelBuffers(channels: channels, capacity: latency)
		}
		output.withPointers { cog_signalsmith_flush(stretch, $0, Int32(latency)) }
		output.join(latency, into: &result)
	}
}

/// Speed by resampling: soxr in variable-rate mode, with the input/output
/// ratio set to the tempo. Ratio changes glide over a few thousand frames, so
/// moving the slider doesn't click.
private final class VarispeedBackend: StretchBackend {
	/// The fastest ratio soxr is set up for; the sliders stop at 5×.
	private static let maxRatio = 5.0
	private static let minRatio = 0.2
	private static let slewFrames = 4096

	private let resampler: soxr_t
	private let channels: Int
	private var tempo: Double
	private var scratch: [Float] = []

	init?(format: StreamFormat, tempo: Double) {
		var error: soxr_error_t?
		var ioSpec = soxr_io_spec(SOXR_FLOAT32_I, SOXR_FLOAT32_I)
		var qualitySpec = soxr_quality_spec(UInt(SOXR_HQ), UInt(SOXR_VR))
		// Under SOXR_VR the rates only bound the ratio.
		guard let resampler = soxr_create(Self.maxRatio, 1, UInt32(format.channels), &error, &ioSpec, &qualitySpec, nil),
		      error == nil else {
			return nil
		}
		self.resampler = resampler
		channels = format.channels
		self.tempo = Self.clamp(tempo)
		soxr_set_io_ratio(resampler, self.tempo, 0)
	}

	deinit {
		soxr_delete(resampler)
	}

	private static func clamp(_ tempo: Double) -> Double {
		min(max(tempo, minRatio), maxRatio)
	}

	func set(tempo: Double, pitch: Double, options: RubberBandOptions) {
		let tempo = Self.clamp(tempo)
		guard abs(tempo - self.tempo) > 1e-6 else { return }
		self.tempo = tempo
		soxr_set_io_ratio(resampler, tempo, Self.slewFrames)
	}

	func process(_ input: UnsafeBufferPointer<Float>, into output: inout [Float]) {
		let frames = input.count / channels
		var offset = 0
		while offset < frames {
			let remaining = frames - offset
			// Enough for the slowest ratio, plus the filter's own slack.
			let capacity = Int((Double(remaining) / Self.minRatio).rounded(.up)) + 256
			if scratch.count < capacity * channels {
				scratch = [Float](repeating: 0, count: capacity * channels)
			}
			var inputDone = 0
			var outputDone = 0
			scratch.withUnsafeMutableBufferPointer { scratch in
				_ = soxr_process(resampler, input.baseAddress! + offset * channels, remaining, &inputDone,
				                 scratch.baseAddress!, capacity, &outputDone)
			}
			output.append(contentsOf: scratch[0..<(outputDone * channels)])
			offset += inputDone
			if inputDone == 0 && outputDone == 0 {
				break
			}
		}
	}

	func drain(into output: inout [Float]) {
		let capacity = 4096
		if scratch.count < capacity * channels {
			scratch = [Float](repeating: 0, count: capacity * channels)
		}
		// A null input marks the end; soxr then gives up its filter tail.
		while true {
			var outputDone = 0
			scratch.withUnsafeMutableBufferPointer { scratch in
				_ = soxr_process(resampler, nil, 0, nil, scratch.baseAddress!, capacity, &outputDone)
			}
			guard outputDone > 0 else { break }
			output.append(contentsOf: scratch[0..<(outputDone * channels)])
		}
	}
}
