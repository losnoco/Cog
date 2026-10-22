//
//  InfoView.swift
//  Cog (iOS)
//
//  A track's details, as the macOS app's Info Inspector shows them (all but
//  where the file is). What the inspector keeps in a tooltip, the precise
//  length, the ReplayGain values and when the track was first seen and
//  last played, shows under the value here.
//

import CogPlaylist
import SwiftUI

struct InfoView: View {
	@ObservedObject var entry: PlaylistEntry
	var presentation = Presentation.sheet
	/// The rows' background over the album's colors; nil for the usual
	/// grouped list.
	var rowBackground: Color?
	/// The art at the top; not where Now Playing shows it beside.
	var showsArtwork = true
	@Environment(\.dismiss) private var dismiss
	@State private var artwork: UIImage?

	enum Presentation {
		/// In a sheet of its own, with a Done button.
		case sheet
		/// Pushed onto Now Playing's stack, on a phone.
		case page
		/// A card under Now Playing beside the playlist, whose art is in
		/// view already.
		case card
	}

	var body: some View {
		if presentation != .sheet {
			content
		} else {
			NavigationStack {
				content
					.toolbar {
						ToolbarItem(placement: .confirmationAction) {
							Button("Done") { dismiss() }
						}
					}
			}
		}
	}

	private var content: some View {
		List {
			if showsArtwork, presentation != .card, let artwork {
				Image(uiImage: artwork)
					.resizable()
					.aspectRatio(contentMode: .fit)
					.clipShape(RoundedRectangle(cornerRadius: 8))
					.frame(maxWidth: 320)
					.frame(maxWidth: .infinity)
					.listRowBackground(Color.clear)
			}
			Section {
				row("Album Artist", entry.albumartist)
				row("Artist", entry.artist)
				row("Composer", entry.composer)
				row("Album", entry.album)
				row("Title", entry.title)
				row("Track", entry.trackText)
				row("Length", entry.metadataLoaded ? formatTime(entry.length.doubleValue) : nil,
				    detail: entry.metadataLoaded ? Self.preciseTime(entry.length.doubleValue) : nil)
				row("Date", entry.date)
				row("Genre", entry.genre)
			}
			Section {
				row("Filename", entry.filenameFragment)
				row("Sample Rate", entry.sampleRate > 0 ? Self.hertz(entry.sampleRate) : nil)
				row("Channels", entry.channels > 0 ? "\(entry.channels)" : nil)
				row("Bitrate", entry.bitrate > 0 ? "\(entry.bitrate) kbps" : nil)
				row("Bits Per Sample", entry.bitsPerSample > 0 ? "\(entry.bitsPerSample)" : nil)
				row("Codec", entry.codec)
				row("Encoding", entry.encoding)
				row("Cuesheet", entry.cuesheet?.isEmpty ?? true ? String(localized: "No") : String(localized: "Yes"))
			}
			Section {
				row("ReplayGain", gainCorrection, detail: gainInfo)
				row("Play Count", playCount.count, detail: playCount.info)
			}
			if let comment = entry.comment, !comment.isEmpty {
				Section("Comment") {
					Text(comment)
						.textSelection(.enabled)
						.listRowBackground(rowBackground)
				}
			}
		}
		.navigationTitle(presentation == .card ? Text(verbatim: "") : Text("Info"))
		.navigationBarTitleDisplayMode(.inline)
		// The album's colors, or the card's, show through.
		.scrollContentBackground(rowBackground == nil && presentation != .card ? .automatic : .hidden)
		.task(id: entry.artHash) {
			guard showsArtwork, presentation != .card else { return }
			artwork = await ArtworkCache.shared.image(for: entry, pixels: 640)
		}
	}

	/// A label and its value, left out when there is no value.
	@ViewBuilder
	private func row(_ label: LocalizedStringKey, _ value: String?, detail: String? = nil) -> some View {
		if let value, !value.isEmpty {
			LabeledContent {
				VStack(alignment: .trailing, spacing: 2) {
					Text(value)
					if let detail, !detail.isEmpty {
						Text(detail)
							.font(.caption)
							.foregroundStyle(.secondary)
					}
				}
				.multilineTextAlignment(.trailing)
				.textSelection(.enabled)
			} label: {
				Text(label)
			}
			.listRowBackground(rowBackground)
		}
	}

