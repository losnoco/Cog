//
//  PaneDockGuides.swift
//  Cog
//
//  Created by Kevin López Brante on 2026-10-22.
//

import Cocoa

/// Where a pane dragged over the main window docks when let go.
enum PaneDockTarget: Equatable {
	/// A new row outside the others on an edge.
	case outer(PanePlacement.Edge)
	/// A new row next to the playlist.
	case inner(PanePlacement.Edge)
	/// Beside a docked pane, on one of its sides: in its row, or in a new
	/// row next to it.
	case beside(String, PanePlacement.Edge)
}

/// The dock as the guides see it, in screen coordinates.
struct PaneDockGuideLayout {
	/// The playlist and the docks around it.
	var area: NSRect
	/// Where the toolbar ends.
	var contentTop: CGFloat
	var center: NSRect
	var panes: [(id: String, frame: NSRect)]
}

/// Shows the guides over the main window while a pane is dragged: one at
/// each edge, a compass over the playlist and one over the docked pane under
/// the pointer, and where the pane would go. It takes no clicks.
final class PaneDockGuideWindow: NSWindow {
	private let guideView: PaneDockGuideView

	init(layout: PaneDockGuideLayout) {
		guideView = PaneDockGuideView(layout: layout)
		super.init(contentRect: layout.area, styleMask: .borderless, backing: .buffered, defer: false)
		isOpaque = false
		backgroundColor = .clear
		hasShadow = false
		ignoresMouseEvents = true
		isReleasedWhenClosed = false
		// Above the floating panes, the inspectors' panels among them.
		level = NSWindow.Level(rawValue: NSWindow.Level.floating.rawValue + 1)
		contentView = guideView
	}

	/// Follows the pointer, in screen coordinates, and says where the pane
	/// would dock.
	func update(mouse: NSPoint) -> PaneDockTarget? {
		guideView.update(mouse: NSPoint(x: mouse.x - frame.minX, y: mouse.y - frame.minY))
	}
}

private final class PaneDockGuideView: NSView {
	private struct Guide {
		var frame: NSRect
		var symbol: String
		var target: PaneDockTarget
	}

	private let area: NSRect
	private let contentTop: CGFloat
	private let center: NSRect
	private let panes: [(id: String, frame: NSRect)]
	private var guides: [Guide] = []
	private var hoveredPane: String?
	private var target: PaneDockTarget?

	private static let guideSize: CGFloat = 34
	private static let inset: CGFloat = 10

	init(layout: PaneDockGuideLayout) {
		// Everything in the view's own coordinates.
		let origin = layout.area.origin
		func local(_ rect: NSRect) -> NSRect {
			rect.offsetBy(dx: -origin.x, dy: -origin.y)
		}
		area = NSRect(origin: .zero, size: layout.area.size)
		contentTop = layout.contentTop - origin.y
		var center = local(layout.center)
		center.size.height = min(center.maxY, contentTop) - center.minY
		self.center = center
		panes = layout.panes.map { ($0.id, local($0.frame)) }
		super.init(frame: area)
		layoutGuides()
	}

	required init?(coder: NSCoder) {
		fatalError("PaneDockGuideView is made in code")
	}

	private func layoutGuides() {
		let size = Self.guideSize
		let inset = Self.inset
		let middleY = (area.minY + contentTop) / 2
		guides = [
			Guide(frame: NSRect(x: inset, y: middleY - size / 2, width: size, height: size), symbol: "arrow.left.to.line", target: .outer(.left)),
			Guide(frame: NSRect(x: area.maxX - inset - size, y: middleY - size / 2, width: size, height: size), symbol: "arrow.right.to.line", target: .outer(.right)),
			Guide(frame: NSRect(x: area.midX - size / 2, y: contentTop - inset - size, width: size, height: size), symbol: "arrow.up.to.line", target: .outer(.top)),
			Guide(frame: NSRect(x: area.midX - size / 2, y: inset, width: size, height: size), symbol: "arrow.down.to.line", target: .outer(.bottom))
		]
		guides += compass(around: NSPoint(x: center.midX, y: center.midY)) { .inner($0) }
		if let hoveredPane, let pane = panes.first(where: { $0.id == hoveredPane }) {
			guides += compass(around: NSPoint(x: pane.frame.midX, y: pane.frame.midY)) { .beside(hoveredPane, $0) }
		}
	}

	/// Four guides in a cross.
	private func compass(around point: NSPoint, target: (PanePlacement.Edge) -> PaneDockTarget) -> [Guide] {
		let size = Self.guideSize
		let step = size + 4
		func frame(_ dx: CGFloat, _ dy: CGFloat) -> NSRect {
			NSRect(x: point.x + dx * step - size / 2, y: point.y + dy * step - size / 2, width: size, height: size)
		}
		return [
			Guide(frame: frame(-1, 0), symbol: "arrow.left", target: target(.left)),
			Guide(frame: frame(1, 0), symbol: "arrow.right", target: target(.right)),
			Guide(frame: frame(0, 1), symbol: "arrow.up", target: target(.top)),
			Guide(frame: frame(0, -1), symbol: "arrow.down", target: target(.bottom))
		]
	}

