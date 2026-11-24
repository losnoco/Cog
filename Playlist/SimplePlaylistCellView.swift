//
//  SimplePlaylistCellView.swift
//  Cog
//
//  The playlist's row in its simple layout, as the iOS app has it: the
//  status, album art, the title over "artist — album", and the length.
//  PlaylistController fills it in; it knows nothing of entries.
//

import AppKit

@objc(SimplePlaylistCellView)
final class SimplePlaylistCellView: NSTableCellView {
	@objc static let identifier = NSUserInterfaceItemIdentifier("simple")

	private static let padding: CGFloat = 4
	private static let lineSpacing: CGFloat = 1

	private let statusView = NSImageView()
	private let artView = NSImageView()
	private let titleField = NSTextField(labelWithString: "")
	private let subtitleField = NSTextField(labelWithString: "")
	private let lengthField = NSTextField(labelWithString: "")
	private var statusWidth: NSLayoutConstraint!
	private var artSide: NSLayoutConstraint!

	/// Album art drawn at the size of the row, by its hash and side.
	private static let thumbnails = NSCache<NSString, NSImage>()

	override init(frame frameRect: NSRect) {
		super.init(frame: frameRect)
		identifier = Self.identifier

		statusView.imageScaling = .scaleProportionallyDown
		statusView.contentTintColor = .secondaryLabelColor

		artView.imageScaling = .scaleProportionallyUpOrDown
		artView.wantsLayer = true
		artView.layer?.cornerRadius = 3
		artView.layer?.masksToBounds = true
		// Its side is set; a picture's own size mustn't change it.
		for orientation in [NSLayoutConstraint.Orientation.horizontal, .vertical] {
			artView.setContentHuggingPriority(.defaultLow, for: orientation)
			artView.setContentCompressionResistancePriority(.defaultLow, for: orientation)
			statusView.setContentHuggingPriority(.defaultLow, for: orientation)
			statusView.setContentCompressionResistancePriority(.defaultLow, for: orientation)
		}

		for field in [titleField, subtitleField, lengthField] {
			field.lineBreakMode = .byTruncatingTail
			field.allowsDefaultTighteningForTruncation = true
			field.cell?.truncatesLastVisibleLine = true
		}
		subtitleField.textColor = .secondaryLabelColor
		lengthField.textColor = .secondaryLabelColor
		lengthField.alignment = .right

		titleField.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
		subtitleField.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
		lengthField.setContentCompressionResistancePriority(.required, for: .horizontal)
		lengthField.setContentHuggingPriority(.required, for: .horizontal)

		let text = NSStackView(views: [titleField, subtitleField])
		text.orientation = .vertical
		text.alignment = .leading
		text.spacing = Self.lineSpacing
		text.setHuggingPriority(.defaultLow, for: .horizontal)

		let row = NSStackView(views: [statusView, artView, text, lengthField])
		row.orientation = .horizontal
		row.alignment = .centerY
		row.spacing = 8
		row.distribution = .fill
		row.detachesHiddenViews = true
		row.translatesAutoresizingMaskIntoConstraints = false
		addSubview(row)

		statusWidth = statusView.widthAnchor.constraint(equalToConstant: 14)
		artSide = artView.widthAnchor.constraint(equalToConstant: 36)
		NSLayoutConstraint.activate([
			row.leadingAnchor.constraint(equalTo: leadingAnchor, constant: 2),
			row.trailingAnchor.constraint(equalTo: trailingAnchor, constant: -6),
			row.centerYAnchor.constraint(equalTo: centerYAnchor),
			statusWidth,
			statusView.heightAnchor.constraint(equalTo: statusView.widthAnchor),
			artSide,
			artView.heightAnchor.constraint(equalTo: artView.widthAnchor),
		])

		imageView = statusView
		textField = titleField
	}

	@available(*, unavailable)
	required init?(coder: NSCoder) {
		fatalError("init(coder:) has not been implemented")
	}

	private static func titleFont(_ fontSize: CGFloat, bold: Bool) -> NSFont {
		.monospacedDigitSystemFont(ofSize: fontSize, weight: bold ? .bold : .regular)
	}

	private static func subtitleFont(_ fontSize: CGFloat) -> NSFont {
		.monospacedDigitSystemFont(ofSize: max(fontSize - 1, 8), weight: .regular)
	}

	private static func lineHeight(_ font: NSFont) -> CGFloat {
		ceil(NSLayoutManager().defaultLineHeight(for: font))
	}

