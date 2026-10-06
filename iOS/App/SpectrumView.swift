//
//  SpectrumView.swift
//  Cog (iOS)
//
//  The spectrum, as the macOS one draws it (Visualization/SpectrumViewCG.m):
//  the audio the engine posts to VisualizationController, through
//  DeaDBeeF's analyzer in octave note bands, bars with falling peaks.
//

import CogAudio
import SwiftUI

/// The analyzer's state from one frame to the next.
private final class Analyzer {
	private var analyzer = ddb_analyzer_t()
	private var drawData = ddb_analyzer_draw_data_t()
	private var pcm = [Float](repeating: 0, count: 4096)
	private var fft = [Float](repeating: 0, count: 2048)
	private var fftState: UnsafeMutableRawPointer?
	private let controller = VisualizationController.shared()
	/// How far past the newest audio posted the display has got: the
	/// engine posts in blocks, and frames fall between them.
	private var latencyOffset = 0.0
	private var lastPosted: UInt64 = 0
	private var lastFrame: Date?

	init() {
		ddb_analyzer_init(&analyzer)
		analyzer.db_lower_bound = -80
		analyzer.min_freq = 10
		analyzer.max_freq = 22000
		analyzer.peak_hold = 10
		analyzer.view_width = 64
		analyzer.fractional_bars = 1
		analyzer.octave_bars_step = 2
		analyzer.max_of_stereo_data = 1
		analyzer.freq_is_log = 0
		analyzer.mode = DDB_ANALYZER_MODE_OCTAVE_NOTE_BANDS
	}

	deinit {
		controller.freeFFTState(&fftState)
		ddb_analyzer_dealloc(&analyzer)
		ddb_analyzer_draw_data_dealloc(&drawData)
	}

	/// Advances to `date` and draws into `context`, bars from the bottom.
	func draw(in context: inout GraphicsContext, size: CGSize, at date: Date) {
		let posted = controller.samplesPosted()
		if posted != lastPosted {
			lastPosted = posted
			latencyOffset = 0
		} else if let lastFrame {
			latencyOffset -= date.timeIntervalSince(lastFrame)
		}
		lastFrame = date

		controller.copyVisPCM(&pcm, visFFT: &fft, visFFTState: &fftState, latencyOffset: latencyOffset)
		ddb_analyzer_process(&analyzer, Int32(controller.readSampleRate() / 2), 1, &fft, 2048)
		ddb_analyzer_tick(&analyzer)
		ddb_analyzer_get_draw_data(&analyzer, Int32(size.width), Int32(size.height), &drawData)

		guard let bars = drawData.bars else { return }
		let width = CGFloat(drawData.bar_width)
		var body = Path()
		var peaks = Path()
		for bar in UnsafeBufferPointer(start: bars, count: Int(drawData.bar_count)) {
			let x = CGFloat(bar.xpos)
			body.addRect(CGRect(x: x, y: size.height - CGFloat(bar.bar_height), width: width, height: CGFloat(bar.bar_height)))
			peaks.addRect(CGRect(x: x, y: size.height - CGFloat(bar.peak_ypos) - 1, width: width, height: 1.5))
		}
		context.fill(body, with: .color(.white.opacity(0.75)))
		context.fill(peaks, with: .color(.white))
	}
}

struct SpectrumView: View {
	/// Still while paused: no frames are drawn then.
	var isPlaying: Bool
	@State private var analyzer = Analyzer()

	var body: some View {
		TimelineView(.animation(minimumInterval: 1.0 / 60.0, paused: !isPlaying)) { timeline in
			Canvas { context, size in
				analyzer.draw(in: &context, size: size, at: timeline.date)
			}
		}
		.accessibilityHidden(true)
	}
}
