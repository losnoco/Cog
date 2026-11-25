//
//  SpectrumViewSK.swift
//  Cog
//
//  Created by Christopher Snowhill on 2/12/22.
//  Copyright 2022 Christopher Snowhill. All rights reserved.
//
//  The SceneKit spectrum: 11 bars with falling peak dots, drawn from the
//  render thread via the DeaDBeeF analyzer.
//

import AppKit
import CogAudio
import SceneKit
import Accelerate

private let lowerBound: Float = -80

private func visualizerFrameRate() -> Int {
	let rate = UserDefaults.standard.integer(forKey: "visualizerFrameRate")
	return rate > 0 ? min(rate, 120) : 30
}

@objc(SpectrumViewSK)
class SpectrumViewSK: SCNView, SCNSceneRendererDelegate {
	private static let context = UnsafeMutableRawPointer.allocate(byteCount: 1, alignment: 1)

	// Render-thread state: the SceneKit delegate runs off the main actor.
	nonisolated(unsafe) private let visController = VisualizationController.shared()
	nonisolated(unsafe) private var paused = false
	nonisolated(unsafe) private var stopped = false
	nonisolated(unsafe) private var isSetup = false
	@objc nonisolated(unsafe) var isListening = false
	@objc var isWorking = false
	nonisolated(unsafe) private var bandsReset = false
	nonisolated(unsafe) private var cameraControlEnabled = false
	nonisolated(unsafe) private var observersAdded = false
	nonisolated(unsafe) private var isOccluded = false
	nonisolated(unsafe) private var lastTime: TimeInterval = -1

	nonisolated(unsafe) private var analyzer = ddb_analyzer_t()
	nonisolated(unsafe) private var drawData = ddb_analyzer_draw_data_t()
	nonisolated(unsafe) private var visFFTState: UnsafeMutableRawPointer?
	nonisolated(unsafe) private var visPCM = [Float](repeating: 0, count: 4096)
	nonisolated(unsafe) private var visFFT = [Float](repeating: 0, count: 2048)
	nonisolated(unsafe) private var visSamplesLastPosted: UInt64 = 0
	nonisolated(unsafe) private var visLatencyOffset = 0.0

	// Captured at setup so the render thread need not touch the MainActor scene.
	nonisolated(unsafe) private var bandNodes: [SCNNode] = []
	nonisolated(unsafe) private var dotNodes: [SCNNode] = []

	private var cameraPosition2d = SCNVector3Zero
	private var cameraEulerAngles2d = SCNVector3Zero
	private var cameraPosition3d = SCNVector3Zero
	private var cameraEulerAngles3d = SCNVector3Zero

	@objc class func createGuard(withFrame frame: NSRect) -> SpectrumViewSK? {
		if !UserDefaults.standard.bool(forKey: "spectrumSceneKit") { return nil }
		if MTLCreateSystemDefaultDevice() == nil { return nil }
		return SpectrumViewSK(frame: frame)
	}

	override init(frame: NSRect) {
		guard let device = MTLCreateSystemDefaultDevice() else {
			// No Metal: the caller falls back to the CoreGraphics spectrum.
			super.init(frame: frame)
			return
		}
		let options: [String: Any] = [
			SCNView.Option.preferredRenderingAPI.rawValue: SCNRenderingAPI.metal.rawValue,
			SCNView.Option.preferredDevice.rawValue: device,
			SCNView.Option.preferLowPowerDevice.rawValue: false,
		]
		super.init(frame: frame, options: options)
		setup()
	}

	required init?(coder: NSCoder) {
		super.init(coder: coder)
		setup()
	}

	private func updateVisListening() {
		if isListening && (!visibleInWindow || isOccluded || paused || stopped) {
			stopTimer()
			isListening = false
		} else if !isListening && visibleInWindow && !isOccluded && !stopped && !paused {
			startTimer()
			isListening = true
		}
	}

	private func setOccluded(_ occluded: Bool) {
		isOccluded = occluded
		updateVisListening()
	}

