//
//  FileTreeViewController.swift
//  Cog
//
//  Created by Vincent Spader on 6/21/08.
//  Copyright 2008 Vincent Spader. All rights reserved.
//

import AppKit

@objc(FileTreeViewController)
class FileTreeViewController: NSViewController, NSMenuItemValidation {
	@IBOutlet var playlistLoader: PlaylistLoader?
	@IBOutlet var playbackController: PlaybackController?
	@IBOutlet var fileTreeOutlineView: FileTreeOutlineView?

	private var appliedSafeAreaTopInset: CGFloat = 0

	override init(nibName nibNameOrNil: String?, bundle nibBundleOrNil: Bundle?) {
		super.init(nibName: nibNameOrNil, bundle: nibBundleOrNil)
	}

	required init?(coder: NSCoder) {
		super.init(coder: coder)
	}

	convenience init() {
		self.init(nibName: "FileTree", bundle: Bundle.main)
		UserDefaults.standard.register(defaults: ["FileTreeShowSideView": false])
	}

	override func viewDidLoad() {
		super.viewDidLoad()
		fileTreeOutlineView?.style = .sourceList
	}

	// The view is laid out with autoresizing masks, so when the sidebar extends
	// into the titlebar (full-height layout) the top controls must be shifted
	// down below the titlebar and toolbar manually. The inset changes when the
	// toolbar style is toggled at runtime, hence the delta-based adjustment.
	//
	// Derive the inset from the window's content layout rect, not the view's
	// safe-area top inset: in the expanded toolbar style the safe area reports
	// only the titlebar and omits the toolbar row beneath it, which would leave
	// the controls inside the window's drag region (clicking them moves the
	// window). The content layout rect spans both rows in either toolbar style.
	override func viewWillLayout() {
		super.viewWillLayout()

		let topInset = view.window.map { $0.frame.height - $0.contentLayoutRect.height } ?? view.safeAreaInsets.top
		let delta = topInset - appliedSafeAreaTopInset
		if delta != 0 {
			for subview in view.subviews {
				var frame = subview.frame
				if subview.autoresizingMask.contains(.height) {
					frame.size.height -= delta
				} else {
					frame.origin.y -= delta
				}
				subview.frame = frame
			}
			appliedSafeAreaTopInset = topInset
		}
	}

	private var sidebarItem: NSSplitViewItem? {
		if let parent = parent as? NSSplitViewController {
			return parent.splitViewItem(for: self)
		}
		return nil
	}

	@IBAction func toggleSideView(_ sender: Any?) {
		if let parent = parent as? NSSplitViewController {
			parent.toggleSidebar(sender)
		}
	}

	@IBAction func collapseAll(_ sender: Any?) {
		fileTreeOutlineView?.collapseItem(nil, collapseChildren: true)
	}

	func validateMenuItem(_ menuItem: NSMenuItem) -> Bool {
		if menuItem.action == #selector(toggleSideView(_:)) {
			menuItem.state = (sidebarItem != nil && !(sidebarItem!.isCollapsed)) ? .on : .off
		}
		return true
	}

	@objc func doAddToPlaylist(_ urls: [Any], origin: URLOrigin) {
		let loadEntryData: [String: Any] = ["entries": urls,
		                                    "sort": true,
		                                    "origin": origin.rawValue]
		playlistLoader?.addURLs(inBackground: loadEntryData)
	}

	@objc func clear(_ sender: Any?) {
		playlistLoader?.clear(sender)
	}

	@objc func playPauseResume(_ id: NSObject?) {
		playbackController?.playPauseResume(id)
	}

	@objc var outlineView: FileTreeOutlineView? {
		fileTreeOutlineView
	}

	@IBAction func chooseRootFolder(_ sender: Any?) {
		let path = UserDefaults.standard.string(forKey: "fileTreeRootURL")

		let panel = NSOpenPanel()
		panel.allowsMultipleSelection = false
		panel.canChooseDirectories = true
		panel.canChooseFiles = false
		panel.isFloatingPanel = true
		if let path {
			panel.directoryURL = URL(fileURLWithPath: path)
		}
		let result = panel.runModal()
		if result == .OK, let url = panel.url {
			SandboxBroker.shared().addFolderIfMissing(url)
			UserDefaults.standard.setValue(url.absoluteString, forKey: "fileTreeRootURL")
		}
	}
}