	func update(mouse: NSPoint) -> PaneDockTarget? {
		// Over a docked pane, its own compass shows, though not while the
		// pointer is on a guide already showing.
		let onGuide = guides.contains { $0.frame.contains(mouse) }
		let pane = onGuide ? hoveredPane : panes.first { $0.frame.contains(mouse) }?.id
		if pane != hoveredPane {
			hoveredPane = pane
			layoutGuides()
			needsDisplay = true
		}
		let target = guides.first { $0.frame.contains(mouse) }?.target
		if target != self.target {
			self.target = target
			needsDisplay = true
		}
		return target
	}

	// MARK: - Drawing

	override func draw(_ dirtyRect: NSRect) {
		if let target, let hint = hint(for: target) {
			let path = NSBezierPath(roundedRect: hint.insetBy(dx: 2, dy: 2), xRadius: 6, yRadius: 6)
			NSColor.controlAccentColor.withAlphaComponent(0.25).setFill()
			path.fill()
			NSColor.controlAccentColor.setStroke()
			path.lineWidth = 2
			path.stroke()
		}
		for guide in guides {
			let isHovered = guide.target == target
			let path = NSBezierPath(roundedRect: guide.frame, xRadius: 7, yRadius: 7)
			(isHovered ? NSColor.controlAccentColor : NSColor.windowBackgroundColor.withAlphaComponent(0.92)).setFill()
			path.fill()
			NSColor.separatorColor.setStroke()
			path.lineWidth = 1
			path.stroke()
			let color: NSColor = isHovered ? .white : .labelColor
			let configuration = NSImage.SymbolConfiguration(pointSize: 15, weight: .semibold)
				.applying(NSImage.SymbolConfiguration(paletteColors: [color]))
			if let image = NSImage(systemSymbolName: guide.symbol, accessibilityDescription: nil)?.withSymbolConfiguration(configuration) {
				let size = image.size
				image.draw(in: NSRect(x: guide.frame.midX - size.width / 2, y: guide.frame.midY - size.height / 2, width: size.width, height: size.height))
			}
		}
	}

	/// Roughly where the pane would go.
	private func hint(for target: PaneDockTarget) -> NSRect? {
		switch target {
		case .outer(let edge):
			let content = NSRect(x: area.minX, y: area.minY, width: area.width, height: contentTop - area.minY)
			return strip(of: content, on: edge, thickness: PanePlacement.defaultSize(edge))
		case .inner(let edge):
			return strip(of: center, on: edge, thickness: PanePlacement.defaultSize(edge))
		case .beside(let id, let side):
			guard var frame = panes.first(where: { $0.id == id })?.frame else { return nil }
			frame.size.height = min(frame.maxY, contentTop) - frame.minY
			return strip(of: frame, on: side, thickness: .infinity)
		}
	}

	/// A strip along a side of `rect`, at most a third of it, or half of it
	/// for a pane's own side.
	private func strip(of rect: NSRect, on edge: PanePlacement.Edge, thickness: CGFloat) -> NSRect {
		let isBeside = thickness == .infinity
		switch edge {
		case .left, .right:
			let width = isBeside ? rect.width / 2 : min(thickness, rect.width / 3)
			return NSRect(x: edge == .left ? rect.minX : rect.maxX - width, y: rect.minY, width: width, height: rect.height)
		case .top, .bottom:
			let height = isBeside ? rect.height / 2 : min(thickness, rect.height / 3)
			return NSRect(x: rect.minX, y: edge == .bottom ? rect.minY : rect.maxY - height, width: rect.width, height: height)
		}
	}
}

@available(macOS 14.0, *)
#Preview("Guides over a dock", traits: .fixedLayout(width: 900, height: 520)) {
	// The playlist in the middle, the spectrum below it, an inspector on the
	// right; the pointer on the guide below the inspector.
	let view = PaneDockGuideView(layout: PaneDockGuideLayout(
		area: NSRect(x: 0, y: 0, width: 900, height: 520),
		contentTop: 470,
		center: NSRect(x: 0, y: 181, width: 599, height: 339),
		panes: [("spectrum", NSRect(x: 0, y: 0, width: 599, height: 180)),
		        ("signalInspector", NSRect(x: 600, y: 0, width: 300, height: 520))]))
	_ = view.update(mouse: NSPoint(x: 750, y: 235))
	_ = view.update(mouse: NSPoint(x: 750, y: 222))
	return view
}
