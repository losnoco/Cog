//
//  PaneManager.swift
//  Cog
//
//  Created by Kevin López Brante on 2026-10-22.
//

import Cocoa

/// Where a docked pane goes: an edge of the playlist, a layer, a row and a
/// position.
struct PanePlacement: Codable, Equatable {
	enum Edge: String, Codable, CaseIterable {
		case top, bottom, left, right
	}

	var edge: Edge
	/// Docks nest like an onion: layer 0 is next to the playlist, and each
	/// higher layer wraps everything inside it.
	var layer = 0
	/// Rows lie side by side in a dock, row 0 nearest the playlist.
	var row = 0
	/// The panes' order in a row: top to bottom beside the playlist, left to
	/// right above or below it.
	var position = 0
	/// The row's thickness: its width beside the playlist, its height above
	/// or below it.
	var size: CGFloat
	/// The pane's share of its row's length.
	var proportion: CGFloat = 1

	static func defaultSize(_ edge: Edge) -> CGFloat {
		edge == .left || edge == .right ? 300 : 180
	}

	static func symbolName(_ edge: Edge) -> String {
		switch edge {
		case .top:
			return "rectangle.tophalf.filled"
		case .bottom:
			return "rectangle.bottomhalf.filled"
		case .left:
			return "sidebar.left"
		case .right:
			return "sidebar.right"
		}
	}
}

/// Docks panes in the main window, around the playlist. A pane is docked or
/// floats in its controller's own window; the View menu shows and hides it
/// wherever it is. The layout is remembered.
@MainActor
@objc(PaneManager)
final class PaneManager: NSObject {
	@objc static let shared = PaneManager()

	private struct PaneState: Codable {
		var isDocked: Bool
		/// Whether a docked pane shows.
		var isShown: Bool
		var placement: PanePlacement
	}

	private static let defaultsKey = "paneLayout"

	private var panes: [String: PaneWindowController] = [:]
	private var states: [String: PaneState] = [:]
	private var dockController: PaneDockViewController?
	/// Docked panes shown while the main window is hidden, as in mini mode:
	/// they float until it is back.
	private var borrowed: Set<String> = []
	private var dragged: PaneWindowController?
	private var dragTimer: Timer?
	private var guides: PaneDockGuideWindow?

