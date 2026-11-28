//
//  FileTreeDataSource.swift
//  Cog
//
//  Created by Vincent Spader on 10/14/07.
//  Copyright 2007 Vincent Spader. All rights reserved.
//

import AppKit
import CogAudio

// XXX this is only for reference, we have the entitlement for the path anyway
private func pathEscape(_ path: String) -> URL {
	let componentsToRemove = "Library/Containers/\(Bundle.main.bundleIdentifier ?? "")/Data/"
	var path = path
	if let range = path.range(of: componentsToRemove) {
		path.replaceSubrange(range, with: "")
	}
	return URL(fileURLWithPath: path)
}

private func defaultMusicDirectory() -> URL {
	let path = NSSearchPathForDirectoriesInDomains(.musicDirectory, .userDomainMask, true).last ?? ""
	return pathEscape(path)
}

// The search runs on a background queue and the FSEvents callback arrives on
// the run loop it was scheduled on, as in the ObjC original; the mutable state
// is nonisolated(unsafe) to match.
@objc(FileTreeDataSource)
class FileTreeDataSource: NSObject, NSOutlineViewDataSource, NSSearchFieldDelegate, PathWatcherDelegate, @unchecked Sendable {
	@IBOutlet weak var outlineView: NSOutlineView?
	@IBOutlet weak var pathControl: NSPathControl?
	@IBOutlet weak var watcher: PathWatcher?
	weak var searchField: NSSearchField?

	nonisolated(unsafe) private static let observationContext = UnsafeMutableRawPointer.allocate(byteCount: 1, alignment: 1)

	nonisolated(unsafe) private var rootNode: PathNode?
	nonisolated(unsafe) private var sbHandle: UnsafeRawPointer?
	nonisolated(unsafe) private var filteredNodes: [Any]?
	nonisolated(unsafe) private var _filterString: String?
	nonisolated(unsafe) private var keyEventMonitor: Any?

	private static let registerDefaults: Void = {
		let path = defaultMusicDirectory().absoluteString
		UserDefaults.standard.register(defaults: ["fileTreeRootURL": path])
	}()

	@objc var filterString: String? {
		get { _filterString }
		set {
			_filterString = newValue
			if newValue?.isEmpty != false {
				filteredNodes = nil
				outlineView?.reloadData()
			} else if let newValue {
				performSearch(with: newValue)
			}
		}
	}

	@objc var rootURL: URL? {
		get { rootNode?.URL }
		set {
			let sharedSandboxBroker = SandboxBroker.shared()
			if let sbHandle {
				sharedSandboxBroker.endFolderAccess(sbHandle)
			}
			sbHandle = newValue.flatMap { sharedSandboxBroker.beginFolderAccess($0) }

			var rootURL = newValue
			if !FileManager.default.fileExists(atPath: rootURL?.path ?? "") {
				rootURL = defaultMusicDirectory()
			}

			rootNode = DirectoryNode(dataSource: self, url: rootURL)

			watcher?.setPath(rootURL?.path)

			if let rootNode {
				reloadPathNode(rootNode)
			}
		}
	}

	override init() {
		super.init()
		_ = Self.registerDefaults
	}

	override func awakeFromNib() {
		sbHandle = nil
		pathControl?.target = self
		pathControl?.action = #selector(pathControlAction(_:))
		NSUserDefaultsController.shared.addObserver(self,
		                                            forKeyPath: "values.fileTreeRootURL",
		                                            options: [.new, .initial],
		                                            context: Self.observationContext)

		// Build search field programmatically so AppKit sets up all internal
		// geometry (icon inset, field-editor frame, cancel button hit area) correctly.
		if let scrollView = outlineView?.enclosingScrollView, let container = scrollView.superview {
			let scrollFrame = scrollView.frame
			let searchHeight: CGFloat = 22.0
			let searchPadX: CGFloat = 8.0
			let searchPadTop: CGFloat = 5.0
			let searchPadBottom: CGFloat = 6.0
			let newScrollHeight = scrollFrame.size.height - searchHeight - searchPadTop - searchPadBottom
			scrollView.frame = NSRect(x: scrollFrame.origin.x, y: scrollFrame.origin.y,
			                          width: scrollFrame.size.width, height: newScrollHeight)
			let searchFrame = NSRect(x: scrollFrame.origin.x + searchPadX,
			                         y: scrollFrame.origin.y + newScrollHeight + searchPadBottom,
			                         width: scrollFrame.size.width - searchPadX * 2.0, height: searchHeight)
			let sf = NSSearchField(frame: searchFrame)
			sf.placeholderString = NSLocalizedString("Tree Search", comment: "File tree search placeholder")
			sf.delegate = self
			sf.autoresizingMask = [.width, .minYMargin]
			container.addSubview(sf)
			searchField = sf
		}

		keyEventMonitor = NSEvent.addLocalMonitorForEvents(matching: .keyDown) { [weak self] event in
			let flags = event.modifierFlags.intersection([.command, .option, .shift, .control])
			if flags == [.command, .option],
			   event.charactersIgnoringModifiers == "f" {
				if let s = self, let searchField = s.searchField, searchField.window?.isKeyWindow == true {
					searchField.window?.makeFirstResponder(searchField)
					return nil
				}
			}
			return event
		}
	}

