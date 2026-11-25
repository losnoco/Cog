//
//  PathSuggester.swift
//  Preferences
//
//  Created by Christopher Snowhill on 6/21/22.
//  Copyright 2022 Christopher Snowhill. All rights reserved.
//

import AppKit
import CogAudio
import CogPlaylist
import CoreData

@objc(PathItem)
class PathItem: NSObject {
	@objc dynamic var path: String = ""
	@objc dynamic var enabled: Bool = false

	@IBAction func clickEnable(_ sender: Any?) {
		enabled = !enabled
	}
}

@objc(PathSuggesterView)
class PathSuggesterView: NSArrayController, NSTableViewDelegate {
	@IBOutlet var sandboxPathBehaviorController: SandboxPathBehaviorController?
	@IBOutlet var window: NSWindow?

	@IBAction func applyPaths(_ sender: Any?) {
		for case let pi as PathItem in (arrangedObjects as? [Any] ?? []) where pi.enabled {
			let panel = NSOpenPanel()
			panel.allowsMultipleSelection = false
			panel.canChooseDirectories = true
			panel.canChooseFiles = false
			panel.isFloatingPanel = true
			panel.directoryURL = URL(fileURLWithPath: pi.path)
			if panel.runModal() == .OK, let url = panel.url {
				sandboxPathBehaviorController?.addUrl(url)
			}
		}
		window?.close()
	}

	func tableView(_ tableView: NSTableView, viewFor tableColumn: NSTableColumn?, row: Int) -> NSView? {
		var cellIdentifier = ""
		let cellTextAlignment = NSTextAlignment.left

		let objects = arrangedObjects as? [Any] ?? []
		guard row >= 0, row < objects.count else { return nil }
		let pi = objects[row] as? PathItem

		let fontSize: CGFloat = 13.0
		var cellButton: NSButton?
		var cellText = ""

		if let pi {
			cellIdentifier = tableColumn?.identifier.rawValue ?? ""
			if cellIdentifier == "enabled" {
				let button = NSButton(checkboxWithTitle: "", target: pi, action: #selector(PathItem.clickEnable(_:)))
				button.state = pi.enabled ? .on : .off
				cellButton = button
			} else if cellIdentifier == "path" {
				cellText = pi.path
			}
		}

		let view = tableView.makeView(withIdentifier: NSUserInterfaceItemIdentifier(rawValue: cellIdentifier), owner: nil)
		if let cellView = view as? NSTableCellView {
			if let textField = cellView.textField {
				textField.allowsDefaultTighteningForTruncation = true
				textField.font = NSFont.monospacedDigitSystemFont(ofSize: fontSize, weight: .regular)
				textField.stringValue = cellText
				textField.alignment = cellTextAlignment
				if textField.intrinsicContentSize.width > textField.frame.size.width - 4 {
					textField.toolTip = cellText
				}
			}
			if let cellButton {
				cellView.subviews.forEach { $0.removeFromSuperview() }
				cellView.addSubview(cellButton)
			}
		}

		return view
	}
}

@objc(PathSuggester)
class PathSuggester: NSWindowController {
	@IBOutlet var sandboxPathBehaviorController: SandboxPathBehaviorController?
	@IBOutlet var viewController: PathSuggesterView?

	convenience init() {
		self.init(windowNibName: "PathSuggester")
	}

	@IBAction func beginSuggestion(_ sender: Any?) {
		showWindow(sender)

		guard let viewController else { return }
		viewController.remove(contentsOf: (viewController.arrangedObjects as? [Any]) ?? [])

		let pc = PlaylistController.sharedPersistentContainer()

		let hasUrl = NSPredicate(format: "urlString != nil && urlString != %@", "")
		let notDeleted = NSPredicate(format: "deLeted == NO || deLeted == nil")
		let request = NSFetchRequest<NSManagedObject>(entityName: "PlaylistEntry")
		request.predicate = NSCompoundPredicate(andPredicateWithSubpredicates: [notDeleted, hasUrl])

		guard let results = try? pc.viewContext.fetch(request), !results.isEmpty else { return }

		var array = ((results as NSArray).value(forKey: "url") as? [URL] ?? []).filter { $0.isFileURL }

		for url in array {
			if let contained = AudioContainer.dependencyUrls(forContainerURL: url), !contained.isEmpty {
				array.append(contentsOf: contained.compactMap { $0 as? URL })
			}
		}

		let defaults = UserDefaults.standard
		if let fileTreePath = defaults.string(forKey: "fileTreeRootURL"), !fileTreePath.isEmpty {
			// Append false name to dodge the directory/fragment trimmer
			array.append(URL(string: (fileTreePath as NSString).appendingPathComponent("moo.mp3"))!)
		}
		if let soundFontPath = defaults.string(forKey: "soundFontPath"), !soundFontPath.isEmpty {
			array.append(URL(fileURLWithPath: soundFontPath))
		}

		var items: [PathItem] = []
		var itemPaths: [URL] = []

		for fileUrl in array {
			guard let url = SandboxBroker.urlWithoutFragment(fileUrl) else { continue }
			if sandboxPathBehaviorController?.matchesPath(url) == true { continue }

			let pathComponents = url.pathComponents
			if pathComponents.count > 2 {
				for i in 2..<pathComponents.count {
					let subUrl = URL(fileURLWithPath: pathComponents[0..<i].joined(separator: "/"))
					if !itemPaths.contains(subUrl) {
						itemPaths.append(subUrl)
						let item = PathItem()
						item.path = subUrl.path
						item.enabled = false
						items.append(item)
					}
				}
			}
		}

		let sortDescriptor = NSSortDescriptor(key: "path.length", ascending: true)
		viewController.sortDescriptors = [sortDescriptor]
		items.sort { ($0.path.count) < ($1.path.count) }
		viewController.add(contentsOf: items)
		viewController.setSelectedObjects([])
	}
}