	/// The side of the art: the height of the two lines of text.
	private static func artSide(fontSize: CGFloat) -> CGFloat {
		lineHeight(titleFont(fontSize, bold: true)) + lineSpacing + lineHeight(subtitleFont(fontSize))
	}

	/// The row's height at `fontSize`, from the playlist's font size setting.
	@objc static func rowHeight(fontSize: CGFloat) -> CGFloat {
		ceil(artSide(fontSize: fontSize) + 2 * padding)
	}

	/// - Parameters:
	///   - artKey: what identifies `art` (its hash), to keep it drawn small.
	///   - current: the playing track, whose status shows in the accent color.
	///   - bold: the playing track, when it is set to stand out.
	///   - dimmed: an entry that failed to play.
	@objc func configure(status: NSImage?, statusToolTip: String?,
	                     art: NSImage?, artKey: String?, showsArt: Bool,
	                     title: String, subtitle: String?,
	                     length: String, lengthToolTip: String?,
	                     fontSize: CGFloat, current: Bool, bold: Bool, dimmed: Bool) {
		let side = Self.artSide(fontSize: fontSize)

		statusWidth.constant = ceil(fontSize + 2)
		statusView.symbolConfiguration = NSImage.SymbolConfiguration(pointSize: fontSize, weight: .regular)
		statusView.image = status
		statusView.toolTip = statusToolTip
		statusView.contentTintColor = current ? .controlAccentColor : .secondaryLabelColor

		artView.isHidden = !showsArt
		artSide.constant = side
		if showsArt, let art {
			artView.imageScaling = .scaleProportionallyUpOrDown
			artView.symbolConfiguration = nil
			artView.contentTintColor = nil
			artView.image = Self.thumbnail(art, key: artKey, side: side)
		} else if showsArt {
			// A note where the art would be, so the titles line up.
			artView.imageScaling = .scaleNone
			artView.symbolConfiguration = NSImage.SymbolConfiguration(pointSize: round(side * 0.4), weight: .regular)
			artView.contentTintColor = .tertiaryLabelColor
			artView.image = NSImage(systemSymbolName: "music.note", accessibilityDescription: nil)
		} else {
			artView.image = nil
		}

		titleField.font = Self.titleFont(fontSize, bold: bold)
		titleField.stringValue = title
		titleField.textColor = dimmed ? .secondaryLabelColor : .labelColor
		titleField.toolTip = statusToolTip

		subtitleField.font = Self.subtitleFont(fontSize)
		subtitleField.stringValue = subtitle ?? ""
		subtitleField.isHidden = subtitle?.isEmpty ?? true

		lengthField.font = Self.subtitleFont(fontSize)
		lengthField.stringValue = length
		lengthField.toolTip = lengthToolTip
	}

	/// `image` drawn at `side` points (at 2x), cached by `key`; the image
	/// itself without a key.
	private static func thumbnail(_ image: NSImage, key: String?, side: CGFloat) -> NSImage {
		guard let key, side > 0 else { return image }
		let cacheKey = "\(key)-\(Int(side))" as NSString
		if let cached = thumbnails.object(forKey: cacheKey) { return cached }

		let pixels = Int(ceil(side * 2))
		guard let rep = NSBitmapImageRep(bitmapDataPlanes: nil, pixelsWide: pixels, pixelsHigh: pixels,
		                                 bitsPerSample: 8, samplesPerPixel: 4, hasAlpha: true, isPlanar: false,
		                                 colorSpaceName: .deviceRGB, bytesPerRow: 0, bitsPerPixel: 0) else { return image }
		// In points before the context is made, or it draws in pixels: the
		// picture would fill only the lower-left quarter.
		rep.size = NSSize(width: side, height: side)
		guard let context = NSGraphicsContext(bitmapImageRep: rep) else { return image }

		// Fit the whole picture in the square, centered, as the image view would.
		let size = image.size
		let scale = size.width > 0 && size.height > 0 ? min(side / size.width, side / size.height) : 1
		let drawn = NSSize(width: size.width * scale, height: size.height * scale)
		let rect = NSRect(x: (side - drawn.width) / 2, y: (side - drawn.height) / 2, width: drawn.width, height: drawn.height)

		NSGraphicsContext.saveGraphicsState()
		NSGraphicsContext.current = context
		context.imageInterpolation = .high
		image.draw(in: rect, from: .zero, operation: .copy, fraction: 1)
		NSGraphicsContext.restoreGraphicsState()

		let thumbnail = NSImage(size: rep.size)
		thumbnail.addRepresentation(rep)
		thumbnails.setObject(thumbnail, forKey: cacheKey)
		return thumbnail
	}
}