	override private init() {
		super.init()
		if let data = UserDefaults.standard.data(forKey: Self.defaultsKey),
		   let saved = try? JSONDecoder().decode([String: PaneState].self, from: data) {
			states = saved
		}
		NotificationCenter.default.addObserver(self, selector: #selector(windowVisibilityDidChange(_:)), name: NSWindow.didChangeOcclusionStateNotification, object: nil)
	}

	/// The main window's content: `center` with the docks around it.
	@objc func dockViewController(withCenter center: NSViewController) -> NSViewController {
		let controller = PaneDockViewController(center: center, manager: self)
		dockController = controller
		rebuild()
		return controller
	}

	func register(_ pane: PaneWindowController) {
		panes[pane.paneIdentifier] = pane
		rebuild()
	}

	private var mainWindow: NSWindow? {
		dockController?.view.window
	}

	private var isMainWindowVisible: Bool {
		mainWindow?.isVisible == true
	}

	func placement(of pane: PaneWindowController) -> PanePlacement {
		states[pane.paneIdentifier]?.placement ?? pane.defaultPlacement
	}

	func isDocked(_ pane: PaneWindowController) -> Bool {
		states[pane.paneIdentifier]?.isDocked == true
	}

	// MARK: - Changing the layout

	/// Floats every docked pane that shows and forgets the layout.
	@objc func resetLayout() {
		let shown = panes.values.filter { states[$0.paneIdentifier]?.isDocked == true && states[$0.paneIdentifier]?.isShown == true }
		states.removeAll()
		borrowed.removeAll()
		changed()
		for pane in shown {
			pane.showFloating()
		}
	}

	/// Docks a pane, shown, where it last was.
	func dock(_ pane: PaneWindowController) {
		borrowed.remove(pane.paneIdentifier)
		pane.releaseFromWindow()
		states[pane.paneIdentifier] = PaneState(isDocked: true, isShown: true, placement: placement(of: pane))
		changed()
	}

	/// Docks a pane, shown, at the end of the first row on `edge`.
	func move(_ pane: PaneWindowController, to edge: PanePlacement.Edge) {
		var placement = placement(of: pane)
		if placement.edge != edge || !isDocked(pane) {
			let last = states.filter { $0.key != pane.paneIdentifier && $0.value.isDocked && $0.value.placement.edge == edge
				&& $0.value.placement.layer == 0 && $0.value.placement.row == 0 }
				.map(\.value.placement.position).max() ?? -1
			let rowSize = states.values.first { $0.isDocked && $0.placement.edge == edge && $0.placement.layer == 0 && $0.placement.row == 0 }?.placement.size
			placement = PanePlacement(edge: edge, position: last + 1, size: rowSize ?? PanePlacement.defaultSize(edge))
		}
		pane.releaseFromWindow()
		states[pane.paneIdentifier] = PaneState(isDocked: true, isShown: true, placement: placement)
		normalize()
		changed()
	}

	/// Floats a docked pane in its own window.
	func float(_ pane: PaneWindowController) {
		states[pane.paneIdentifier]?.isDocked = false
		changed()
		pane.showFloating()
	}

	/// Hides a docked pane.
	func hide(_ pane: PaneWindowController) {
		states[pane.paneIdentifier]?.isShown = false
		changed()
	}

	/// Shows a docked pane, for showWindow:. False if the pane floats. With
	/// the main window hidden, as in mini mode, the pane floats until it is
	/// back.
	func show(_ pane: PaneWindowController) -> Bool {
		guard isDocked(pane) else { return false }
		guard isMainWindowVisible else {
			borrowed.insert(pane.paneIdentifier)
			pane.showFloating()
			return true
		}
		if states[pane.paneIdentifier]?.isShown != true {
			states[pane.paneIdentifier]?.isShown = true
			changed()
		}
		return true
	}

	/// Shows or hides a docked pane, for toggleWindow:. False if it floats.
	func toggle(_ pane: PaneWindowController) -> Bool {
		guard isDocked(pane) else { return false }
		if borrowed.contains(pane.paneIdentifier), pane.window?.isVisible == true {
			pane.hideFloating()
			return true
		}
		if states[pane.paneIdentifier]?.isShown == true && isMainWindowVisible {
			hide(pane)
			return true
		}
		return show(pane)
	}

	// MARK: - Dragging

	/// A floating pane's window began to move: the guides show over the main
	/// window until it is let go.
	func paneWindowWillMove(_ pane: PaneWindowController) {
		guard NSEvent.pressedMouseButtons & 1 != 0, dragged == nil, isMainWindowVisible,
		      let layout = dockController?.guideLayout(excluding: pane.paneIdentifier) else { return }
		dragged = pane
		let guides = PaneDockGuideWindow(layout: layout)
		guides.orderFront(nil)
		self.guides = guides
		// AppKit says when a drag begins but not when it ends.
		let timer = Timer(timeInterval: 1.0 / 60, repeats: true) { [weak self] _ in
			MainActor.assumeIsolated { self?.trackDrag() }
		}
		RunLoop.main.add(timer, forMode: .common)
		dragTimer = timer
	}

	private func trackDrag() {
		guard let pane = dragged, let guides else { return }
		let target = guides.update(mouse: NSEvent.mouseLocation)
		guard NSEvent.pressedMouseButtons & 1 == 0 else { return }
		dragTimer?.invalidate()
		dragTimer = nil
		guides.orderOut(nil)
		self.guides = nil
		dragged = nil
		if let target {
			dock(pane, at: target)
		}
	}

	/// Pulls a docked pane out by its caption: it floats under the pointer
	/// and goes on being dragged until the mouse is let go.
	func tearOff(_ pane: PaneWindowController) {
		guard let mainWindow else { return }
		states[pane.paneIdentifier]?.isDocked = false
		changed()
		let mouse = NSEvent.mouseLocation
		pane.showFloating(under: mouse)
		guard let window = pane.window, let layout = dockController?.guideLayout(excluding: pane.paneIdentifier) else { return }
		let grab = NSPoint(x: mouse.x - window.frame.minX, y: mouse.y - window.frame.minY)
		let guides = PaneDockGuideWindow(layout: layout)
		guides.orderFront(nil)
		// The main window keeps the mouse from the click on the caption, so
		// its events move the new window; a window server drag started now
		// loses its place.
		var target: PaneDockTarget?
		while let next = mainWindow.nextEvent(matching: [.leftMouseDragged, .leftMouseUp]) {
			let point = NSEvent.mouseLocation
			window.setFrameOrigin(NSPoint(x: point.x - grab.x, y: point.y - grab.y))
			target = guides.update(mouse: point)
			if next.type == .leftMouseUp {
				break
			}
		}
		guides.orderOut(nil)
		if let target {
			dock(pane, at: target)
		}
	}

	private func dock(_ pane: PaneWindowController, at target: PaneDockTarget) {
		let id = pane.paneIdentifier
		var placement: PanePlacement
		switch target {
		case .outer(let edge):
			let rows = docked(edge: edge, layer: 0).map(\.placement.row)
			placement = PanePlacement(edge: edge, row: (rows.max() ?? -1) + 1, size: PanePlacement.defaultSize(edge))
		case .inner(let edge):
			shiftRows(edge: edge, layer: 0, from: 0)
			placement = PanePlacement(edge: edge, size: PanePlacement.defaultSize(edge))
		case .beside(let otherID, let side):
			guard let other = states[otherID]?.placement else { return }
			let isAlongRow = (other.edge == .left || other.edge == .right) == (side == .top || side == .bottom)
			if isAlongRow {
				// Before the pane is above it beside the playlist, left of it
				// above or below it.
				let position = side == .top || side == .left ? other.position : other.position + 1
				shiftPositions(edge: other.edge, layer: other.layer, row: other.row, from: position)
				placement = PanePlacement(edge: other.edge, layer: other.layer, row: other.row, position: position, size: other.size)
			} else {
				let towardPlaylist: PanePlacement.Edge = [.left: .right, .right: .left, .top: .bottom, .bottom: .top][other.edge] ?? side
				let row = side == towardPlaylist ? other.row : other.row + 1
				shiftRows(edge: other.edge, layer: other.layer, from: row)
				placement = PanePlacement(edge: other.edge, layer: other.layer, row: row, size: PanePlacement.defaultSize(other.edge))
			}
		}
		pane.releaseFromWindow()
		states[id] = PaneState(isDocked: true, isShown: true, placement: placement)
		normalize()
		changed()
	}

	private func docked(edge: PanePlacement.Edge, layer: Int) -> [(id: String, placement: PanePlacement)] {
		states.filter { $0.value.isDocked && $0.value.placement.edge == edge && $0.value.placement.layer == layer }
			.map { ($0.key, $0.value.placement) }
	}

	private func shiftRows(edge: PanePlacement.Edge, layer: Int, from row: Int) {
		for (id, placement) in docked(edge: edge, layer: layer) where placement.row >= row {
			states[id]?.placement.row += 1
		}
	}

	private func shiftPositions(edge: PanePlacement.Edge, layer: Int, row: Int, from position: Int) {
		for (id, placement) in docked(edge: edge, layer: layer) where placement.row == row && placement.position >= position {
			states[id]?.placement.position += 1
		}
	}

	/// Numbers the rows of each dock, and the panes of each row, from 0.
	private func normalize() {
		for edge in PanePlacement.Edge.allCases {
			for layer in Set(states.values.filter { $0.isDocked && $0.placement.edge == edge }.map(\.placement.layer)) {
				let panes = docked(edge: edge, layer: layer)
				for (row, oldRow) in Set(panes.map(\.placement.row)).sorted().enumerated() {
					let inRow = panes.filter { $0.placement.row == oldRow }.sorted { $0.placement.position < $1.placement.position }
					for (position, pane) in inRow.enumerated() {
						states[pane.id]?.placement.row = row
						states[pane.id]?.placement.position = position
					}
				}
			}
		}
	}

	/// Sizes the user dragged a divider to.
	func record(sizes: [String: CGFloat], proportions: [String: CGFloat]) {
		for (id, size) in sizes {
			states[id]?.placement.size = size
		}
		for (id, proportion) in proportions {
			states[id]?.placement.proportion = proportion
		}
		save()
	}

	private func changed() {
		save()
		rebuild()
	}

	private func save() {
		if let data = try? JSONEncoder().encode(states) {
			UserDefaults.standard.set(data, forKey: Self.defaultsKey)
		}
	}

	/// Lays the dock out again from the docked panes that show.
	private func rebuild() {
		guard let dockController else { return }
		let docked = panes.values.compactMap { pane -> (PaneWindowController, PanePlacement)? in
			guard let state = states[pane.paneIdentifier], state.isDocked, state.isShown,
			      !borrowed.contains(pane.paneIdentifier) else { return nil }
			pane.releaseFromWindow()
			return (pane, state.placement)
		}
		dockController.layOut(docked)
		for pane in panes.values {
			pane.paneVisibilityDidChange()
		}
	}

	@objc private func windowVisibilityDidChange(_ notification: Notification) {
		// Docked panes come and go with the main window.
		guard let window = notification.object as? NSWindow, window === mainWindow else { return }
		if window.isVisible && !borrowed.isEmpty {
			// Back from mini mode, panes shown meanwhile return to the dock.
			for id in borrowed {
				states[id]?.isShown = panes[id]?.window?.isVisible == true
				panes[id]?.releaseFromWindow()
			}
			borrowed.removeAll()
			changed()
			return
		}
		for (id, pane) in panes where states[id]?.isDocked == true {
			pane.paneVisibilityDidChange()
		}
	}
}

/// The playlist with the docks around it: a pair of split views for each
/// layer, beside the playlist (left and right) and spanning them (top and
/// bottom), each holding rows of panes.
final class PaneDockViewController: NSViewController, NSSplitViewDelegate {
	private let center: NSViewController
	private unowned let manager: PaneManager
	private var containers: [String: PaneContainerView] = [:]
	private var placements: [String: PanePlacement] = [:]
	private var needsSizes = false
	private var isApplyingSizes = false

