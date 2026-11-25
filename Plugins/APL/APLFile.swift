//
//  APLFile.swift
//  APL
//
//  Monkey's Audio Image Link File: the audio file it points at, and the
//  block range of it to play.
//

import Foundation

@objc(APLFile)
class APLFile: NSObject {
	private(set) var startBlock = 0
	private(set) var endBlock = 0
	private(set) var file: URL?

	@objc(createWithFile:)
	class func create(withFile f: String) -> APLFile? {
		return APLFile(file: f)
	}

	// rather hack-style substitution to fgets...
	private func readline(_ f: FileHandle) -> String? {
		var d = Data()
		d.reserveCapacity(100) // it will grow, here should be most expected value
		while true {
			let byte = f.readData(ofLength: 1)
			if byte.isEmpty {
				if f.offsetInFile >= UInt64(d.count) {
					try? f.seek(toOffset: f.offsetInFile - UInt64(d.count))
				}
				return nil
			}
			d.append(byte)
			if byte[0] == UInt8(ascii: "\n") { break }
		}
		// handle encoding error (in case binary data starts not from newline,
		// impossible on real apl, but who knows...)
		return String(data: d, encoding: .utf8)
	}

	private func urlForPath(_ path: String, relativeTo baseFilename: String) -> URL? {
		if path.range(of: "://") != nil {
			return URL(string: path)
		}

		var unixPath = path
		if !unixPath.hasPrefix("/") {
			// Only relative paths would have windows backslashes.
			unixPath = unixPath.replacingOccurrences(of: "\\", with: "/")

			let basePath = ((baseFilename as NSString).standardizingPath as NSString).deletingLastPathComponent + "/"
			unixPath = basePath + unixPath
		}
		return URL(string: URL(fileURLWithPath: unixPath).absoluteString)
	}

	@objc init?(file filename: String) {
		super.init()

		// startBlock must be always >= 0
		guard let f = FileHandle(forReadingAtPath: filename) else {
			NSLog("Failed to open apl file '\(filename)' for reading")
			return nil
		}
		let header = "[Monkey's Audio Image Link File]\r\n"
		let da = f.readData(ofLength: header.utf16.count)
		if da.isEmpty {
			NSLog("Cannot read header")
			return nil
		}
		guard let str = String(data: da, encoding: .ascii),
		      str.caseInsensitiveCompare(header) == .orderedSame
		else {
			NSLog("APL header mismatch")
			return nil
		}
		// now read by lines, skip empty, up to line <tagline> (or any starting
		// with '-' - may be other tags can be present)
		while let line = readline(f) {
			if line.caseInsensitiveCompare("----- APE TAG (DO NOT TOUCH!!!) -----\r\n") == .orderedSame { break }
			if line.first == "-" { break }
			let scanner = Scanner(string: line)
			guard let field = scanner.scanUpToString("="),
			      scanner.scanString("=") != nil,
			      let value = scanner.scanUpToString("\r\n")
			else { continue }
			if field.caseInsensitiveCompare("Image File") == .orderedSame {
				file = urlForPath(value, relativeTo: filename)
				#if DEBUG
					NSLog("APL refers to file '\(file?.description ?? "(null)")' read '\(value)'")
				#endif
				continue
			}
			if field.caseInsensitiveCompare("Start Block") == .orderedSame {
				startBlock = Int((value as NSString).intValue) //!!! bugs with files over 2GB
				continue
			}
			if field.caseInsensitiveCompare("Finish Block") == .orderedSame {
				endBlock = Int((value as NSString).intValue) //!!! bugs with files over 2GB
				continue
			}
		}
		f.closeFile()
	}
}