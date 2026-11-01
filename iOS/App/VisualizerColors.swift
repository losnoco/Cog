//
//  VisualizerColors.swift
//  Cog (iOS)
//
//  The visualizer's colors: the album's own unless one is chosen in
//  Settings. The keys are macOS's, though each platform keeps its own.
//

import SwiftUI
import UIKit

enum VisualizerColor: String, CaseIterable {
	case spectrumBars = "spectrumBarColor"
	case spectrumPeaks = "spectrumDotColor"
	case waveform = "waveformVisualizerColor"

	/// The chosen color in `data`, or else the album's: its accent for the
	/// bars and the waveform, white for the peaks, as before colors could be
	/// chosen.
	func resolved(_ data: Data?, palette: ArtworkPalette?) -> Color {
		if let color = Self.color(from: data) {
			return color
		}
		switch self {
		case .spectrumBars, .waveform: return (palette?.accent ?? .white).opacity(0.8)
		case .spectrumPeaks: return .white
		}
	}

	static func color(from data: Data?) -> Color? {
		guard let data,
		      let color = try? NSKeyedUnarchiver.unarchivedObject(ofClass: UIColor.self, from: data) else { return nil }
		return Color(uiColor: color)
	}

	static func data(from color: Color) -> Data? {
		try? NSKeyedArchiver.archivedData(withRootObject: UIColor(color), requiringSecureCoding: true)
	}
}

/// A color for the visualizer, matching the album's art until one is chosen.
struct VisualizerColorPicker: View {
	var title: LocalizedStringKey
	@AppStorage private var data: Data?

	init(_ title: LocalizedStringKey, color: VisualizerColor) {
		self.title = title
		_data = AppStorage(color.rawValue)
	}

	var body: some View {
		Toggle(isOn: matchesArt) {
			Text(title)
			Text("Match Album Art")
		}
		if data != nil {
			ColorPicker(selection: chosen, supportsOpacity: true) {
				Text("Color")
			}
		}
	}

	private var matchesArt: Binding<Bool> {
		Binding(get: { data == nil }, set: { matches in
			// White to start from: what each looks like without art.
			data = matches ? nil : VisualizerColor.data(from: .white)
		})
	}

	private var chosen: Binding<Color> {
		Binding(get: { VisualizerColor.color(from: data) ?? .white }, set: { data = VisualizerColor.data(from: $0) })
	}
}