	override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		if context == Self.observationContext {
			if keyPath == "values.fileTreeRootURL" {
				let url = NSUserDefaultsController.shared.defaults.string(forKey: "fileTreeRootURL")
				rootURL = url.flatMap { URL(string: $0) }
			}
		} else {
			super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
		}
	}

	@objc func changeURL(_ rootURL: URL?) {
		if let rootURL {
			NSUserDefaultsController.shared.defaults.set(rootURL.absoluteString, forKey: "fileTreeRootURL")
		}
	}

	@objc private func pathControlAction(_ sender: Any?) {
		if let item = pathControl?.clickedPathItem, let url = item.url {
			changeURL(url)
		}
	}

	func controlTextDidChange(_ obj: Notification) {
		filterString = (obj.object as? NSSearchField)?.stringValue
	}

	func searchFieldDidEndSearching(_ sender: NSSearchField) {
		filterString = ""
	}

	private func performSearch(with searchString: String) {
		let captured = searchString
		guard let rootURL else { return }

		DispatchQueue.global(qos: .userInitiated).async { [self] in
			var results: [PathNode] = []
			let rootPath = rootURL.path
			let rootPathWithSlash = rootPath + "/"

			guard let enumerator = FileManager.default.enumerator(at: rootURL,
			                                                      includingPropertiesForKeys: [.nameKey, .isDirectoryKey],
			                                                      options: [.skipsHiddenFiles],
			                                                      errorHandler: nil) else {
				return
			}

			for case let url as URL in enumerator {
				if captured != _filterString { return }

				let name = url.lastPathComponent
				if name.range(of: captured, options: .caseInsensitive) == nil {
					continue
				}

				let isDir = (try? url.resourceValues(forKeys: [.isDirectoryKey]))?.isDirectory ?? false

				if !isDir && !((AudioPlayer.fileTypes() as? [String]) ?? []).contains(url.pathExtension.lowercased()) {
					continue
				}

				let fullPath = url.path
				let relativePath = fullPath.hasPrefix(rootPathWithSlash)
					? String(fullPath.dropFirst(rootPathWithSlash.count))
					: fullPath

				let node: PathNode = isDir
					? DirectoryNode(dataSource: self, url: url)
					: FileNode(dataSource: self, url: url)
				node.display = relativePath
				results.append(node)

				if results.count >= 500 { break }
			}

			DispatchQueue.main.async { [self] in
				if captured == _filterString {
					filteredNodes = results
					outlineView?.reloadData()
				}
			}
		}
	}

	deinit {
		if let keyEventMonitor {
			NSEvent.removeMonitor(keyEventMonitor)
		}
		if let sbHandle {
			SandboxBroker.shared().endFolderAccess(sbHandle)
		}
	}

	private func nodeForPath(_ path: String) -> PathNode? {
		let rootPath = rootURL?.path ?? ""
		let replaced = (path as NSString).replacingOccurrences(of: rootPath + "/", with: "", options: .anchored, range: NSRange(location: 0, length: (path as NSString).length))
		var relativePath = (replaced as NSString).standardizingPath
		if relativePath == rootPath {
			relativePath = ""
		}
		var node = rootNode
		for c in (relativePath as NSString).pathComponents {
			guard let subnode = node?.subpathsLookup?[c] else { return nil }
			node = subnode
		}

		return node
	}

	func pathDidChange(_ path: String, flags: FSEventStreamEventFlags) {
		let watched = FSEventStreamEventFlags(kFSEventStreamEventFlagItemCreated | kFSEventStreamEventFlagItemRemoved | kFSEventStreamEventFlagItemRenamed)
		if flags & watched == 0 {
			// We only care about changes that would affect whether a file exists, or has a new name
			// Apparently, Xattr changes are causing crashes in this notification tracker, oops.
			return
		}
		// Need to find the corresponding node...and call node.updatePath(), then reloadPathNode(node)
		var path = path
		var node: PathNode?
		repeat {
			node = nodeForPath(path)
			path = (path as NSString).deletingLastPathComponent
			if path.count < 2 { return }
		} while node == nil

		guard let node else { return }

		if flags & FSEventStreamEventFlags(kFSEventStreamEventFlagItemRemoved) != 0 {
			let parentNode = nodeForPath(path)
			parentNode?.updatePath()
			if let parentNode {
				reloadPathNode(parentNode)
			}
		} else {
			node.updatePath()
			reloadPathNode(node)
		}
	}

	func outlineView(_ outlineView: NSOutlineView, numberOfChildrenOfItem item: Any?) -> Int {
		if let filteredNodes {
			return item == nil ? filteredNodes.count : 0
		}
		let n = (item as? PathNode) ?? rootNode
		return n?.subpaths?.count ?? 0
	}

	func outlineView(_ outlineView: NSOutlineView, isItemExpandable item: Any) -> Bool {
		if filteredNodes != nil { return false }
		let n = (item as? PathNode) ?? rootNode
		return !(n?.isLeaf() ?? true)
	}

	func outlineView(_ outlineView: NSOutlineView, child index: Int, ofItem item: Any?) -> Any {
		if let filteredNodes {
			return filteredNodes[index]
		}
		let n = (item as? PathNode) ?? rootNode
		return n?.subpaths?[index] ?? NSNull()
	}

	func outlineView(_ outlineView: NSOutlineView, objectValueFor tableColumn: NSTableColumn?, byItem item: Any?) -> Any? {
		let n = (item as? PathNode) ?? rootNode
		return n
	}

	func outlineView(_ outlineView: NSOutlineView, pasteboardWriterForItem item: Any) -> (any NSPasteboardWriting)? {
		let paste = NSPasteboardItem()
		if let node = item as? PathNode, let url = node.URL {
			paste.setData(url.dataRepresentation, forType: .fileURL)
		}
		return paste
	}

	@objc func reloadPathNode(_ item: PathNode) {
		if filteredNodes != nil { return } // filesystem changes don't update the search results view
		if item === rootNode {
			outlineView?.reloadData()
		} else {
			outlineView?.reloadItem(item, reloadChildren: true)
		}
	}
}
