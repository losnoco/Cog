//
//  PlsContainer.swift
//  Pls
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

import CogAudio
import Foundation

@objc(PlsContainer)
class PlsContainer: NSObject, CogContainer {
	@objc class func fileTypes() -> [Any] {
		return ["pls"]
	}

	@objc class func mimeTypes() -> [Any] {
		return ["audio/x-scpls", "application/pls"]
	}

	@objc class func priority() -> Float {
		return 1.0
	}

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

	@objc(urlsForContainerURL:)
	class func urls(forContainerURL url: URL) -> [Any] {
		var data = Data()

		do {
			guard let source = AudioSource.audioSource(for: url) else {
				return []
			}

			if !source.open(url) {
				return []
			}

			var chunk = [UInt8](repeating: 0, count: 1024)
			while true {
				let bytesRead = chunk.withUnsafeMutableBytes {
					source.read($0.baseAddress!, amount: 1024)
				}
				if bytesRead > 0 {
					data.append(contentsOf: chunk[0 ..< bytesRead])
				}
				if bytesRead < 1024 {
					break
				}
			}
		}

		// Handle macOS Classic and Windows line endings
		for index in data.indices where data[index] == UInt8(ascii: "\r") {
			data[index] = UInt8(ascii: "\n")
		}
		// The file was read as a C string, which ends at the first NUL.
		if let nul = data.firstIndex(of: 0) {
			data = data[..<nul]
		}

		#if DEBUG
			NSLog("Trying UTF8")
		#endif
		var contents = String(data: data, encoding: .utf8)
		if contents == nil {
			#if DEBUG
				NSLog("Trying windows GB 18030 2000")
			#endif
			let gb18030 = String.Encoding(rawValue: CFStringConvertEncodingToNSStringEncoding(CFStringEncoding(CFStringEncodings.GB_18030_2000.rawValue)))
			contents = String(data: data, encoding: gb18030)
		}
		if contents == nil {
			#if DEBUG
				NSLog("Trying windows CP1251")
			#endif
			contents = String(data: data, encoding: .windowsCP1251)
		}
		if contents == nil {
			#if DEBUG
				NSLog("Trying latin1")
			#endif
			contents = String(data: data, encoding: .isoLatin1)
		}
		guard let contents else {
			NSLog("Could not open file...\(url)")
			return []
		}

		var entries: [URL] = []

		for entry in contents.components(separatedBy: "\n") {
			let line = entry.trimmingCharacters(in: .whitespacesAndNewlines)

			let scanner = Scanner(string: line)

			guard let lhs = scanner.scanUpToString("="), // get LHS
			      scanner.scanString("=") != nil // skip the =
			else {
				continue
			}
			// The scanner skips whitespace before the value, as NSScanner did.
			let rhs = String(scanner.string[scanner.currentIndex...]).trimmingCharacters(in: .whitespacesAndNewlines) // get RHS
			guard !rhs.isEmpty,
			      lhs.range(of: "File", options: [.caseInsensitive, .anchored]) != nil // We only want file entries
			else {
				continue
			}

			// need to add basepath if its a file, and convert to URL
			if let fileUrl = urlForPath(rhs, relativeTo: url.path) {
				entries.append(fileUrl)
			}
		}

		return entries
	}
}