	private static let minimumPane: CGFloat = 120
	private static let minimumCenter: CGFloat = 300

	init(center: NSViewController, manager: PaneManager) {
		self.center = center
		self.manager = manager
		super.init(nibName: nil, bundle: nil)
		addChild(center)
	}

	required init?(coder: NSCoder) {
		fatalError("PaneDockViewController is made in code")
	}

	override func loadView() {
		view = NSView(frame: center.view.frame)
	}

	func layOut(_ panes: [(PaneWindowController, PanePlacement)]) {
		_ = view
		let firstResponder = view.window?.firstResponder
		// The old split views would report the panes leaving them as resizes.
		var oldSplits = view.subviews
		while let split = oldSplits.popLast() {
			(split as? DockSplitView)?.delegate = nil
			oldSplits += split.subviews.filter { $0 is DockSplitView }
		}
		placements = Dictionary(uniqueKeysWithValues: panes.map { ($0.0.paneIdentifier, $0.1) })
		for (id, container) in containers where placements[id] == nil {
			container.removeFromSuperview()
			containers[id] = nil
		}

		var inner: NSView = center.view
		for layer in Set(panes.map(\.1.layer)).sorted() {
			let inLayer = panes.filter { $0.1.layer == layer }
			func rows(_ edge: PanePlacement.Edge) -> [NSView] {
				let onEdge = inLayer.filter { $0.1.edge == edge }
				return Set(onEdge.map(\.1.row)).sorted().map { row in
					rowView(onEdge.filter { $0.1.row == row }.sorted { $0.1.position < $1.1.position }.map(\.0), edge: edge)
				}
			}
			// Row 0 is nearest the playlist.
			let left = rows(.left), right = rows(.right), top = rows(.top), bottom = rows(.bottom)
			if !left.isEmpty || !right.isEmpty {
				inner = edgeSplit(Array(left.reversed()) + [inner] + right, innerIndex: left.count, vertical: true)
			}
			if !top.isEmpty || !bottom.isEmpty {
				inner = edgeSplit(Array(top.reversed()) + [inner] + bottom, innerIndex: top.count, vertical: false)
			}
		}

		for subview in view.subviews where subview !== inner {
			subview.removeFromSuperview()
		}
		if inner.superview !== view {
			inner.removeFromSuperview()
			// A split view arranges its subviews with constraints, so one
			// coming out of a split view has to be laid out by its frame again.
			inner.translatesAutoresizingMaskIntoConstraints = true
			inner.frame = view.bounds
			inner.autoresizingMask = [.width, .height]
			view.addSubview(inner)
		}
		// Moving the views drops the focus; it goes back where it was.
		if let firstResponder = firstResponder as? NSView, let window = view.window,
		   firstResponder.window === window, window.firstResponder !== firstResponder {
			window.makeFirstResponder(firstResponder)
		}
		needsSizes = true
		view.needsLayout = true
	}