	@objc private func windowChangedOcclusionState(_ notification: Notification) {
		guard (notification.object as? NSWindow) === window else { return }
		var curOccluded = window == nil
		if !curOccluded, let window {
			curOccluded = !window.occlusionState.contains(.visible)
		}
		if curOccluded != isOccluded {
			setOccluded(curOccluded)
		}
	}

	override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		if context == Self.context {
			if keyPath == "self.window.visible" {
				updateVisListening()
			} else if keyPath == "values.visualizerFrameRate" {
				if isListening { preferredFramesPerSecond = visualizerFrameRate() }
			} else {
				updateControls()
			}
		} else {
			super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
		}
	}

	private func updateControls() {
		let projectionMode = cameraControlEnabled ? false : UserDefaults.standard.bool(forKey: "spectrumProjectionMode")
		guard let rootNode = scene?.rootNode,
		      let cameraNode = rootNode.childNode(withName: "camera", recursively: false),
		      let camera = cameraNode.camera else { return }
		if projectionMode {
			cameraNode.eulerAngles = cameraEulerAngles2d
			cameraNode.position = cameraPosition2d
			camera.usesOrthographicProjection = true
			camera.orthographicScale = 0.6
		} else {
			cameraNode.eulerAngles = cameraEulerAngles3d
			cameraNode.position = cameraPosition3d
			camera.usesOrthographicProjection = false
			camera.orthographicScale = 1.0
		}

		let transformer = ValueTransformer(forName: NSValueTransformerName("ColorToValueTransformer"))
		let barColor = transformer?.transformedValue(UserDefaults.standard.data(forKey: "spectrumBarColor"))
		let dotColor = transformer?.transformedValue(UserDefaults.standard.data(forKey: "spectrumDotColor"))

		if let barNode = rootNode.childNode(withName: "cylinder0", recursively: false),
		   let material = barNode.geometry?.materials.first {
			material.diffuse.contents = barColor
			material.emission.contents = barColor
		}
		if let dotNode = rootNode.childNode(withName: "sphere0", recursively: false),
		   let material = dotNode.geometry?.materials.first {
			material.diffuse.contents = dotColor
			material.emission.contents = dotColor
		}
	}

	@objc func enableCameraControl() {
		allowsCameraControl = true
		cameraControlEnabled = true
		updateControls()
	}

	private func setup() {
		stopped = true
		paused = false
		isListening = false
		cameraControlEnabled = false

		backgroundColor = .clear

		let theScene = SCNScene(named: "Scenes.scnassets/Spectrum.scn")
		scene = theScene

		antialiasingMode = .multisampling8X

		if let rootNode = scene?.rootNode {
			let nodes = rootNode.childNodes
			if nodes.count >= 23 {
				bandNodes = (0..<11).map { nodes[$0 + 1] }
				dotNodes = (0..<11).map { nodes[$0 + 1 + 11] }
			}
			if let cameraNode = rootNode.childNode(withName: "camera", recursively: false) {
				cameraPosition2d = SCNVector3(0.0, 0.5, 1.0)
				cameraEulerAngles2d = SCNVector3Zero
				cameraPosition3d = cameraNode.position
				cameraEulerAngles3d = cameraNode.eulerAngles
			}
		}
		updateControls()

		bandsReset = false
		drawBaseBands()

		let freqMode = UserDefaults.standard.bool(forKey: "spectrumFreqMode")

		ddb_analyzer_init(&analyzer)
		analyzer.db_lower_bound = lowerBound
		analyzer.min_freq = 10
		analyzer.max_freq = 22000
		analyzer.peak_hold = 10
		analyzer.view_width = 1000
		analyzer.fractional_bars = 1
		analyzer.octave_bars_step = 2
		analyzer.max_of_stereo_data = 1
		analyzer.freq_is_log = 0
		analyzer.mode = freqMode ? DDB_ANALYZER_MODE_FREQUENCIES : DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS

		delegate = self

		addPlaybackObservers()

		isSetup = true
	}

	private func addPlaybackObservers() {
		let center = NotificationCenter.default
		center.addObserver(self, selector: #selector(playbackDidBegin(_:)), name: .CogPlaybackDidBeginNotificiation, object: nil)
		center.addObserver(self, selector: #selector(playbackDidPause(_:)), name: .CogPlaybackDidPauseNotificiation, object: nil)
		center.addObserver(self, selector: #selector(playbackDidResume(_:)), name: .CogPlaybackDidResumeNotificiation, object: nil)
		center.addObserver(self, selector: #selector(playbackDidStop(_:)), name: .CogPlaybackDidStopNotificiation, object: nil)
	}

	nonisolated private func removePlaybackObservers() {
		let center = NotificationCenter.default
		center.removeObserver(self, name: .CogPlaybackDidBeginNotificiation, object: nil)
		center.removeObserver(self, name: .CogPlaybackDidPauseNotificiation, object: nil)
		center.removeObserver(self, name: .CogPlaybackDidResumeNotificiation, object: nil)
		center.removeObserver(self, name: .CogPlaybackDidStopNotificiation, object: nil)
	}

	private func addObservers() {
		if !observersAdded {
			let defaults = NSUserDefaultsController.shared
			defaults.addObserver(self, forKeyPath: "values.spectrumProjectionMode", options: [], context: Self.context)
			defaults.addObserver(self, forKeyPath: "values.spectrumBarColor", options: [], context: Self.context)
			defaults.addObserver(self, forKeyPath: "values.spectrumDotColor", options: [], context: Self.context)
			defaults.addObserver(self, forKeyPath: "values.visualizerFrameRate", options: [], context: Self.context)

			addObserver(self, forKeyPath: "self.window.visible", options: [], context: Self.context)

			NotificationCenter.default.addObserver(self, selector: #selector(windowChangedOcclusionState(_:)), name: NSWindow.didChangeOcclusionStateNotification, object: nil)

			observersAdded = true
		}
	}

	deinit {
		ddb_analyzer_dealloc(&analyzer)
		ddb_analyzer_draw_data_dealloc(&drawData)
		stopTimer()
		removePlaybackObservers()
		removeObservers()
		visController.freeFFTState(&visFFTState)
	}

	nonisolated private func removeObservers() {
		if observersAdded {
			let defaults = NSUserDefaultsController.shared
			defaults.removeObserver(self, forKeyPath: "values.spectrumProjectionMode", context: Self.context)
			defaults.removeObserver(self, forKeyPath: "values.spectrumBarColor", context: Self.context)
			defaults.removeObserver(self, forKeyPath: "values.spectrumDotColor", context: Self.context)
			defaults.removeObserver(self, forKeyPath: "values.visualizerFrameRate", context: Self.context)

			removeObserver(self, forKeyPath: "self.window.visible", context: Self.context)

			NotificationCenter.default.removeObserver(self, name: NSWindow.didChangeOcclusionStateNotification, object: nil)

			observersAdded = false
		}
	}

	nonisolated func renderer(_ renderer: SCNSceneRenderer, updateAtTime time: TimeInterval) {
		if !isSetup { return }

		// Need the window to exist first before we can register for visibility
		if !observersAdded {
			DispatchQueue.main.async { self.addObservers() }
		}

		DispatchQueue.main.async { self.updateVisListening() }

		if stopped {
			drawBaseBands()
			return
		}

		if lastTime >= 0 {
			let delta = time - lastTime
			if delta < 1.0 {
				visLatencyOffset -= delta
			} else {
				visLatencyOffset = 0.0 // delay in rendering
			}
		} else {
			visLatencyOffset = 0.0
		}
		lastTime = time

		let samplesPosted = visController.samplesPosted()
		if samplesPosted != visSamplesLastPosted {
			visSamplesLastPosted = samplesPosted
			visLatencyOffset = 0.0
		}

		visController.copyVisPCM(&visPCM, visFFT: &visFFT, visFFTState: &visFFTState, latencyOffset: visLatencyOffset)

		ddb_analyzer_process(&analyzer, Int32(visController.readSampleRate() / 2.0), 1, &visFFT, 2048)
		ddb_analyzer_tick(&analyzer)
		ddb_analyzer_get_draw_data(&analyzer, 11, 1, &drawData)

		drawAnalyzer()
	}

	private func startTimer() {
		lastTime = -1
		preferredFramesPerSecond = visualizerFrameRate()
		rendersContinuously = true
	}

	nonisolated private func stopTimer() {
		lastTime = -1
		DispatchQueue.main.async {
			self.preferredFramesPerSecond = 1
			self.rendersContinuously = false
		}
	}

	@objc func startPlayback() {
		lastTime = -1
		playbackDidBegin(nil)
	}

	@objc private func playbackDidBegin(_ notification: Notification?) {
		stopped = false
		paused = false
		updateVisListening()
	}

	@objc private func playbackDidPause(_ notification: Notification?) {
		lastTime = -1
		stopped = false
		paused = true
		updateVisListening()
	}

	@objc private func playbackDidResume(_ notification: Notification?) {
		lastTime = -1
		stopped = false
		paused = false
		updateVisListening()
	}

	@objc private func playbackDidStop(_ notification: Notification?) {
		stopped = true
		paused = false
		bandsReset = false
		updateVisListening()
		needsDisplay = true
		lastTime = -1
	}

	nonisolated private func drawBaseBands() {
		if bandsReset { return }
		guard bandNodes.count == 11, dotNodes.count == 11 else { return }

		for i in 0..<11 {
			let node = bandNodes[i]
			let dotNode = dotNodes[i]
			var position = node.position
			position.y = 0.0
			node.scale = SCNVector3(1.0, 0.0, 1.0)
			node.position = position

			position = dotNode.position
			position.y = 0.072
			dotNode.position = position
		}

		bandsReset = true
	}

	nonisolated private func drawAnalyzerOctaveBands() {
		let maxBars = Int(ceilf(Float(drawData.bar_count) / 11.0))
		let barStep = Int(floorf(Float(drawData.bar_count) / 11.0))
		guard let bar = drawData.bars, bandNodes.count == 11, dotNodes.count == 11 else { return }

		let stride = MemoryLayout<ddb_analyzer_draw_bar_t>.stride / MemoryLayout<Float>.stride
		let heightOff = MemoryLayout<ddb_analyzer_draw_bar_t>.offset(of: \.bar_height)! / MemoryLayout<Float>.stride
		let peakOff = MemoryLayout<ddb_analyzer_draw_bar_t>.offset(of: \.peak_ypos)! / MemoryLayout<Float>.stride

		for i in 0..<11 {
			var maxValue: Float = 0.0
			var maxMax: Float = 0.0
			let barBase = i * barStep
			let barEnd = min(barBase + maxBars, Int(drawData.bar_count))
			let barCount = barEnd - barBase
			if barCount > 0 {
				let baseRaw = UnsafeRawPointer(bar.advanced(by: barBase))
				baseRaw.advanced(by: heightOff * MemoryLayout<Float>.stride).withMemoryRebound(to: Float.self, capacity: barCount * stride) { ptr in
					vDSP_maxv(ptr, vDSP_Stride(stride), &maxValue, vDSP_Length(barCount))
				}
				baseRaw.advanced(by: peakOff * MemoryLayout<Float>.stride).withMemoryRebound(to: Float.self, capacity: barCount * stride) { ptr in
					vDSP_maxv(ptr, vDSP_Stride(stride), &maxMax, vDSP_Length(barCount))
				}
			}
			let node = bandNodes[i]
			let dotNode = dotNodes[i]
			var position = node.position
			position.y = CGFloat(maxValue * 0.5)
			node.scale = SCNVector3(1.0, CGFloat(maxValue), 1.0)
			node.position = position

			position = dotNode.position
			position.y = CGFloat(maxMax) + 0.072
			dotNode.position = position
		}

		bandsReset = false
	}

	nonisolated private func drawAnalyzer() {
		drawAnalyzerOctaveBands()
	}

	override func mouseDown(with event: NSEvent) {
		if cameraControlEnabled { return }

		let freqMode = !UserDefaults.standard.bool(forKey: "spectrumFreqMode")
		UserDefaults.standard.set(freqMode, forKey: "spectrumFreqMode")

		analyzer.mode = freqMode ? DDB_ANALYZER_MODE_FREQUENCIES : DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS
		analyzer.mode_did_change = 1

		needsDisplay = true
	}
}