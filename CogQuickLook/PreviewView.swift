//
//  PreviewView.swift
//  CogQuickLook
//
//  The preview: the file's artwork and tags, play and pause, and a seek
//  bar with the time heard and the length.
//

import CogAudio
import SwiftUI

struct PreviewView: View {
	@ObservedObject var player: PreviewPlayer

	var body: some View {
		HStack(spacing: 16) {
			artwork
				.frame(width: 128, height: 128)
				.clipShape(RoundedRectangle(cornerRadius: 6))
			VStack(alignment: .leading, spacing: 4) {
				Text(player.title)
					.font(.headline)
					.lineLimit(2)
				if !player.subtitle.isEmpty {
					Text(player.subtitle)
						.foregroundStyle(.secondary)
						.lineLimit(2)
				}
				if player.failed {
					Text("Cog could not play this file.")
						.foregroundStyle(.secondary)
				}
				Spacer(minLength: 8)
				controls
			}
		}
		.padding(20)
		.frame(minWidth: 400, minHeight: 168)
	}

	@ViewBuilder private var artwork: some View {
		if let image = player.artwork {
			Image(nsImage: image)
				.resizable()
				.aspectRatio(contentMode: .fill)
		} else {
			ZStack {
				Rectangle().fill(.quaternary)
				Image(systemName: "music.note")
					.font(.system(size: 48))
					.foregroundStyle(.secondary)
			}
		}
	}

	private var controls: some View {
		HStack(spacing: 10) {
			Button(action: player.togglePlayPause) {
				Image(systemName: player.status == .playing ? "pause.fill" : "play.fill")
					.frame(width: 16)
			}
			.help(player.status == .playing ? "Pause" : "Play")
			if player.duration > 0 {
				Text(Self.time(player.position))
					.monospacedDigit()
				Slider(value: $player.position, in: 0...player.duration) { editing in
					player.scrubbing = editing
					if !editing { player.seek(to: player.position) }
				}
				Text(Self.time(player.duration))
					.monospacedDigit()
			} else {
				Text(Self.time(player.position))
					.monospacedDigit()
				Spacer()
			}
		}
		.font(.callout)
		.foregroundStyle(.secondary)
	}

	/// 1:05, or 1:02:05 past an hour.
	private static func time(_ seconds: Double) -> String {
		let total = Int(max(0, seconds).rounded(.down))
		let (hours, minutes, secs) = (total / 3600, total / 60 % 60, total % 60)
		return hours > 0
			? String(format: "%d:%02d:%02d", hours, minutes, secs)
			: String(format: "%d:%02d", minutes, secs)
	}
}