	/// The dock for the drag guides, without the pane being dragged.
	func guideLayout(excluding id: String) -> PaneDockGuideLayout? {
		guard let window = view.window else { return nil }
		func onScreen(_ view: NSView) -> NSRect {
			window.convertToScreen(view.convert(view.bounds, to: nil))
		}
		let area = onScreen(view)
		let contentTop = min(area.maxY, window.convertToScreen(window.contentLayoutRect).maxY)
		let panes = containers.filter { $0.key != id && $0.value.window === window }.map { (id: $0.key, frame: onScreen($0.value)) }
		return PaneDockGuideLayout(area: area, contentTop: contentTop, center: onScreen(center.view), panes: panes)
	}

	private func container(for pane: PaneWindowController) -> PaneContainerView {
		if let container = containers[pane.paneIdentifier] {
			container.attachContent()
			return container
		}
		let container = PaneContainerView(pane: pane, manager: manager)
		containers[pane.paneIdentifier] = container
		return container
	}

	/// A row of panes: across it beside the playlist, along it above or below.
	private func rowView(_ panes: [PaneWindowController], edge: PanePlacement.Edge) -> NSView {
		let views = panes.map { container(for: $0) }
		guard views.count > 1 else { return views[0] }
		let split = DockSplitView()
		split.isVertical = edge == .top || edge == .bottom
		split.paneIDs = panes.map { [$0.paneIdentifier] }
		configure(split, views)
		return split
	}

