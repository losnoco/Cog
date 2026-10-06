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
	private var palettes: [String: ArtworkPalette] = [:]

	/// The colors of `entry`'s art, if they have been worked out already.
	func cachedPalette(for entry: PlaylistEntry) -> ArtworkPalette? {
		entry.artHash.flatMap { palettes[$0] }
	}

	/// The colors of `entry`'s art; nil if it has none.
	func palette(for entry: PlaylistEntry) async -> ArtworkPalette? {
		guard let hash = entry.artHash else { return nil }
		if let cached = palettes[hash] { return cached }
		guard let data = entry.albumArtInternal else { return nil }
		let palette = await Task.detached(priority: .userInitiated) {
			Self.decode(data, pixels: 32).flatMap { $0.cgImage }.flatMap(ArtworkPalette.init)
		}.value
		if let palette { palettes[hash] = palette }
		return palette
	}

	/// The art of `entry` at `pixels` on its longer side; nil if it has none.
	func image(for entry: PlaylistEntry, pixels: Int) async -> UIImage? {
		guard let hash = entry.artHash else { return nil }
		let key = "\(hash)-\(pixels)" as NSString
		if let cached = cache.object(forKey: key) { return cached }
		guard let data = entry.albumArtInternal else { return nil }
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

/// Colors to theme what shows an album's art: a dark gradient from the
/// art's own colors, behind light text, and its most vivid color, made
/// bright enough to read on that.
struct ArtworkPalette: Equatable, Sendable {
	var top: Color
	var bottom: Color
	var accent: Color

	/// From a thumbnail of the art, a few dozen pixels across.
	nonisolated init?(_ image: CGImage) {
		let width = image.width, height = image.height
		guard width > 0, height > 0 else { return nil }
		var pixels = [UInt8](repeating: 0, count: width * height * 4)
		guard let context = CGContext(data: &pixels, width: width, height: height, bitsPerComponent: 8, bytesPerRow: width * 4,
		                              space: CGColorSpaceCreateDeviceRGB(),
		                              bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue) else { return nil }
		context.draw(image, in: CGRect(x: 0, y: 0, width: width, height: height))

		var halves = [(r: 0.0, g: 0.0, b: 0.0, n: 0.0), (r: 0.0, g: 0.0, b: 0.0, n: 0.0)]
		// Vivid pixels by hue, in twelve slices: each slice's weight, and
		// its colors summed.
		var slices = Array(repeating: (weight: 0.0, r: 0.0, g: 0.0, b: 0.0), count: 12)
		for y in 0..<height {
			for x in 0..<width {
				let i = (y * width + x) * 4
				let r = Double(pixels[i]) / 255, g = Double(pixels[i + 1]) / 255, b = Double(pixels[i + 2]) / 255
				let half = y < height / 2 ? 0 : 1
				halves[half].r += r; halves[half].g += g; halves[half].b += b; halves[half].n += 1

				var hue: CGFloat = 0, saturation: CGFloat = 0, brightness: CGFloat = 0
				UIColor(red: r, green: g, blue: b, alpha: 1).getHue(&hue, saturation: &saturation, brightness: &brightness, alpha: nil)
				guard saturation > 0.3, brightness > 0.25 else { continue }
				let slice = min(Int(hue * 12), 11)
				let weight = Double(saturation * brightness)
				slices[slice].weight += weight
				slices[slice].r += r * weight; slices[slice].g += g * weight; slices[slice].b += b * weight
			}
		}

		// The bitmap's rows run top down.
		top = Self.background(halves[0])
		bottom = Self.background(halves[1])
		if let vivid = slices.max(by: { $0.weight < $1.weight }), vivid.weight > Double(width * height) * 0.02 {
			let color = UIColor(red: vivid.r / vivid.weight, green: vivid.g / vivid.weight, blue: vivid.b / vivid.weight, alpha: 1)
			var hue: CGFloat = 0, saturation: CGFloat = 0, brightness: CGFloat = 0
			color.getHue(&hue, saturation: &saturation, brightness: &brightness, alpha: nil)
			accent = Color(hue: hue, saturation: min(max(saturation, 0.45), 0.85), brightness: max(brightness, 0.9))
		} else {
			accent = Color(white: 0.92)
		}
	}

	/// An average, kept dark under light text.
	private static func background(_ sum: (r: Double, g: Double, b: Double, n: Double)) -> Color {
		let n = max(sum.n, 1)
		var hue: CGFloat = 0, saturation: CGFloat = 0, brightness: CGFloat = 0
		UIColor(red: sum.r / n, green: sum.g / n, blue: sum.b / n, alpha: 1).getHue(&hue, saturation: &saturation, brightness: &brightness, alpha: nil)
		return Color(hue: hue, saturation: min(saturation, 0.7), brightness: min(brightness, 0.38) * 0.85 + 0.06)
	}
}

extension View {
	/// Keeps `palette` the colors of `entry`'s art: nil without art, and
	/// worked out again only when the art changes.
	///
	/// Colors already worked out apply before the view is first drawn, so it
	/// opens in them rather than changing to them just after.
	func albumPalette(_ palette: Binding<ArtworkPalette?>, of entry: PlaylistEntry?) -> some View {
		onAppear {
			if let entry, let cached = ArtworkCache.shared.cachedPalette(for: entry) {
				palette.wrappedValue = cached
			}
		}
		.task(id: entry?.artHash) {
			if let entry {
				palette.wrappedValue = await ArtworkCache.shared.palette(for: entry)
			} else {
				palette.wrappedValue = nil
			}
		}
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
