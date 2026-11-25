//
//  FileSource.swift
//  FileSource
//
//  Created by Vincent Spader on 3/1/07.
//  Copyright 2007 Vincent Spader. All rights reserved.
//

import CogAudio
import Foundation

@objc(FileSource)
class FileSource: NSObject, CogSource {
	// fex_init once, as +initialize did.
	private static let fexInitialized: Void = { _ = fex_init() }()

	private var fex: OpaquePointer?
	private var data: UnsafeRawPointer?
	private var offset = 0
	private var size = 0

	private var fd: UnsafeMutablePointer<FILE>?

	private var urlStorage: URL?

	private var sbHandle: UnsafeRawPointer?

	@objc class func schemes() -> [Any] {
		return ["file"]
	}

	// As CogURLNormalization.h's CogNormalizeURL: fopen() cannot do anything
	// with a file reference URL, so resolve it to a plain path URL, keeping
	// any fragment.
	private static func normalizeURL(_ url: URL) -> URL {
		let nsURL = url as NSURL
		if nsURL.isFileReferenceURL() {
			return nsURL.filePathURL ?? url
		}
		return url
	}

	func open(_ url: URL) -> Bool {
		_ = Self.fexInitialized

		let url = Self.normalizeURL(url)

		urlStorage = url

		let broker = (NSClassFromString("SandboxBroker") as? CogSandboxBroker.Type)?.shared()
		sbHandle = broker?.beginFolderAccess(url)

		let path = url.path
		if path.isEmpty {
			NSLog("Invalid URL: \(url)")
			return false
		}

		var type: OpaquePointer?
		var error = fex_identify_file(&type, path)

		if error == nil, let type, strcmp(fex_type_name(type), "file") != 0 {
			error = fex_open_type(&fex, path, type)
			if error == nil {
				while fex_done(fex) == 0 {
					let name = String(cString: fex_name(fex)).lowercased()
					if name.hasSuffix(".diz") || name.hasSuffix(".txt") || name.hasSuffix(".nfo") {
						error = fex_next(fex)
						if error != nil {
							return false
						}
						continue
					}
					break
				}
				if fex_done(fex) != 0 {
					return false
				}

				var dataOut: UnsafeRawPointer?
				error = fex_data(fex, &dataOut)
				if error != nil {
					return false
				}
				data = dataOut

				size = Int(fex_size(fex))
				offset = 0

				return true
			} else {
				return false
			}
		}

		fd = fopen(path, "r")

		return fd != nil
	}

	func seekable() -> Bool {
		return true
	}

	func seek(_ position: Int, whence: Int32) -> Bool {
		var position = position
		if fex != nil {
			switch whence {
			case SEEK_CUR:
				position += offset
			case SEEK_END:
				position += size
			default:
				break
			}

			offset = position

			return position >= 0 && position < size
		}

		guard let fd else {
			offset = 0
			size = 0
			return false
		}

		return fseeko(fd, off_t(position), whence) == 0
	}

	func tell() -> Int {
		if fex != nil {
			return offset
		}
		guard let fd else { return 0 }
		return Int(ftello(fd))
	}

	func read(_ buffer: UnsafeMutableRawPointer, amount: Int) -> Int {
		if fex != nil {
			var amount = amount
			if amount + offset > size {
				amount = max(0, size - offset)
			}
			memcpy(buffer, data?.advanced(by: offset), amount)
			offset += amount
			return amount
		} else {
			return fread(buffer, 1, amount, fd)
		}
	}

	func close() {
		if let fd {
			fclose(fd)
			self.fd = nil
		}

		if let fex {
			fex_close(fex)
			self.fex = nil
		}

		if let sbHandle {
			let broker = (NSClassFromString("SandboxBroker") as? CogSandboxBroker.Type)?.shared()
			broker?.endFolderAccess(sbHandle)
			self.sbHandle = nil
		}
	}

	func filesBesideURL(_ url: URL) -> [AnyHashable: Any]? {
		let url = Self.normalizeURL(url)
		let folder = (url.path as NSString).deletingLastPathComponent

		let broker = (NSClassFromString("SandboxBroker") as? CogSandboxBroker.Type)?.shared()

		let handle = broker?.beginFolderAccess(url)
		let names = (try? FileManager.default.contentsOfDirectory(atPath: folder)) ?? []
		broker?.endFolderAccess(handle)

		var files: [String: URL] = [:]
		for name in names where !name.hasPrefix(".") {
			files[name] = URL(fileURLWithPath: (folder as NSString).appendingPathComponent(name))
		}
		return files
	}

	func url() -> URL? {
		return urlStorage
	}

	func mimeType() -> String? {
		return nil
	}

	deinit {
		close()
	}
}