	private func edgeSplit(_ views: [NSView], innerIndex: Int, vertical: Bool) -> NSView {
		let split = DockSplitView()
		split.isVertical = vertical
		split.innerIndex = innerIndex
		split.paneIDs = views.enumerated().map { index, view in index == innerIndex ? [] : paneIDs(in: view) }
		configure(split, views)
		// The playlist takes the window's resizing; the docks keep their size.
		for index in views.indices {
			split.setHoldingPriority(index == innerIndex ? .defaultLow : .defaultLow + 10, forSubviewAt: index)
		}
		return split
	}

	private func configure(_ split: DockSplitView, _ views: [NSView]) {
		split.dividerStyle = .thin
		split.delegate = self
		for view in views {
			view.removeFromSuperview()
			split.addSubview(view)
		}
	}

	private func paneIDs(in view: NSView) -> [String] {
		if let container = view as? PaneContainerView {
			return [container.paneIdentifier]
		}
		return (view as? DockSplitView)?.paneIDs.flatMap { $0 } ?? []
	}

	// MARK: - Sizes

	override func viewDidLayout() {
		super.viewDidLayout()
		guard needsSizes, view.bounds.width > 0, view.bounds.height > 0, let root = view.subviews.first as? DockSplitView else { return }
		needsSizes = false
		isApplyingSizes = true
		applySizes(root)
		isApplyingSizes = false
	}

