//
//  SpectrumViewCG.swift
//  Cog
//
//  Created by Christopher Snowhill on 2/12/22.
//  Copyright 2022 Christopher Snowhill. All rights reserved.
//
//  The CoreGraphics spectrum, drawn on the main thread from a timer.
//

import AppKit
import CogAudio

private let lowerBound: Float = -80

private func visualizerFrameRate() -> Int {
	let rate = UserDefaults.standard.integer(forKey: "visualizerFrameRate")
	return rate > 0 ? min(rate, 120) : 30
}

@objc(SpectrumViewCG)
class SpectrumViewCG: NSView {
	private static let context = UnsafeMutableRawPointer.allocate(byteCount: 1, alignment: 1)

	nonisolated(unsafe) private let visController = VisualizationController.shared()
	nonisolated(unsafe) private var timer: Timer?
	private var frameInterval: TimeInterval = 0
	private var paused = false
	private var stopped = false
	private var isSetup = false
	@objc var isListening = false
	nonisolated(unsafe) private var observersAdded = false
	nonisolated(unsafe) private var defaultsController: NSUserDefaultsController?
	private var isFullView = false
	private var isOccluded = false

	private var textAttrs: [NSAttributedString.Key: Any] = [:]
	private var baseColor = NSColor.clear
	private var peakColor = NSColor.clear
	private var backgroundColor = NSColor.clear
	private var borderColor = NSColor.clear
	nonisolated(unsafe) private var analyzer = ddb_analyzer_t()
	nonisolated(unsafe) private var drawData = ddb_analyzer_draw_data_t()

	nonisolated(unsafe) private var visFFTState: UnsafeMutableRawPointer?
	private var visPCM = [Float](repeating: 0, count: 4096)
	private var visFFT = [Float](repeating: 0, count: 2048)

	private var visSamplesLastPosted: UInt64 = 0
	private var visLatencyOffset = 0.0

