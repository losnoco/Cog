//
//  XmlContainer.swift
//  Xml
//
//  Created by Christopher Snowhill on 10/9/13.
//  Copyright 2013 __NoWork, Inc__. All rights reserved.
//

import AppKit

@objc(XmlContainer)
class XmlContainer: NSObject {
	@objc class func urlForPath(_ path: String, relativeTo baseFilename: String) -> URL? {
		if path.range(of: "://") != nil {
			return URL(string: path)
		}

		var unixPath = path

		// Get the fragment
		var fragment = ""
		let scanner = Scanner(string: path)
		let characterSet = CharacterSet(charactersIn: "#1234567890")
		while !scanner.isAtEnd {
			_ = scanner.scanUpToString("#")

			if let possibleFragment = scanner.scanCharacters(from: characterSet), scanner.isAtEnd {
				fragment = possibleFragment
				unixPath.removeLast(possibleFragment.count)
				break
			}
		}
		#if DEBUG
			NSLog("Fragment: \(fragment)")
		#endif

		if !unixPath.hasPrefix("/") {
			// Only relative paths would have windows backslashes.
			unixPath = unixPath.replacingOccurrences(of: "\\", with: "/")

			let basePath = ((baseFilename as NSString).standardizingPath as NSString).deletingLastPathComponent + "/"

			unixPath = basePath + unixPath
		}

		// Append the fragment
		return URL(string: URL(fileURLWithPath: unixPath).absoluteString + fragment)
	}

	@objc(entriesForContainerURL:)
	class func entries(forContainerURL url: URL) -> [AnyHashable: Any]? {
		if !url.isFileURL {
			return nil
		}

		let filename = url.path

		let broker = SandboxBroker.shared()
		let sbHandle = broker.beginFolderAccess(url)

		let contents: String?
		do {
			contents = try String(contentsOfFile: filename, encoding: .utf8)
		} catch {
			NSLog("Error: \(error)")
			contents = nil
		}

		broker.endFolderAccess(sbHandle)

		guard let contents, let plistData = contents.data(using: .utf8) else {
			return nil
		}

		let plist: Any
		do {
			plist = try PropertyListSerialization.propertyList(from: plistData, options: [], format: nil)
		} catch {
			NSLog("Error: \(error)")
			return nil
		}

		var items: [[AnyHashable: Any]]
		var albumArt: [AnyHashable: Any]?
		var queueList: [Any]
		if let array = plist as? [Any] {
			items = array.compactMap { $0 as? [AnyHashable: Any] }
			albumArt = nil
			queueList = []
		} else if let dict = plist as? [AnyHashable: Any] {
			items = (dict["items"] as? [Any])?.compactMap { $0 as? [AnyHashable: Any] } ?? []
			albumArt = dict["albumArt"] as? [AnyHashable: Any]
			queueList = dict["queue"] as? [Any] ?? []
		} else {
			return nil
		}

		var entries: [[AnyHashable: Any]] = []

		for entry in items {
			var preparedEntry = entry

			preparedEntry["URL"] = urlForPath(preparedEntry["URL"] as? String ?? "", relativeTo: filename)

			if let albumArt, let artKey = preparedEntry["albumArt"] as? AnyHashable {
				preparedEntry["albumArt"] = albumArt[artKey]
			}

			entries.append(preparedEntry)
		}

		return ["entries": entries, "queue": queueList]
	}
}