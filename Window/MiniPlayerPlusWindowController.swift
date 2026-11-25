//
//  MiniPlayerPlusWindowController.swift
//  Cog
//
//  The mini-plus window's track info panel: album art, title, artist/album
//  and the technical rows, laid out top-down in a scroll view.
//

import AppKit
import CogPlaylist

private let sidePad: CGFloat = 16
private let topPad: CGFloat = 12
private let itemSpacing: CGFloat = 2
private let sectionGap: CGFloat = 10

private class FlippedView: NSView {
	override var isFlipped: Bool { true }
}

@objc(MiniPlayerPlusWindowController)
class MiniPlayerPlusWindowController: NSWindowController, NSWindowDelegate {
	private static let context = UnsafeMutableRawPointer.allocate(byteCount: 1, alignment: 1)

	@IBOutlet nonisolated(unsafe) var playlistSelectionController: NSArrayController?
	@IBOutlet nonisolated(unsafe) var currentEntryController: NSObjectController?
	@IBOutlet var appController: AppController?

	weak var valueToDisplay: PlaylistEntry?

	private var scrollView: NSScrollView?

	override func awakeFromNib() {
		guard let contentView = window?.contentView else { return }
		let scroll = NSScrollView(frame: contentView.bounds)
		scroll.autoresizingMask = [.width, .height]
		scroll.hasVerticalScroller = true
		scroll.hasHorizontalScroller = false
		scroll.autohidesScrollers = true
		scroll.drawsBackground = false
		contentView.addSubview(scroll)
		scrollView = scroll

		window?.delegate = self

		playlistSelectionController?.addObserver(self, forKeyPath: "selection", options: .new, context: Self.context)
		currentEntryController?.addObserver(self, forKeyPath: "content", options: .new, context: Self.context)
		UserDefaults.standard.addObserver(self, forKeyPath: "trackInfoSource", options: .new, context: Self.context)
		NotificationCenter.default.addObserver(self, selector: #selector(playbackDidBegin(_:)), name: .CogPlaybackDidBeginNotificiation, object: nil)
		rebuildContent()
	}

	deinit {
		playlistSelectionController?.removeObserver(self, forKeyPath: "selection", context: Self.context)
		currentEntryController?.removeObserver(self, forKeyPath: "content", context: Self.context)
		UserDefaults.standard.removeObserver(self, forKeyPath: "trackInfoSource", context: Self.context)
		NotificationCenter.default.removeObserver(self, name: .CogPlaybackDidBeginNotificiation, object: nil)
	}

	override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		if context == Self.context {
			valueToDisplay = entryToDisplay
			rebuildContent()
		} else {
			super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
		}
	}

	/// The track to show, as the setting says, or the other one where that
	/// leaves none, as PaneWindowController's trackToShow does.
	private var entryToDisplay: PlaylistEntry? {
		let selected = playlistSelectionController?.selectedObjects.first as? PlaylistEntry
		let playing = currentEntryController?.content as? PlaylistEntry
		if UserDefaults.standard.string(forKey: "trackInfoSource") == "selection" {
			return selected ?? playing
		}
		return playing ?? selected
	}

	@objc private func playbackDidBegin(_ notification: Notification) {
		valueToDisplay = entryToDisplay
		rebuildContent()
	}

	func windowDidEndLiveResize(_ notification: Notification) {
		rebuildContent()
	}

