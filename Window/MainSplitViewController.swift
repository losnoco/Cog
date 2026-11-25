//
//  MainSplitViewController.swift
//  Cog
//
//  Created by Kevin López Brante on 6/10/26.
//  Copyright 2026 Kevin López Brante. All rights reserved.
//

import AppKit

@objc(MainSplitViewController)
class MainSplitViewController: NSSplitViewController {
	private static let observerContext = UnsafeMutableRawPointer.allocate(byteCount: 1, alignment: 1)

	// Responders to focus when the sidebar is revealed or hidden, respectively.
	@objc weak var sidebarFirstResponder: NSResponder?
	@objc weak var contentFirstResponder: NSResponder?

	// Only touched on the main thread; nonisolated(unsafe) so deinit can let
	// go of the observer.
	nonisolated(unsafe) private var sidebarItemStorage: NSSplitViewItem?

	@objc var sidebarItem: NSSplitViewItem? { sidebarItemStorage }

	@objc func setup(sidebarViewController: NSViewController, contentViewController: NSViewController) {
		splitView.isVertical = true
		splitView.dividerStyle = .thin

		let sidebar = NSSplitViewItem(sidebarWithViewController: sidebarViewController)
		sidebar.minimumThickness = 150
		sidebar.maximumThickness = 400
		sidebar.canCollapse = true
		sidebar.holdingPriority = NSLayoutConstraint.Priority.defaultLow + 10
		sidebar.allowsFullHeightLayout = true
		sidebar.titlebarSeparatorStyle = .automatic

		let contentItem = NSSplitViewItem(viewController: contentViewController)
		contentItem.minimumThickness = 400
		contentItem.holdingPriority = .defaultLow

		addSplitViewItem(sidebar)
		addSplitViewItem(contentItem)

		splitView.autosaveName = "MainWindowSplitView"

		// Visibility is owned by the FileTreeShowSideView default, not the split view autosave
		sidebar.isCollapsed = !UserDefaults.standard.bool(forKey: "FileTreeShowSideView")

		sidebar.addObserver(self, forKeyPath: "collapsed", options: [], context: Self.observerContext)
		sidebarItemStorage = sidebar
	}

	deinit {
		sidebarItemStorage?.removeObserver(self, forKeyPath: "collapsed", context: Self.observerContext)
	}

	override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		if context != Self.observerContext {
			super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
			return
		}

		let collapsed = sidebarItemStorage?.isCollapsed ?? false
		UserDefaults.standard.set(!collapsed, forKey: "FileTreeShowSideView")

		if let window = splitView.window {
			let responder = collapsed ? contentFirstResponder : sidebarFirstResponder
			if let responder {
				window.makeFirstResponder(responder)
			}
		}
	}
}