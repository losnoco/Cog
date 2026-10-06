//
//  PlaylistExpander.swift
//  CogPlaylist
//
//  What a set of added URLs stands for: folders listed, containers (cue
//  sheets, playlists, archives, multi-track files) opened through the
//  plugins, and the result de-duplicated, ordered and filtered to what Cog
//  plays. Moved from the macOS app's PlaylistLoader.m (fileURLsAtPath:,
//  expandURLs:underTask:, the container pass of insertURLs:atIndex:sort:,
//  and validURLsFrom:unique:sort:underTask:), in the same three steps, so
//  the app can report progress between them. One expander serves one add.
//

import CogAudio
#if os(macOS)
import CoreServices
#endif
import Foundation

/// Access to files outside the app's own, which the macOS app's sandbox
/// broker grants and keeps.
@objc public protocol PlaylistExpanderSandbox: NSObjectProtocol {
	@objc(beginAccessToFolder:) func beginAccess(toFolder url: URL) -> UnsafeRawPointer?
	@objc(endAccess:) func endAccess(_ handle: UnsafeRawPointer?)
	@objc(addFolder:) func addFolder(_ url: URL)
	@objc(addFile:) func addFile(_ url: URL)
	@objc(requestFolderForFile:) func requestFolder(forFile url: URL)
}

@objc public final class PlaylistExpander: NSObject {
	@objc public var sandbox: PlaylistExpanderSandbox?
	/// Whether a listed folder's cue sheets, and its playlists, are added.
	@objc public var readsCueSheetsInFolders = false
	@objc public var readsPlaylistsInFolders = false
	/// Whether adding a file adds the rest of its folder.
	@objc public var addsOtherFilesInFolders = false
	/// Whether files a container plays from (a cue sheet's audio file) are
	/// left out, being its tracks rather than entries of their own.
	@objc public var skipsContainerDependencies = false
	/// Told each step's progress, from 0 to 100.
	@objc public var progress: ((Double) -> Void)?

	/// XML playlists met while opening containers, which the macOS app reads
	/// itself.
	@objc public private(set) var xmlPlaylistURLs: [URL] = []

	/// Every URL seen so far, so each is added once.
	private var unique = Set<URL>()
	private var dependencies = Set<String>()
	private let lock = NSLock()

	// MARK: - Listing

	/// The files and streams `urls` stand for, folders listed, keyed for
	/// de-duplication.
	@objc public func expand(_ urls: [URL]) -> [String: URL] {
		var expanded: [String: URL] = [:]
		var listedFolders = Set<URL>()
		let step = urls.isEmpty ? 0 : 100 / Double(urls.count)
		var done = 0.0
		for original in urls {
			// Drops and services can hand over file reference URLs, which most
			// of the player cannot open and which must never be stored.
			let url = PlaylistEntry.normalize(original)
			defer {
				done += step
				progress?(done)
			}
			if url.isFileURL {
				var isDirectory: ObjCBool = false
				guard FileManager.default.fileExists(atPath: url.path, isDirectory: &isDirectory) else { continue }
				if isDirectory.boolValue {
					sandbox?.addFolder(url)
					for file in fileURLs(inFolder: url.path) {
						expanded[Self.key(for: file)] = file
					}
				} else if addsOtherFilesInFolders {
					let folder = url.deletingLastPathComponent()
					if listedFolders.insert(folder).inserted {
						sandbox?.requestFolder(forFile: url)
						for file in fileURLs(inFolder: folder.path) {
							expanded[Self.key(for: file)] = file
						}
					}
				} else {
					sandbox?.addFile(url)
					expanded[Self.key(for: url)] = url
				}
			} else {
				expanded[Self.key(for: url)] = url
			}
		}
		return expanded
	}

	/// The files in a folder and its subfolders, cue sheets and playlists as
	/// the preferences have them.
	func fileURLs(inFolder path: String) -> [URL] {
		let handle = sandbox?.beginAccess(toFolder: URL(fileURLWithPath: path))
		let subpaths = FileManager.default.subpaths(atPath: path) ?? []
		sandbox?.endAccess(handle)

		var urls: [URL] = []
		for subpath in subpaths {
			let absolute = (path as NSString).appendingPathComponent(subpath)
			var isDirectory: ObjCBool = false
			guard FileManager.default.fileExists(atPath: absolute, isDirectory: &isDirectory), !isDirectory.boolValue else { continue }
			switch (absolute as NSString).pathExtension.lowercased() {
			case "cue": if !readsCueSheetsInFolders { continue }
			case "m3u", "m3u8", "pls": if !readsPlaylistsInFolders { continue }
			default: break
			}
			urls.append(URL(fileURLWithPath: absolute))
		}
		return urls
	}

	/// The key that identifies a URL among those added, its periods escaped
	/// so a key path never splits it.
	@objc(keyForURL:) public static func key(for url: URL) -> String {
		url.absoluteString.replacingOccurrences(of: ".", with: "%2E")
	}