	// MARK: - As PlaylistEntry.m has them

	/// Which gain playback can use, the most specific first.
	private var gainCorrection: String {
		if entry.replayGainAlbumGain != 0 {
			return entry.replayGainAlbumPeak != 0 ? String(localized: "GainAlbumGainPeak") : String(localized: "GainAlbumGain")
		} else if entry.replayGainTrackGain != 0 {
			return entry.replayGainTrackPeak != 0 ? String(localized: "GainTrackGainPeak") : String(localized: "GainTrackGain")
		} else if !(entry.soundcheck?.isEmpty ?? true) {
			return String(localized: "GainSoundcheck")
		} else if entry.volume != 0 && entry.volume != 1 {
			return String(localized: "GainVolumeScale")
		}
		return String(localized: "GainNone")
	}

	/// Each gain the track has, and its value.
	private var gainInfo: String {
		var items: [String] = []
		if entry.replayGainAlbumGain != 0 {
			items.append(String(format: "%@: %+.2f dB", String(localized: "GainAlbumGain"), entry.replayGainAlbumGain))
		}
		if entry.replayGainAlbumPeak != 0 {
			items.append(String(format: "%@: %.6f", String(localized: "GainAlbumPeak"), entry.replayGainAlbumPeak))
		}
		if entry.replayGainTrackGain != 0 {
			items.append(String(format: "%@: %+.2f dB", String(localized: "GainTrackGain"), entry.replayGainTrackGain))
		}
		if entry.replayGainTrackPeak != 0 {
			items.append(String(format: "%@: %.6f", String(localized: "GainTrackPeak"), entry.replayGainTrackPeak))
		}
		if let soundcheck = entry.soundcheckDisplay, !soundcheck.isEmpty {
			items.append("\(String(localized: "GainSoundcheck")): \(soundcheck)")
		}
		if entry.volume != 0 && entry.volume != 1 {
			items.append(String(format: "%@: %.2f×", String(localized: "GainVolumeScale"), entry.volume))
		}
		return items.joined(separator: "\n")
	}

	/// How often the track has been played, and when it was first seen and
	/// last played.
	private var playCount: (count: String, info: String?) {
		guard let item = PlayCounts.item(for: entry) else { return ("0", nil) }
		let count = item.value(forKey: "count") as? Int64 ?? 0
		let firstSeen = (item.value(forKey: "firstSeen") as? Date).map { "\(String(localized: "TimeFirstSeen")): \(Self.dateText($0))" }
		let lastPlayed = count > 0 ? (item.value(forKey: "lastPlayed") as? Date).map { "\(String(localized: "TimeLastPlayed")): \(Self.dateText($0))" } : nil
		return ("\(count)", [firstSeen, lastPlayed].compactMap { $0 }.joined(separator: "\n"))
	}

	private static func dateText(_ date: Date) -> String {
		date.formatted(date: .abbreviated, time: .shortened)
	}

	/// "44,100 Hz", as NumberHertzToStringTransformer has it.
	private static func hertz(_ rate: Float) -> String {
		let formatter = NumberFormatter()
		formatter.maximumFractionDigits = 2
		formatter.minimumFractionDigits = 0
		formatter.minimumIntegerDigits = 1
		formatter.roundingIncrement = 0.5
		formatter.usesGroupingSeparator = true
		formatter.groupingSeparator = ","
		formatter.groupingSize = 3
		return "\(formatter.string(from: NSNumber(value: rate)) ?? "\(rate)") Hz"
	}

	/// "3:07.250", as SecondsFractionFormatter has it.
	private static func preciseTime(_ seconds: Double) -> String? {
		guard seconds.isFinite, seconds > 0 else { return nil }
		let minutes = Int(seconds / 60)
		let rest = seconds.truncatingRemainder(dividingBy: 60)
		let (hours, mins) = (minutes / 60, minutes % 60)
		return hours > 0 ? String(format: "%d:%02d:%06.3f", hours, mins, rest) : String(format: "%d:%06.3f", mins, rest)
	}
}
