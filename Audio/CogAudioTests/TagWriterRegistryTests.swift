//
//  TagWriterRegistryTests.swift
//  CogAudioTests
//
//  The tag-writing half of the plugin registry: which writer a file gets,
//  what is refused before any writer runs, and that a save is read back
//  from the file rather than from the tag cache.
//

import CogAudio
import XCTest

private let fileType = "cogtagtest"

/// Claims the extension first, but can't save anything.
final class UnwritableTagWriter: NSObject, CogMetadataWriter {
	static var writes = 0

	static func fileTypes() -> [Any]! { [fileType] }
	static func priority() -> Float { 2 }
	@objc(capabilitiesForURL:) static func capabilities(for url: URL!) -> [AnyHashable: Any]! { [:] }
	@objc(tagsForURL:) static func tags(for url: URL!) -> [AnyHashable: Any]! { ["title": ["wrong"]] }
	@objc(writeTags:toURL:replace:error:) static func writeTags(_ tags: [AnyHashable: Any]!, to url: URL!, replace: Bool) throws {
		writes += 1
	}
}

/// Below the first in priority, and able to save.
final class WritableTagWriter: NSObject, CogMetadataWriter {
	static var written: [AnyHashable: Any]?
	static var replaced = false

	static func fileTypes() -> [Any]! { [fileType] }
	static func priority() -> Float { 1 }
	@objc(capabilitiesForURL:) static func capabilities(for url: URL!) -> [AnyHashable: Any]! {
		[CogTagWriterWritable: true, CogTagWriterMultiValue: true]
	}
	@objc(tagsForURL:) static func tags(for url: URL!) -> [AnyHashable: Any]! { ["title": ["right"]] }
	@objc(writeTags:toURL:replace:error:) static func writeTags(_ tags: [AnyHashable: Any]!, to url: URL!, replace: Bool) throws {
		written = tags
		replaced = replace
	}
}

/// Counts its reads, to see whether the registry's cache answered instead.
final class CountingTagReader: NSObject, CogMetadataReader {
	static var reads = 0

	static func fileTypes() -> [Any]! { [fileType] }
	static func mimeTypes() -> [Any]! { [] }
	static func priority() -> Float { 1 }
	@objc(metadataForURL:) static func metadata(for url: URL!) -> [AnyHashable: Any]! {
		reads += 1
		return ["title": ["read \(reads)"]]
	}
}

final class TagWriterRegistryTests: XCTestCase {
	private static let registered: PluginController = {
		let controller = PluginController.shared() as! PluginController
		controller.setupMetadataWriter(NSStringFromClass(UnwritableTagWriter.self))
		controller.setupMetadataWriter(NSStringFromClass(WritableTagWriter.self))
		controller.setupMetadataReader(NSStringFromClass(CountingTagReader.self))
		return controller
	}()

	private var controller: PluginController { Self.registered }
	private let file = URL(fileURLWithPath: "/tmp/cog-tag-writer-test.\(fileType)")

	override func setUp() {
		UnwritableTagWriter.writes = 0
		WritableTagWriter.written = nil
		WritableTagWriter.replaced = false
	}

	/// The highest-priority claimant whose probe can save wins; one that
	/// can't is passed over rather than tried.
	func testWritableWriterIsChosenOverAHigherPriorityOneThatCannotSave() throws {
		let capabilities = controller.tagWritingCapabilities(for: file)
		XCTAssertEqual(capabilities?[CogTagWriterWritable] as? Bool, true)
		XCTAssertEqual(capabilities?[CogTagWriterMultiValue] as? Bool, true)
		XCTAssertEqual(controller.writableTags(for: file)?["title"] as? [String], ["right"])

		try controller.writeTags(["artist": ["someone"]], to: file, replace: true)
		XCTAssertEqual(WritableTagWriter.written?["artist"] as? [String], ["someone"])
		XCTAssertTrue(WritableTagWriter.replaced)
		XCTAssertEqual(UnwritableTagWriter.writes, 0)
	}

	/// A cue track or subsong isn't a file of its own: nothing is probed or
	/// written for it.
	func testURLWithAFragmentIsRefused() {
		let track = URL(string: file.absoluteString + "#3")!
		XCTAssertNil(controller.tagWritingCapabilities(for: track)?[CogTagWriterWritable])
		XCTAssertNil(controller.writableTags(for: track))
		XCTAssertThrowsError(try controller.writeTags(["title": ["x"]], to: track, replace: false))
		XCTAssertNil(WritableTagWriter.written)
	}

	func testExtensionWithNoWriterIsNotWritable() {
		let other = URL(fileURLWithPath: "/tmp/cog-tag-writer-test.nowriter")
		XCTAssertNil(controller.tagWritingCapabilities(for: other)?[CogTagWriterWritable])
		XCTAssertThrowsError(try controller.writeTags(["title": ["x"]], to: other, replace: false))
	}

	/// After a save the next read goes to the file, for the file and for
	/// its subsongs, instead of the cache's copy from before it.
	func testSaveForgetsCachedTags() throws {
		let track = URL(string: file.absoluteString + "#2")!
		_ = controller.metadata(for: file, skipCue: false)
		_ = controller.metadata(for: track, skipCue: true)
		let readsBefore = CountingTagReader.reads
		_ = controller.metadata(for: file, skipCue: false)
		_ = controller.metadata(for: track, skipCue: true)
		XCTAssertEqual(CountingTagReader.reads, readsBefore, "the second reads should come from the cache")

		try controller.writeTags(["title": ["new"]], to: file, replace: false)

		_ = controller.metadata(for: file, skipCue: false)
		_ = controller.metadata(for: track, skipCue: true)
		XCTAssertEqual(CountingTagReader.reads, readsBefore + 2)
	}
}
