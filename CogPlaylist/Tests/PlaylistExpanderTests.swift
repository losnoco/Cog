//
//  PlaylistExpanderTests.swift
//  CogPlaylistTests
//
//  Listing what is added, as the macOS loader did. (Filtering by what the
//  plugins play needs them loaded, as the iOS app's tests have them.)
//

@testable import CogPlaylist
import XCTest

final class PlaylistExpanderTests: XCTestCase {
	private var folder: URL!

	override func setUpWithError() throws {
		folder = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString, isDirectory: true)
		for path in ["a.flac", "b.flac", "album.cue", "list.m3u", "Disc 2/c.flac"] {
			let file = folder.appendingPathComponent(path)
			try FileManager.default.createDirectory(at: file.deletingLastPathComponent(), withIntermediateDirectories: true)
			try Data([0]).write(to: file)
		}
	}

	override func tearDownWithError() throws {
		try? FileManager.default.removeItem(at: folder)
	}

	private func names(_ urls: [URL]) -> Set<String> {
		Set(urls.map { $0.path.replacingOccurrences(of: folder.resolvingSymlinksInPath().path + "/", with: "")
			.replacingOccurrences(of: folder.path + "/", with: "") })
	}

	func testFoldersListTheirFilesLeavingCueSheetsAndPlaylistsAsAsked() {
		let expander = PlaylistExpander()
		XCTAssertEqual(names(expander.fileURLs(inFolder: folder.path)), ["a.flac", "b.flac", "Disc 2/c.flac"])
		expander.readsCueSheetsInFolders = true
		expander.readsPlaylistsInFolders = true
		XCTAssertEqual(names(expander.fileURLs(inFolder: folder.path)), ["a.flac", "b.flac", "Disc 2/c.flac", "album.cue", "list.m3u"])
	}

	func testExpandingKeysEachURLOnce() {
		let expander = PlaylistExpander()
		let stream = URL(string: "http://example.com/stream.mp3")!
		let expanded = expander.expand([folder, folder.appendingPathComponent("a.flac"), stream, folder.appendingPathComponent("missing.flac")])
		XCTAssertEqual(names(Array(expanded.values).filter(\.isFileURL)), ["a.flac", "b.flac", "Disc 2/c.flac"])
		XCTAssertEqual(expanded[PlaylistExpander.key(for: stream)], stream)
		XCTAssertFalse(expanded.keys.contains { $0.contains(".") }, "periods are escaped in keys")
	}

	func testAddingAFileCanAddItsFolder() {
		let expander = PlaylistExpander()
		expander.addsOtherFilesInFolders = true
		let expanded = expander.expand([folder.appendingPathComponent("a.flac"), folder.appendingPathComponent("b.flac")])
		XCTAssertEqual(names(Array(expanded.values)), ["a.flac", "b.flac", "Disc 2/c.flac"])
	}

	func testTheSandboxIsAskedForWhatIsAdded() {
		let sandbox = RecordingSandbox()
		let expander = PlaylistExpander()
		expander.sandbox = sandbox
		_ = expander.expand([folder, folder.appendingPathComponent("a.flac")])
		XCTAssertEqual(sandbox.folders, [folder])
		XCTAssertEqual(sandbox.files, [folder.appendingPathComponent("a.flac")])
		XCTAssertEqual(sandbox.accesses, 1)
		XCTAssertEqual(sandbox.open, 0, "every folder access is ended")
	}

	func testProgressReachesTheWholeOfEachStep() {
		let expander = PlaylistExpander()
		var last = 0.0
		expander.progress = { last = $0 }
		_ = expander.expand([folder, folder.appendingPathComponent("a.flac")])
		XCTAssertEqual(last, 100, accuracy: 1e-9)
	}

	func testFinderOrderTakesDigitsAsNumbersAndIgnoresCase() {
		let sorted = ["track10", "Track2", "track1"].sorted { PlaylistExpander.finderCompare($0, $1) == .orderedAscending }
		XCTAssertEqual(sorted, ["track1", "Track2", "track10"])
	}
}

private final class RecordingSandbox: NSObject, PlaylistExpanderSandbox {
	var folders: [URL] = []
	var files: [URL] = []
	var accesses = 0
	var open = 0

	func beginAccess(toFolder url: URL) -> UnsafeRawPointer? {
		accesses += 1
		open += 1
		return UnsafeRawPointer(bitPattern: 1)
	}

	func endAccess(_ handle: UnsafeRawPointer?) {
		open -= 1
	}

	func addFolder(_ url: URL) {
		folders.append(url)
	}

	func addFile(_ url: URL) {
		files.append(url)
	}

	func requestFolder(forFile url: URL) {}
}