	override init(frame frameRect: NSRect) {
		super.init(frame: frameRect)
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

	private func setup() {
		stopped = true
		paused = false
		isListening = false

		colorsDidChange(nil)

		let freqMode = UserDefaults.standard.bool(forKey: "spectrumFreqMode")

		ddb_analyzer_init(&analyzer)
		analyzer.db_lower_bound = lowerBound
		analyzer.min_freq = 10
		analyzer.max_freq = 22000
		analyzer.peak_hold = 10
		analyzer.view_width = 64
		analyzer.fractional_bars = 1
		analyzer.octave_bars_step = 2
		analyzer.max_of_stereo_data = 1
		analyzer.freq_is_log = 0
		analyzer.mode = freqMode ? DDB_ANALYZER_MODE_FREQUENCIES : DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS

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

	@objc func enableFullView() {
		isFullView = true
		analyzer.freq_is_log = 1
		repaint()
	}

	override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		if context == Self.context {
			if keyPath == "self.window.visible" {
				updateVisListening()
			} else if keyPath == "values.spectrumFreqMode" {
				freqModeDidChange()
			} else if keyPath == "values.visualizerFrameRate" {
				if isListening { startTimer() }
			} else {
				colorsDidChange(nil)
			}
		} else {
			super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
		}
	}

	private func addObservers() {
		if !observersAdded {
			let defaults = NSUserDefaultsController.shared
			defaultsController = defaults
			defaults.addObserver(self, forKeyPath: "values.spectrumBarColor", options: [], context: Self.context)
			defaults.addObserver(self, forKeyPath: "values.spectrumFreqMode", options: [], context: Self.context)
			defaults.addObserver(self, forKeyPath: "values.spectrumDotColor", options: [], context: Self.context)
			defaults.addObserver(self, forKeyPath: "values.visualizerFrameRate", options: [], context: Self.context)

			addObserver(self, forKeyPath: "self.window.visible", options: [], context: Self.context)

			let center = NotificationCenter.default
			center.addObserver(self, selector: #selector(colorsDidChange(_:)), name: NSColor.systemColorsDidChangeNotification, object: nil)
			center.addObserver(self, selector: #selector(windowChangedOcclusionState(_:)), name: NSWindow.didChangeOcclusionStateNotification, object: nil)

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
			let defaults = defaultsController
			defaults?.removeObserver(self, forKeyPath: "values.spectrumBarColor", context: Self.context)
			defaults?.removeObserver(self, forKeyPath: "values.spectrumFreqMode", context: Self.context)
			defaults?.removeObserver(self, forKeyPath: "values.spectrumDotColor", context: Self.context)
			defaults?.removeObserver(self, forKeyPath: "values.visualizerFrameRate", context: Self.context)

			removeObserver(self, forKeyPath: "self.window.visible", context: Self.context)

			let center = NotificationCenter.default
			center.removeObserver(self, name: NSColor.systemColorsDidChangeNotification, object: nil)
			center.removeObserver(self, name: NSWindow.didChangeOcclusionStateNotification, object: nil)

			observersAdded = false
		}
	}

	private func repaint() {
		needsDisplay = true
	}

	private func startTimer() {
		stopTimer()
		visLatencyOffset = 0.0
		frameInterval = 1.0 / Double(visualizerFrameRate())
		let t = Timer(timeInterval: frameInterval, target: self, selector: #selector(timerRun(_:)), userInfo: nil, repeats: true)
		RunLoop.current.add(t, forMode: .common)
		timer = t
	}

	nonisolated private func stopTimer() {
		timer?.invalidate()
		timer = nil
	}

	@objc private func timerRun(_ timer: Timer) {
		repaint()
		visLatencyOffset -= frameInterval
	}

	@objc private func colorsDidChange(_ notification: Notification?) {
		backgroundColor = NSColor.textBackgroundColor.withAlphaComponent(0.0)
		borderColor = NSColor.systemGray

		let transformer = ValueTransformer(forName: NSValueTransformerName("ColorToValueTransformer"))
		baseColor = (transformer?.transformedValue(UserDefaults.standard.data(forKey: "spectrumBarColor")) as? NSColor) ?? .clear
		peakColor = (transformer?.transformedValue(UserDefaults.standard.data(forKey: "spectrumDotColor")) as? NSColor) ?? .clear

		let paragraphStyle = NSMutableParagraphStyle()
		paragraphStyle.alignment = .left

		textAttrs = [
			.font: NSFont(name: "HelveticaNeue", size: 10) ?? NSFont.systemFont(ofSize: 10),
			.paragraphStyle: paragraphStyle,
			.foregroundColor: borderColor,
		]

		repaint()
	}

	@objc func startPlayback() {
		playbackDidBegin(nil)
	}

	@objc private func playbackDidBegin(_ notification: Notification?) {
		stopped = false
		paused = false
		updateVisListening()
	}

	@objc private func playbackDidPause(_ notification: Notification?) {
		stopped = false
		paused = true
		updateVisListening()
	}

	@objc private func playbackDidResume(_ notification: Notification?) {
		stopped = false
		paused = false
		updateVisListening()
	}

	@objc private func playbackDidStop(_ notification: Notification?) {
		stopped = true
		paused = false
		updateVisListening()
		repaint()
	}

	private func drawSaGrid(in context: CGContext) {
		context.setStrokeColor(borderColor.cgColor)
		let lower = -floor(CGFloat(lowerBound))
		var db: CGFloat = 10
		while db < lower {
			let y = (db / lower) * bounds.height
			if y >= bounds.height { break }
			context.move(to: CGPoint(x: 0, y: y))
			context.addLine(to: CGPoint(x: bounds.width - 1, y: y))
			db += 10
		}
		context.setLineDash(phase: 0, lengths: [1, 2])
		context.strokePath()
		context.setLineDash(phase: 0, lengths: [])

		db = 10
		while db < lower {
			let y = (db / lower) * bounds.height
			if y >= bounds.height { break }
			let string = "\(Int(-db)) dB" as NSString
			string.draw(at: NSPoint(x: 0, y: bounds.height - y - 12), withAttributes: textAttrs)
			db += 10
		}
	}

	private func drawFrequencyLabels() {
		let count = Int(drawData.label_freq_count)
		let positions = withUnsafeBytes(of: drawData.label_freq_positions) { Array($0.bindMemory(to: Float.self).prefix(count)) }
		let texts = withUnsafeBytes(of: drawData.label_freq_texts) { bytes in
			(0..<count).map { index in
				String(decoding: bytes[(index * 4)..<(index * 4 + 4)].prefix { $0 != 0 }, as: UTF8.self)
			}
		}
		for (position, text) in zip(positions, texts) where position >= 0 {
			(text as NSString).draw(at: NSPoint(x: CGFloat(position), y: bounds.height - 12), withAttributes: textAttrs)
		}
	}

	private func drawAnalyzerDiscreteFrequencies(in context: CGContext) {
		guard let bars = drawData.bars else { return }
		let drawn = UnsafeBufferPointer(start: bars, count: Int(drawData.bar_count))
		for bar in drawn {
			context.move(to: CGPoint(x: CGFloat(bar.xpos), y: 0))
			context.addLine(to: CGPoint(x: CGFloat(bar.xpos), y: CGFloat(bar.bar_height)))
		}
		context.setStrokeColor(baseColor.cgColor)
		context.strokePath()

		for bar in drawn {
			context.move(to: CGPoint(x: CGFloat(bar.xpos) - 0.5, y: CGFloat(bar.peak_ypos)))
			context.addLine(to: CGPoint(x: CGFloat(bar.xpos) + 0.5, y: CGFloat(bar.peak_ypos)))
		}
		context.setStrokeColor(peakColor.cgColor)
		context.strokePath()
	}

	private func drawAnalyzerOctaveBands(in context: CGContext) {
		guard let bars = drawData.bars else { return }
		let drawn = UnsafeBufferPointer(start: bars, count: Int(drawData.bar_count))
		let width = CGFloat(drawData.bar_width)
		for bar in drawn {
			context.addRect(CGRect(x: CGFloat(bar.xpos), y: 0, width: width, height: CGFloat(bar.bar_height)))
		}
		context.setFillColor(baseColor.cgColor)
		context.fillPath()

		for bar in drawn {
			context.addRect(CGRect(x: CGFloat(bar.xpos), y: CGFloat(bar.peak_ypos), width: width, height: 1.0))
		}
		context.setFillColor(peakColor.cgColor)
		context.fillPath()
	}

	private func drawAnalyzer(in context: CGContext) {
		if analyzer.mode == DDB_ANALYZER_MODE_FREQUENCIES {
			drawAnalyzerDiscreteFrequencies(in: context)
		} else {
			drawAnalyzerOctaveBands(in: context)
		}
	}

	override func draw(_ dirtyRect: NSRect) {
		super.draw(dirtyRect)

		if !isSetup { return }

		// Need to delay this to add the window visibility observer
		if !observersAdded {
			addObservers()
		}

		updateVisListening()

		backgroundColor.setFill()
		dirtyRect.fill()

		if stopped { return }

		if isFullView {
			analyzer.view_width = Int32(bounds.size.width)
		}

		let samplesPosted = visController.samplesPosted()
		if samplesPosted != visSamplesLastPosted {
			visSamplesLastPosted = samplesPosted
			visLatencyOffset = 0.0
		}

		visController.copyVisPCM(&visPCM, visFFT: &visFFT, visFFTState: &visFFTState, latencyOffset: visLatencyOffset)

		ddb_analyzer_process(&analyzer, Int32(visController.readSampleRate() / 2.0), 1, &visFFT, 2048)
		ddb_analyzer_tick(&analyzer)
		ddb_analyzer_get_draw_data(&analyzer, Int32(bounds.size.width), Int32(bounds.size.height), &drawData)

		guard let context = NSGraphicsContext.current?.cgContext else { return }

		if isFullView {
			drawSaGrid(in: context)
			drawFrequencyLabels()
		}

		drawAnalyzer(in: context)
	}

	// The spectrum window's menu switches the mode; the toolbar's follows it.
	private func freqModeDidChange() {
		let freqMode = UserDefaults.standard.bool(forKey: "spectrumFreqMode")
		analyzer.mode = freqMode ? DDB_ANALYZER_MODE_FREQUENCIES : DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS
		analyzer.mode_did_change = 1
		repaint()
	}
}