	private func thickness(_ ids: [String]) -> CGFloat {
		ids.compactMap { placements[$0]?.size }.max() ?? 0
	}

	/// Sets the dividers from the saved sizes, outside in.
	private func applySizes(_ split: DockSplitView) {
		split.adjustSubviews()
		let total = split.isVertical ? split.bounds.width : split.bounds.height
		let divider = split.dividerThickness
		let count = split.subviews.count
		if let inner = split.innerIndex {
			var offset: CGFloat = 0
			for index in 0..<inner {
				offset += thickness(split.paneIDs[index])
				split.setPosition(offset, ofDividerAt: index)
				offset += divider
			}
			var end = total
			for index in stride(from: count - 1, to: inner, by: -1) {
				end -= thickness(split.paneIDs[index])
				split.setPosition(end - divider, ofDividerAt: index - 1)
				end -= divider
			}
		} else {
			let shares = split.paneIDs.map { placements[$0[0]]?.proportion ?? 1 }
			let sum = shares.reduce(0, +)
			let available = total - divider * CGFloat(count - 1)
			var offset: CGFloat = 0
			for index in 0..<(count - 1) {
				offset += available * shares[index] / sum
				split.setPosition(offset, ofDividerAt: index)
				offset += divider
			}
		}
		for case let child as DockSplitView in split.subviews {
			applySizes(child)
		}
	}

	func splitViewDidResizeSubviews(_ notification: Notification) {
		// Only a dragged divider changes the saved sizes.
		guard !isApplyingSizes, notification.userInfo?["NSSplitViewDividerIndex"] != nil,
		      let split = notification.object as? DockSplitView, split.window != nil,
		      split.subviews.count == split.paneIDs.count else { return }
		var sizes: [String: CGFloat] = [:]
		var proportions: [String: CGFloat] = [:]
		for (index, subview) in split.subviews.enumerated() where index < split.paneIDs.count {
			let length = split.isVertical ? subview.frame.width : subview.frame.height
			if split.innerIndex != nil {
				for id in split.paneIDs[index] {
					sizes[id] = length
				}
			} else if let id = split.paneIDs[index].first {
				proportions[id] = length
			}
		}
		if !proportions.isEmpty {
			let sum = proportions.values.reduce(0, +)
			proportions = proportions.mapValues { $0 / sum * CGFloat(proportions.count) }
		}
		for (id, size) in sizes {
			placements[id]?.size = size
		}
		manager.record(sizes: sizes, proportions: proportions)
	}

	private func minimum(_ split: DockSplitView, at index: Int) -> CGFloat {
		index == split.innerIndex ? Self.minimumCenter : Self.minimumPane
	}

	func splitView(_ splitView: NSSplitView, constrainMinCoordinate proposedMinimumPosition: CGFloat, ofSubviewAt dividerIndex: Int) -> CGFloat {
		guard let split = splitView as? DockSplitView else { return proposedMinimumPosition }
		let subview = split.subviews[dividerIndex]
		let start = split.isVertical ? subview.frame.minX : subview.frame.minY
		return max(proposedMinimumPosition, start + minimum(split, at: dividerIndex))
	}