	// MARK: - Opening containers

	/// Each expanded URL as itself, or, for a container, the URLs inside it,
	/// several at a time. A container no plugin can open stays a file.
	@objc public func openContainers(_ expanded: [String: URL]) -> [String: Any] {
		let containerTypes = Set(((AudioPlayer.containerTypes() as? [String]) ?? []).map { $0.lowercased() })
		let items = Array(expanded)
		var loaded: [String: Any] = [:]
		let step = items.isEmpty ? 0 : 100 / Double(items.count)
		var done = 0.0

		let queue = OperationQueue()
		queue.maxConcurrentOperationCount = 8
		for (key, url) in items {
			queue.addOperation { [self] in
				defer {
					lock.lock()
					done += step
					progress?(done)
					lock.unlock()
				}
				lock.lock()
				let seen = unique.contains(url)
				lock.unlock()
				if seen { return }

				let type = url.pathExtension.lowercased()
				if containerTypes.contains(type) {
					let inner = PluginCalls.run(for: url) { AudioContainer.urls(forContainerURL: url) as? [URL] } ?? []
					if inner.isEmpty {
						// Every container parser failed: the raw file, then.
						lock.lock()
						loaded[key] = url
						lock.unlock()
						return
					}
					let dependencies = PluginCalls.run(for: url) { AudioContainer.dependencyUrls(forContainerURL: url) as? [URL] } ?? []
					lock.lock()
					loaded[key] = inner
					// So the container is not added as well.
					unique.insert(url)
					self.dependencies.formUnion(dependencies.map(\.absoluteString))
					lock.unlock()
					if inner.contains(where: \.isFileURL) || dependencies.contains(where: \.isFileURL) {
						sandbox?.requestFolder(forFile: url)
					}
				} else if type == "xml" {
					lock.lock()
					xmlPlaylistURLs.append(url)
					lock.unlock()
				} else {
					lock.lock()
					loaded[key] = url
					lock.unlock()
				}
			}
		}
		queue.waitUntilAllOperationsAreFinished()
		return loaded
	}

	// MARK: - Filtering

	/// The URLs to add, in order: each container's contents, and each other
	/// file not already among them, that has a supported scheme and, for a
	/// local file, a supported extension. With `sort`, in Finder's order of
	/// their keys.
	@objc(validURLsFrom:sort:) public func validURLs(from loaded: [String: Any], sort: Bool) -> [URL] {
		var keys = Array(loaded.keys)
		if sort {
			keys.sort { Self.finderCompare($0, $1) == .orderedAscending }
		}
		let values = keys.map { loaded[$0]! }

		// Containers' contents first, so a file a container already holds is
		// not added again on its own...
		for case let inner as [URL] in values {
			unique.formUnion(inner)
		}
		// ...then the other files, once each, and every container's contents.
		var fileURLs: [URL] = []
		for value in values {
			if let url = value as? URL {
				if unique.insert(url).inserted {
					fileURLs.append(url)
				}
			} else if let inner = value as? [URL] {
				fileURLs.append(contentsOf: inner)
			}
		}

		let schemes = Set((AudioPlayer.schemes() as? [String]) ?? [])
		let fileTypes = Set((AudioPlayer.fileTypes() as? [String]) ?? [])
		let step = fileURLs.isEmpty ? 0 : 100 / Double(fileURLs.count)
		var done = 0.0
		var valid: [URL] = []
		for url in fileURLs {
			done += step
			defer { progress?(done) }
			guard let scheme = url.scheme, schemes.contains(scheme) else { continue }
			if url.isFileURL && !fileTypes.contains(url.pathExtension.lowercased()) { continue }
			if skipsContainerDependencies && dependencies.contains(url.absoluteString) { continue }
			valid.append(url)
		}
		return valid
	}

	/// All three steps, for an app with no progress to show between them.
	public func urls(for urls: [URL], sort: Bool) -> [URL] {
		validURLs(from: openContainers(expand(urls)), sort: sort)
	}

	/// Finder's order: case, width and composition aside, digits as numbers.
	static func finderCompare(_ lhs: String, _ rhs: String) -> ComparisonResult {
		#if os(macOS)
		let left = Array(lhs.utf16)
		let right = Array(rhs.utf16)
		var result: Int32 = 0
		let options = UInt32(kUCCollateComposeInsensitiveMask | kUCCollateWidthInsensitiveMask | kUCCollateCaseInsensitiveMask |
		                     kUCCollateDigitsOverrideMask | kUCCollateDigitsAsNumberMask | kUCCollatePunctuationSignificantMask)
		_ = UCCompareTextDefault(options, left, left.count, right, right.count, nil, &result)
		// It orders by sign, not by -1 and 1 (it gives 2 and -2).
		return result < 0 ? .orderedAscending : result > 0 ? .orderedDescending : .orderedSame
		#else
		return lhs.localizedStandardCompare(rhs)
		#endif
	}
}
