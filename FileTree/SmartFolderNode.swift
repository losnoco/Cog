//
//  SmartFolderNode.swift
//  Cog
//
//  Created by Vincent Spader on 9/25/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

import AppKit
import CoreServices

@objc(SmartFolderNode)
class SmartFolderNode: PathNode {
	private var query: MDQuery?

	override func isLeaf() -> Bool {
		false
	}

	override func updatePath() {
		let sbHandle = URL.flatMap { SandboxBroker.shared().beginFolderAccess($0) }
		let doc = NSDictionary(contentsOfFile: URL?.path ?? "")
		SandboxBroker.shared().endFolderAccess(sbHandle)
		let rawQuery = doc?["RawQuery"] as? String
		let searchPaths = (doc?["SearchCriteria"] as? [String: Any])?["CurrentFolderPath"] as? [String]

		// Ugh, Carbon from now on...
		guard let query = MDQueryCreate(kCFAllocatorDefault, rawQuery as CFString?, nil, nil) else { return }
		self.query = query

		MDQuerySetSearchScope(query, searchPaths as CFArray?, 0)

		NotificationCenter.default.addObserver(self, selector: #selector(queryFinished(_:)), name: Notification.Name(kMDQueryDidFinishNotification as String), object: query)
		NotificationCenter.default.addObserver(self, selector: #selector(queryUpdate(_:)), name: Notification.Name(kMDQueryDidUpdateNotification as String), object: query)

		MDQueryExecute(query, CFOptionFlags(kMDQueryWantsUpdates.rawValue))

		// Note: This is asynchronous!
	}

	@objc func countOfSubpaths() -> UInt32 {
		UInt32(subpaths?.count ?? 0)
	}

	@objc(objectInSubpathsAtIndex:)
	func objectInSubpaths(atIndex index: UInt32) -> PathNode? {
		subpaths?[Int(index)] as? PathNode
	}

	@objc private func queryFinished(_ notification: Notification) {
		guard let obj = notification.object else { return }
		let query = obj as! MDQuery

		var results: [String] = []

		MDQueryDisableUpdates(query)
		let c = MDQueryGetResultCount(query)

		for i in 0..<c {
			if let ptr = MDQueryGetResultAtIndex(query, i) {
				let item = unsafeBitCast(ptr, to: MDItem.self)
				if let itemPath = MDItemCopyAttribute(item, kMDItemPath) as? String {
					results.append(itemPath)
				}
			}
		}

		MDQueryEnableUpdates(query)

		processPaths(results.sorted { $0.caseInsensitiveCompare($1) == .orderedAscending })

		dataSource?.reloadPathNode(self)
	}

	@objc private func queryUpdate(_ notification: Notification) {
		queryFinished(notification)
	}

	deinit {
		NotificationCenter.default.removeObserver(self)
	}
}