	func splitView(_ splitView: NSSplitView, constrainMaxCoordinate proposedMaximumPosition: CGFloat, ofSubviewAt dividerIndex: Int) -> CGFloat {
		guard let split = splitView as? DockSplitView else { return proposedMaximumPosition }
		let subview = split.subviews[dividerIndex + 1]
		let end = split.isVertical ? subview.frame.maxX : subview.frame.maxY
		return min(proposedMaximumPosition, end - minimum(split, at: dividerIndex + 1) - split.dividerThickness)
	}
}

/// A split view of the dock, which knows the panes in each subview.
private final class DockSplitView: NSSplitView {
	/// Beside the playlist, the subview holding it; along a row, nil.
	var innerIndex: Int?
	var paneIDs: [[String]] = []
}

/// A docked pane: a caption with its title, Float and Close, over the
/// content.
private final class PaneContainerView: NSView {
	let paneIdentifier: String
	private unowned let pane: PaneWindowController
	private unowned let manager: PaneManager
	private let content = PaneContentView()
	private var captionTop: NSLayoutConstraint!

	init(pane: PaneWindowController, manager: PaneManager) {
		self.paneIdentifier = pane.paneIdentifier
		self.pane = pane
		self.manager = manager
		super.init(frame: NSRect(x: 0, y: 0, width: 300, height: 200))

		let title = NSTextField(labelWithString: pane.paneTitle)
		title.font = .systemFont(ofSize: NSFont.smallSystemFontSize, weight: .semibold)
		title.textColor = .secondaryLabelColor
		title.lineBreakMode = .byTruncatingTail
		title.setContentCompressionResistancePriority(.defaultLow, for: .horizontal)
		title.setContentHuggingPriority(.defaultLow, for: .horizontal)
		let float = button("rectangle.on.rectangle", NSLocalizedString("Float in a Window", comment: "Docked pane: the button that moves it to its own window"), #selector(floatPane(_:)))
		let close = button("xmark", NSLocalizedString("Close", comment: "Docked pane: the button that hides it"), #selector(closePane(_:)))
		let caption = PaneCaptionView(views: [title, float, close])
		caption.onDragOut = { [weak self] in
			guard let self else { return }
			withExtendedLifetime(self) {
				self.manager.tearOff(self.pane)
			}
		}
		caption.orientation = .horizontal
		caption.spacing = 4
		caption.edgeInsets = NSEdgeInsets(top: 0, left: 8, bottom: 0, right: 4)
		caption.setHuggingPriority(.defaultLow, for: .horizontal)
		caption.menu = captionMenu()

		content.appearance = pane.paneAppearance
		// Views draw past their bounds since macOS 14, and a scroll view's edge
		// effect reaches up for the toolbar: the content would cover the caption.
		if #available(macOS 14.0, *) {
			content.clipsToBounds = true
		}
		for view in [caption, content] {
			view.translatesAutoresizingMaskIntoConstraints = false
			addSubview(view)
		}
		captionTop = caption.topAnchor.constraint(equalTo: topAnchor)
		NSLayoutConstraint.activate([
			captionTop,
			caption.leadingAnchor.constraint(equalTo: leadingAnchor),
			caption.trailingAnchor.constraint(equalTo: trailingAnchor),
			caption.heightAnchor.constraint(equalToConstant: 24),
			content.topAnchor.constraint(equalTo: caption.bottomAnchor),
			content.leadingAnchor.constraint(equalTo: leadingAnchor),
			content.trailingAnchor.constraint(equalTo: trailingAnchor),
			content.bottomAnchor.constraint(equalTo: bottomAnchor)
		])
		attachContent()
	}

	required init?(coder: NSCoder) {
		fatalError("PaneContainerView is made in code")
	}

	override func layout() {
		// The main window's content runs under the toolbar, so the caption
		// keeps below it; the safe area does not reach into the split views.
		if let window {
			let layoutRect = convert(window.contentLayoutRect, from: nil)
			let inset = max(0, bounds.maxY - layoutRect.maxY)
			if captionTop.constant != inset {
				captionTop.constant = inset
			}
		}
		super.layout()
	}

	/// Takes the pane's content back from its window.
	func attachContent() {
		let view = pane.paneView
		guard view.superview !== content else { return }
		view.removeFromSuperview()
		view.translatesAutoresizingMaskIntoConstraints = true
		view.frame = content.bounds
		view.autoresizingMask = [.width, .height]
		content.addSubview(view)
	}

	private func button(_ symbol: String, _ label: String, _ action: Selector) -> NSButton {
		let button = NSButton(image: NSImage(systemSymbolName: symbol, accessibilityDescription: label) ?? NSImage(), target: self, action: action)
		button.isBordered = false
		button.toolTip = label
		button.imageScaling = .scaleProportionallyDown
		button.widthAnchor.constraint(equalToConstant: 18).isActive = true
		return button
	}

	private func captionMenu() -> NSMenu {
		let menu = NSMenu()
		let titles: [PanePlacement.Edge: String] = [
			.left: NSLocalizedString("Dock on the Left", comment: "Docked pane: move it to the left of the playlist"),
			.right: NSLocalizedString("Dock on the Right", comment: "Docked pane: move it to the right of the playlist"),
			.top: NSLocalizedString("Dock Above", comment: "Docked pane: move it above the playlist"),
			.bottom: NSLocalizedString("Dock Below", comment: "Docked pane: move it below the playlist")
		]
		for edge in [PanePlacement.Edge.left, .right, .top, .bottom] {
			let item = NSMenuItem(title: titles[edge] ?? "", action: #selector(movePane(_:)), keyEquivalent: "")
			item.target = self
			item.representedObject = edge.rawValue
			item.image = NSImage(systemSymbolName: PanePlacement.symbolName(edge), accessibilityDescription: nil)
			menu.addItem(item)
		}
		menu.addItem(.separator())
		let float = NSMenuItem(title: NSLocalizedString("Float in a Window", comment: "Docked pane: the button that moves it to its own window"), action: #selector(floatPane(_:)), keyEquivalent: "")
		float.target = self
		menu.addItem(float)
		let close = NSMenuItem(title: NSLocalizedString("Close", comment: "Docked pane: the button that hides it"), action: #selector(closePane(_:)), keyEquivalent: "")
		close.target = self
		menu.addItem(close)
		return menu
	}

	@objc private func movePane(_ sender: NSMenuItem) {
		guard let edge = (sender.representedObject as? String).flatMap(PanePlacement.Edge.init(rawValue:)) else { return }
		manager.move(pane, to: edge)
	}

	@objc private func floatPane(_ sender: Any?) {
		manager.float(pane)
	}

	@objc private func closePane(_ sender: Any?) {
		manager.hide(pane)
	}
}

/// Under the pane's content, the window's background in the pane's own
/// appearance, so a pane drawn for a dark window stays dark.
private final class PaneContentView: NSView {
	override func draw(_ dirtyRect: NSRect) {
		NSColor.windowBackgroundColor.setFill()
		dirtyRect.fill()
	}
}

/// A docked pane's caption, which pulls the pane out when dragged.
private final class PaneCaptionView: NSStackView {
	var onDragOut: (() -> Void)?
	private var mouseDownPoint: NSPoint?

	/// How far the pointer goes before the pane comes out.
	private static let threshold: CGFloat = 6

	override var mouseDownCanMoveWindow: Bool {
		false
	}

	override func mouseDown(with event: NSEvent) {
		mouseDownPoint = event.locationInWindow
	}

	override func mouseDragged(with event: NSEvent) {
		guard let start = mouseDownPoint else { return }
		let point = event.locationInWindow
		if hypot(point.x - start.x, point.y - start.y) > Self.threshold {
			mouseDownPoint = nil
			onDragOut?()
		}
	}

	override func mouseUp(with event: NSEvent) {
		mouseDownPoint = nil
	}
}
