//
//  InfoInspectorView.swift
//  Cog
//
//  Created by Kevin López Brante on 2026-10-02.
//

import CogPlaylist
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
	@AppStorage("infoInspectorShowsRawTags") private var showsRawTags = false

	var body: some View {
		VStack(alignment: .leading, spacing: 4) {
			Toggle("Show raw tags", isOn: $showsRawTags)
				.toggleStyle(.checkbox)
				.controlSize(.small)
				.help("Show every tag under the name it was read by, in place of the named fields.")
				.padding(.bottom, 2)

			// PlaylistEntry's Objective-C category header can't be imported
			// into Swift, so its derived properties are read through KVC.
			if showsRawTags {
				rawTagRows
				Divider()
					.padding(.vertical, 2)
				row("Length:", "lengthText", help: "lengthInfo")
				fileRows
			} else {
				row("Album Artist:", "albumartist")
				row("Artist:", "artist")
				row("Composer:", "composer")
				row("Album:", "album")
				row("Title:", "title")
				row("Track:", "trackText")
				row("Length:", "lengthText", help: "lengthInfo")
				row("Date:", "date")
				row("Genre:", "genre")
				fileRows
				row("Comment:", "comment")
			}

			AlbumArtView(image: entry?.value(forKey: "albumArt") as? NSImage)
				.padding(.top, 4)
		}
		.font(.system(size: NSFont.smallSystemFontSize))
		.padding(.horizontal, 8)
		.padding(.vertical, 6)
		.onPreferenceChange(LabelWidthKey.self) { labelWidth = $0 }
	}

	/// What the file is, as opposed to what its tags say.
	@ViewBuilder private var fileRows: some View {
		row("Filename:", "filename")
		InfoRow(label: Text("Sample Rate:"), value: sampleRate, labelWidth: labelWidth)
		row("Channels:", "channels")
		row("Bitrate:", "bitrate")
		row("Bits Per Sample:", "bitsPerSample")
		row("Codec:", "codec")
		row("Encoding:", "encoding")
		row("Cuesheet:", "cuesheetPresent")
		row("ReplayGain:", "gainCorrection", help: "gainInfo")
		row("Play Count:", "playCount", help: "playCountInfo")
	}

	/// One row per tag the entry holds, under the name its reader gave it,
	/// which the named fields above fold together or leave out.
	@ViewBuilder private var rawTagRows: some View {
		let tags = rawTags
		if tags.isEmpty {
			Text("No tags")
				.foregroundColor(.secondary)
		} else {
			ForEach(tags, id: \.name) { tag in
				InfoRow(label: Text(verbatim: tag.name), value: tag.value, labelWidth: labelWidth, truncatesLabel: true)
			}
		}
	}

	private var rawTags: [(name: String, value: String)] {
		guard let blob = entry?.value(forKey: "metadataBlob") as? [String: Any] else { return [] }
		return blob.map { key, value in
			let values = (value as? [Any])?.map { "\($0)" } ?? ["\(value)"]
			return (name: PlaylistEntry.metaTag(forKey: key), value: values.joined(separator: ", "))
		}
		.sorted { $0.name.localizedStandardCompare($1.name) == .orderedAscending }
	}

	private func row(_ label: LocalizedStringKey, _ key: String, help helpKey: String? = nil) -> some View {
		InfoRow(label: Text(label), value: string(for: key), help: helpKey.map(string(for:)), labelWidth: labelWidth)
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
	let label: Text
	let value: String
	var help: String?
	let labelWidth: CGFloat
	/// A raw tag's name can be any length; it is cut short where a field's
	/// label, which sets the column's width, never is.
	var truncatesLabel = false

	var body: some View {
		HStack(alignment: .firstTextBaseline, spacing: 8) {
			labelView
				.frame(width: labelWidth > 0 ? labelWidth : nil, alignment: .trailing)
			Text(value)
				.lineLimit(1)
				.truncationMode(.middle)
				.textSelection(.enabled)
				.frame(maxWidth: .infinity, alignment: .leading)
				.help(help ?? value)
		}
	}

	@ViewBuilder private var labelView: some View {
		if truncatesLabel {
			label
				.lineLimit(1)
				.truncationMode(.tail)
				.textSelection(.enabled)
				.frame(maxWidth: 120, alignment: .trailing)
				.background(GeometryReader { proxy in
					Color.clear.preference(key: LabelWidthKey.self, value: proxy.size.width)
				})
		} else {
			label
				.lineLimit(1)
				.fixedSize()
				.background(GeometryReader { proxy in
					Color.clear.preference(key: LabelWidthKey.self, value: proxy.size.width)
				})
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
