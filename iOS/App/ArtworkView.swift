//
//  ArtworkView.swift
//  Cog (iOS)
//
//  Album art from the playlist's store, decoded off the main thread at the
//  size it is shown (ImageIO reads JPEG, PNG and AVIF alike), and cached.
//

import CogPlaylist
import ImageIO
import SwiftUI
import UIKit

@MainActor
final class ArtworkCache {
	static let shared = ArtworkCache()

	private let cache = NSCache<NSString, UIImage>()

	/// The art of `entry` at `pixels` on its longer side; nil if it has none.
	func image(for entry: PlaylistEntry, pixels: Int) async -> UIImage? {
		guard let hash = entry.artHash else { return nil }
		let key = "\(hash)-\(pixels)" as NSString
		if let cached = cache.object(forKey: key) { return cached }
		guard let data = entry.albumArtData else { return nil }
		let image = await Task.detached(priority: .userInitiated) { Self.decode(data, pixels: pixels) }.value
		if let image { cache.setObject(image, forKey: key) }
		return image
	}

	nonisolated private static func decode(_ data: Data, pixels: Int) -> UIImage? {
		guard let source = CGImageSourceCreateWithData(data as CFData, nil) else { return nil }
		let options: [CFString: Any] = [kCGImageSourceCreateThumbnailFromImageAlways: true,
		                                kCGImageSourceCreateThumbnailWithTransform: true,
		                                kCGImageSourceThumbnailMaxPixelSize: pixels]
		guard let image = CGImageSourceCreateThumbnailAtIndex(source, 0, options as CFDictionary) else { return nil }
		return UIImage(cgImage: image)
	}
}

/// An entry's art, or a placeholder note, `size` points square.
struct ArtworkView: View {
	@ObservedObject var entry: PlaylistEntry
	let size: CGFloat
	var cornerRadius: CGFloat = 6
	@Environment(\.displayScale) private var scale
	@State private var image: UIImage?

	var body: some View {
		ZStack {
			if let image {
				Image(uiImage: image)
					.resizable()
					.scaledToFill()
			} else {
				Rectangle()
					.fill(.quaternary)
				Image(systemName: "music.note")
					.font(.system(size: size * 0.4))
					.foregroundStyle(.secondary)
			}
		}
		.frame(width: size, height: size)
		.clipShape(RoundedRectangle(cornerRadius: cornerRadius))
		// Again at a new size (a rotation), keeping the image shown meanwhile.
		.task(id: "\(entry.artHash ?? "")@\(Int(size * scale))") {
			image = await ArtworkCache.shared.image(for: entry, pixels: Int(size * scale))
		}
	}
}