	private func rebuildContent() {
		guard let scrollView else { return }
		let entry = valueToDisplay
		var width = scrollView.bounds.width
		if width < 1 { width = 360 }

		let flipped = FlippedView(frame: NSRect(x: 0, y: 0, width: width, height: 2000))
		var curY = topPad

		// --- Album Art ---
		if let art = entry?.albumArt {
			let artSize = width - 2 * sidePad
			let artView = NSImageView(frame: NSRect(x: sidePad, y: curY, width: artSize, height: artSize))
			artView.image = art
			artView.imageScaling = .scaleProportionallyUpOrDown
			artView.imageAlignment = .alignCenter
			flipped.addSubview(artView)
			curY += artSize + sectionGap
		}

		// --- Song Title ---
		var title = entry?.title ?? ""
		if title.isEmpty { title = entry?.display ?? "" }
		if !title.isEmpty {
			let titleField = labelField(title, font: .boldSystemFont(ofSize: 16), width: width - 2 * sidePad, atY: curY)
			flipped.addSubview(titleField)
			curY += titleField.frame.height + 3
		}

		// --- Artist + Album (medium, secondary) ---
		let artist = entry?.artist ?? ""
		let albumArtist = entry?.albumartist ?? ""
		let album = entry?.album ?? ""
		let displayArtist = !artist.isEmpty ? artist : (!albumArtist.isEmpty ? albumArtist : "")
		var subLine = ""
		if !displayArtist.isEmpty { subLine += displayArtist }
		if !album.isEmpty {
			if !subLine.isEmpty { subLine += " — " }
			subLine += album
		}
		if !subLine.isEmpty {
			let subField = labelField(subLine, font: .systemFont(ofSize: 13), width: width - 2 * sidePad, atY: curY)
			subField.textColor = .secondaryLabelColor
			flipped.addSubview(subField)
			curY += subField.frame.height + sectionGap
		} else {
			curY += sectionGap
		}

		// --- Separator line ---
		let sep = NSBox(frame: NSRect(x: sidePad, y: curY, width: width - 2 * sidePad, height: 1))
		sep.boxType = .separator
		flipped.addSubview(sep)
		curY += 6

		// --- Technical info rows ---
		if let entry {
			let trackText = entry.trackText
			if !trackText.isEmpty && trackText != "0" {
				curY = addRow(NSLocalizedString("Track", comment: "Track title"), value: trackText, to: flipped, atY: curY, width: width)
			}
			let lengthText = entry.lengthText
			if !lengthText.isEmpty {
				curY = addRow(NSLocalizedString("Length", comment: "Length title"), value: lengthText, to: flipped, atY: curY, width: width)
			}
			if let genre = entry.genre, !genre.isEmpty {
				curY = addRow(NSLocalizedString("Genre", comment: "Genre title"), value: genre, to: flipped, atY: curY, width: width)
			}
			if let composer = entry.composer, !composer.isEmpty {
				curY = addRow(NSLocalizedString("Composer", comment: "Composer title"), value: composer, to: flipped, atY: curY, width: width)
			}
			if let date = entry.date, !date.isEmpty {
				curY = addRow(NSLocalizedString("Date", comment: "Date title"), value: date, to: flipped, atY: curY, width: width)
			}
			if let comment = entry.comment, !comment.isEmpty {
				curY = addRow(NSLocalizedString("Comment", comment: "Comment title"), value: comment, to: flipped, atY: curY, width: width)
			}
		}

		let sep2 = NSBox(frame: NSRect(x: sidePad, y: curY, width: width - 2 * sidePad, height: 1))
		sep2.boxType = .separator
		flipped.addSubview(sep2)
		curY += 6

		if let entry {
			let sr = entry.sampleRate
			if sr > 0 {
				let t = ValueTransformer(forName: NSValueTransformerName("NumberHertzToStringTransformer"))
				let srStr = (t?.transformedValue(sr) as? String) ?? String(format: "%.0f Hz", sr)
				curY = addRow(NSLocalizedString("Sample Rate", comment: "Sample Rate title"), value: srStr, to: flipped, atY: curY, width: width)
			}
			let ch = Int(entry.channels)
			if ch > 0 { curY = addRow(NSLocalizedString("Channels", comment: "Channels title"), value: "\(ch)", to: flipped, atY: curY, width: width) }
			let bps = Int(entry.bitsPerSample)
			if bps > 0 { curY = addRow(NSLocalizedString("Bits/Sample", comment: "Bits per sample title"), value: "\(bps)", to: flipped, atY: curY, width: width) }
			let br = Int(entry.bitrate)
			if br > 0 { curY = addRow(NSLocalizedString("Bitrate", comment: "Bitrate title"), value: "\(br) kbps", to: flipped, atY: curY, width: width) }
			if let codec = entry.codec, !codec.isEmpty {
				curY = addRow(NSLocalizedString("Codec", comment: "Codec title"), value: codec, to: flipped, atY: curY, width: width)
			}
			if let encoding = entry.encoding, !encoding.isEmpty {
				curY = addRow(NSLocalizedString("Encoding", comment: "Encoding title"), value: encoding, to: flipped, atY: curY, width: width)
			}

			if entry.replayGainAlbumGain != 0 || entry.replayGainTrackGain != 0 ||
				(entry.soundcheck?.isEmpty == false) || (entry.volume != 0 && entry.volume != 1.0) {
				let replayGain = entry.gainCorrection
				if !replayGain.isEmpty {
					curY = addRow(NSLocalizedString("ReplayGain", comment: "ReplayGain title"), value: replayGain, to: flipped, atY: curY, width: width)
				}
			}
			let playCount = entry.playCount
			if !playCount.isEmpty && playCount != "0" {
				curY = addRow(NSLocalizedString("Play Count", comment: "Play Count title"), value: playCount, to: flipped, atY: curY, width: width)
			}
			let filename = entry.filename
			if !filename.isEmpty {
				curY = addRow(NSLocalizedString("File", comment: "File title"), value: filename, to: flipped, atY: curY, width: width)
			}
		}

		curY += topPad

		flipped.frame = NSRect(x: 0, y: 0, width: width, height: curY)
		scrollView.documentView = flipped
	}

