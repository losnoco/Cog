//
//  InfoInspectorView.swift
//  Cog
//
//  Created by Kevin López Brante on 2026-10-02.
//

import SwiftUI

@MainActor
final class InfoInspectorModel: ObservableObject {
	@Published var entry: PlaylistEntry?
}

struct InfoInspectorView: View {
	@ObservedObject var model: InfoInspectorModel

	var body: some View {
		Group {
			if let entry = model.entry {
				EntryDetails(entry: entry)
			} else {
				EntryDetailsContent(entry: nil)
			}
		}
		.frame(minWidth: 240, maxWidth: .infinity, minHeight: 594, maxHeight: .infinity, alignment: .top)
	}
}

/// Observes the entry itself, so late-loading metadata, ReplayGain and album
/// art refresh the inspector the way the old Cocoa bindings did.
private struct EntryDetails: View {
	@ObservedObject var entry: PlaylistEntry

	var body: some View {
		EntryDetailsContent(entry: entry)
	}
}

private struct EntryDetailsContent: View {
	let entry: PlaylistEntry?

	@State private var labelWidth: CGFloat = 0

	var body: some View {
		VStack(alignment: .leading, spacing: 4) {
			// PlaylistEntry's Objective-C category header can't be imported
			// into Swift, so its derived properties are read through KVC.
			row("Album Artist:", "albumartist")
			row("Artist:", "artist")
			row("Composer:", "composer")
			row("Album:", "album")
			row("Title:", "title")
			row("Track:", "trackText")
			row("Length:", "lengthText", help: "lengthInfo")
			row("Date:", "date")
			row("Genre:", "genre")
			row("Filename:", "filename")
			InfoRow(label: "Sample Rate:", value: sampleRate, labelWidth: labelWidth)
			row("Channels:", "channels")
			row("Bitrate:", "bitrate")
			row("Bits Per Sample:", "bitsPerSample")
			row("Codec:", "codec")
			row("Encoding:", "encoding")
			row("Cuesheet:", "cuesheetPresent")
			row("ReplayGain:", "gainCorrection", help: "gainInfo")
			row("Play Count:", "playCount", help: "playCountInfo")
			row("Comment:", "comment")

			AlbumArtView(image: entry?.value(forKey: "albumArt") as? NSImage)
				.padding(.top, 4)
		}
		.font(.system(size: NSFont.smallSystemFontSize))
		.padding(.horizontal, 8)
		.padding(.vertical, 6)
		.onPreferenceChange(LabelWidthKey.self) { labelWidth = $0 }
	}

	private func row(_ label: LocalizedStringKey, _ key: String, help helpKey: String? = nil) -> some View {
		InfoRow(label: label, value: string(for: key), help: helpKey.map(string(for:)), labelWidth: labelWidth)
	}

	private func string(for key: String) -> String {
		guard let value = entry?.value(forKey: key) else { return "" }
		return (value as? String) ?? String(describing: value)
	}

	private var sampleRate: String {
		guard let value = entry?.value(forKey: "sampleRate") else { return "" }
		return NumberHertzToStringTransformer().transformedValue(value) as? String ?? ""
	}
}

private struct InfoRow: View {
	let label: LocalizedStringKey
	let value: String
	var help: String?
	let labelWidth: CGFloat

	var body: some View {
		HStack(alignment: .firstTextBaseline, spacing: 8) {
			Text(label)
				.lineLimit(1)
				.fixedSize()
				.background(GeometryReader { proxy in
					Color.clear.preference(key: LabelWidthKey.self, value: proxy.size.width)
				})
				.frame(width: labelWidth > 0 ? labelWidth : nil, alignment: .trailing)
			Text(value)
				.lineLimit(1)
				.truncationMode(.middle)
				.textSelection(.enabled)
				.frame(maxWidth: .infinity, alignment: .leading)
				.help(help ?? value)
		}
	}
}

private struct LabelWidthKey: PreferenceKey {
	static let defaultValue: CGFloat = 0

	static func reduce(value: inout CGFloat, nextValue: () -> CGFloat) {
		value = max(value, nextValue())
	}
}

/// Scales the art down to fit, never up, falling back to the missing-art image.
private struct AlbumArtView: View {
	let image: NSImage?

	var body: some View {
		let art = image ?? NSImage(named: "missingArt") ?? NSImage()
		Image(nsImage: art)
			.resizable()
			.aspectRatio(contentMode: .fit)
			.frame(maxWidth: art.size.width, maxHeight: art.size.height)
			.frame(maxWidth: .infinity, maxHeight: .infinity)
	}
}

#Preview {
	InfoInspectorView(model: InfoInspectorModel())
}