	/// A wrapping text label that auto-sizes its height, left-aligned at y.
	private func labelField(_ str: String, font: NSFont, width w: CGFloat, atY y: CGFloat) -> NSTextField {
		let f = NSTextField(frame: NSRect(x: sidePad, y: y, width: w, height: 10000))
		f.stringValue = str
		f.font = font
		f.isBezeled = false
		f.drawsBackground = false
		f.isEditable = false
		f.isSelectable = true
		f.lineBreakMode = .byWordWrapping
		f.preferredMaxLayoutWidth = w
		f.sizeToFit()
		let needed = f.cell?.cellSize(forBounds: NSRect(x: 0, y: 0, width: w, height: 10000)) ?? .zero
		f.frame = NSRect(x: sidePad, y: y, width: w, height: needed.height)
		return f
	}

	/// Add a label:value row, returns new Y position.
	private func addRow(_ label: String, value: String, to parent: NSView, atY y: CGFloat, width: CGFloat) -> CGFloat {
		let labelW: CGFloat = 110
		let valueX = sidePad + labelW + 4
		let valueW = width - valueX - sidePad

		let lbl = NSTextField(frame: NSRect(x: sidePad, y: y, width: labelW, height: 17))
		lbl.stringValue = label + ":"
		lbl.font = .systemFont(ofSize: NSFont.smallSystemFontSize)
		lbl.alignment = .right
		lbl.textColor = .secondaryLabelColor
		lbl.isBezeled = false
		lbl.drawsBackground = false
		lbl.isEditable = false

		let val = NSTextField(frame: NSRect(x: valueX, y: y, width: valueW, height: 10000))
		val.stringValue = value
		val.font = .systemFont(ofSize: NSFont.smallSystemFontSize)
		val.isBezeled = false
		val.drawsBackground = false
		val.isEditable = false
		val.isSelectable = true
		val.lineBreakMode = .byWordWrapping
		let needed = val.cell?.cellSize(forBounds: NSRect(x: 0, y: 0, width: valueW, height: 10000)) ?? .zero
		let rowH = max(needed.height, 17)
		val.frame = NSRect(x: valueX, y: y, width: valueW, height: rowH)
		lbl.frame = NSRect(x: sidePad, y: y + (rowH - 17) / 2, width: labelW, height: 17)

		parent.addSubview(lbl)
		parent.addSubview(val)
		return y + rowH + itemSpacing
	}
}