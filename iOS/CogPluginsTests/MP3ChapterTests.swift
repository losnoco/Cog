//
//  MP3ChapterTests.swift
//  CogPluginsTests
//
//  The minimp3 decoder playing "file.mp3#N", the chapter rows
//  FFMPEGContainer lists: its span from the ID3v2 chapters the TagLib
//  reader answers, its title the chapter's, and the next chapter carried
//  on to with setTrack: rather than reopened. The spans must match what
//  FFmpeg's decoder plays for the same rows, which number them by start.
//

@testable import CogAudio
import XCTest

final class MP3ChapterTests: XCTestCase {
	private var directory: URL!

	override func setUpWithError() throws {
		_ = PluginController.shared()
		directory = FileManager.default.temporaryDirectory.appendingPathComponent(UUID().uuidString)
		try FileManager.default.createDirectory(at: directory, withIntermediateDirectories: true)
	}

	override func tearDownWithError() throws {
		try? FileManager.default.removeItem(at: directory)
	}

	private var plugins: CogPluginController { PluginController.shared() }

	// MARK: - Fixtures

	/// About a second of silent MPEG-1 Layer III frames, as TagWriterTests
	/// builds them: AVAudioFile can't write MP3. 40 frames of 1152 samples
	/// at 44100 Hz.
	private func writeMP3(named name: String = "chapters.mp3") throws -> URL {
		let url = directory.appendingPathComponent(name)
		var frame = Data([0xFF, 0xFB, 0x90, 0x00])
		frame.append(Data(count: 417 - 4))
		var data = Data()
		for _ in 0..<40 { data.append(frame) }
		try data.write(to: url)
		return url
	}

	private func writeChapters(to url: URL, replace: Bool = true) throws {
		let chapters: [[String: Any]] = [
			[CogChapterStart: 600, CogChapterTitle: "Third"],
			[CogChapterStart: 0, CogChapterTitle: "First"],
			[CogChapterStart: 300, CogChapterTitle: "Second"],
		]
		try plugins.writeTags(["title": ["The Book"], CogTagChapters: chapters], to: url, replace: replace)
	}

	private func trackURL(_ file: URL, _ index: Int) -> URL {
		URL(string: file.absoluteString + "#\(index)")!
	}

	private func openMP3Decoder(_ url: URL) throws -> CogDecoder {
		let source = try XCTUnwrap(plugins.audioSource(for: url))
		XCTAssertTrue(source.open(url))
		let decoder = try XCTUnwrap((NSClassFromString("MP3Decoder") as? NSObject.Type)?.init() as? CogDecoder)
		XCTAssertTrue(decoder.open(source), "MP3Decoder would not open \(url)")
		return decoder
	}

	private func totalFrames(of decoder: CogDecoder) -> Int {
		(decoder.properties()?["totalFrames"] as? NSNumber)?.intValue ?? -1
	}

	// A decoder's metadata values are plain strings (minimp3) or arrays of
	// strings (FFmpeg); read the first either way.
	private func metadataValue(_ decoder: CogDecoder, _ key: String) -> String {
		guard let value = decoder.metadata()?[key] else { return "" }
		if let string = value as? String { return string }
		if let array = value as? [String] { return array.first ?? "" }
		return ""
	}

	private func title(of decoder: CogDecoder) -> String {
		metadataValue(decoder, "title")
	}

	private func album(of decoder: CogDecoder) -> String {
		metadataValue(decoder, "album")
	}

	private func readToEnd(_ decoder: CogDecoder) -> Int {
		var frames = 0
		while let chunk = decoder.readAudio(), chunk.frameCount() > 0 {
			frames += Int(chunk.frameCount())
		}
		return frames
	}

	/// The first channel's samples, read sequentially.
	private func channel0(_ decoder: CogDecoder) -> [Float] {
		var out: [Float] = []
		while let chunk = decoder.readAudio(), chunk.frameCount() > 0 {
			let data = chunk.removeSamples(chunk.frameCount())
			let channels = Int(chunk.format.mChannelsPerFrame)
			data.withUnsafeBytes { raw in
				let floats = raw.bindMemory(to: Float.self)
				var i = 0
				while i < floats.count {
					out.append(floats[i])
					i += channels
				}
			}
		}
		return out
	}

	private func maxAbsDiff(_ a: [Float], _ b: [Float], count: Int) -> Float {
		var diff: Float = 0
		for i in 0..<count {
			let d = abs(a[i] - b[i])
			if d > diff { diff = d }
		}
		return diff
	}

	/// The span the decoder should play: the chapter's milliseconds at the
	/// file's rate, its end clamped to the file's length.
	private func expectedSpan(_ chapter: [String: Any], fileFrames: Int) -> Int {
		let startMs = (chapter[CogChapterStart] as? NSNumber)?.uint64Value ?? 0
		let endMs = (chapter[CogChapterEnd] as? NSNumber)?.uint64Value ?? 0
		let startFrame = Int((startMs * 44100 + 500) / 1000)
		let endFrame = min(Int((endMs * 44100 + 500) / 1000), fileFrames)
		return endFrame - startFrame
	}

	private func writtenChapters(_ url: URL) throws -> [[String: Any]] {
		try XCTUnwrap(plugins.writableTags(for: url)?[CogTagChapters] as? [[String: Any]])
	}

	// MARK: - The span and the title

	func testTheDecoderPlaysEachChaptersSpan() throws {
		let url = try writeMP3()
		try writeChapters(to: url)
		let fileDecoder = try openMP3Decoder(url)
		let fileFrames = totalFrames(of: fileDecoder)
		XCTAssertEqual(fileFrames, 40 * 1152)
		fileDecoder.close()

		for (index, chapter) in try writtenChapters(url).enumerated() {
			let decoder = try openMP3Decoder(trackURL(url, index))
			XCTAssertEqual(totalFrames(of: decoder), expectedSpan(chapter, fileFrames: fileFrames), "chapter \(index)")
			XCTAssertEqual(title(of: decoder), chapter[CogChapterTitle] as? String, "chapter \(index)")
			XCTAssertEqual(album(of: decoder), "The Book", "the file's title is the chapter's album")
			XCTAssertEqual(readToEnd(decoder), totalFrames(of: decoder), "chapter \(index) plays its span and stops")
			decoder.close()
		}
	}

	func testTheWholeFileStillPlaysWithoutAFragment() throws {
		let url = try writeMP3()
		try writeChapters(to: url)
		let decoder = try openMP3Decoder(url)
		XCTAssertEqual(totalFrames(of: decoder), 40 * 1152)
		XCTAssertEqual(readToEnd(decoder), 40 * 1152)
		decoder.close()
	}

	func testTheLoaderReadsTheChaptersLengthAndTitle() throws {
		let url = try writeMP3()
		try writeChapters(to: url)
		// The properties path the playlist loader reads, which for MP3 is
		// the decoder's own.
		let info = try XCTUnwrap(plugins.properties(for: trackURL(url, 1), skipCue: false) as? [String: Any])
		XCTAssertEqual((info["totalFrames"] as? NSNumber)?.intValue, 13230, "300 ms at 44100")
		XCTAssertEqual(info["title"] as? String, "Second")
	}

	// MARK: - Agreement with the container and FFmpeg

	func testTheContainerAndFFmpegAgreeOnTheSpans() throws {
		let url = try writeMP3()
		try writeChapters(to: url)
		let tracks = try XCTUnwrap(plugins.urls(forContainerURL: url) as? [URL])
		XCTAssertEqual(tracks.map(\.fragment), ["0", "1", "2"])

		for (index, track) in tracks.enumerated() {
			let mine = try openMP3Decoder(track)
			let source = try XCTUnwrap(plugins.audioSource(for: track))
			XCTAssertTrue(source.open(track))
			let ffmpeg = try XCTUnwrap((NSClassFromString("FFMPEGDecoder") as? NSObject.Type)?.init() as? CogDecoder)
			XCTAssertTrue(ffmpeg.open(source))
			XCTAssertEqual(Double(totalFrames(of: mine)), Double(totalFrames(of: ffmpeg)), accuracy: 1, "chapter \(index)")
			XCTAssertEqual(title(of: mine), title(of: ffmpeg), "chapter \(index)")
			XCTAssertEqual(album(of: mine), album(of: ffmpeg), "chapter \(index) album")
			mine.close()
			ffmpeg.close()
		}
	}

	// MARK: - Refusals that pass the row to FFmpeg

	func testAFragmentWithoutChaptersIsRefused() throws {
		let url = try writeMP3()
		let track = trackURL(url, 0)
		let source = try XCTUnwrap(plugins.audioSource(for: track))
		XCTAssertTrue(source.open(track))
		let decoder = try XCTUnwrap((NSClassFromString("MP3Decoder") as? NSObject.Type)?.init() as? CogDecoder)
		XCTAssertFalse(decoder.open(source), "no chapters, so nothing to play for the fragment")

		// The registry's next claimant takes it.
		XCTAssertTrue(source.seek(0, whence: SEEK_SET))
		let multi = try XCTUnwrap(plugins.audioDecoder(for: source, skipCue: false))
		XCTAssertTrue(multi.open(source))
		XCTAssertEqual((multi.properties()?["codec"] as? String)?.lowercased(), "mp3", "FFmpeg plays it")
		multi.close()
	}

	func testAnOutOfRangeFragmentIsRefused() throws {
		let url = try writeMP3()
		try writeChapters(to: url)
		let track = trackURL(url, 3)
		let source = try XCTUnwrap(plugins.audioSource(for: track))
		XCTAssertTrue(source.open(track))
		let decoder = try XCTUnwrap((NSClassFromString("MP3Decoder") as? NSObject.Type)?.init() as? CogDecoder)
		XCTAssertFalse(decoder.open(source))
	}

	func testANonNumericFragmentIsRefused() throws {
		let url = try writeMP3()
		try writeChapters(to: url)
		// intValue would take the leading number of "1x" or "-1"; the whole
		// fragment must be the index.
		for fragment in ["1x", "-1", "abc", "1.5", " 1"] {
			let track = URL(string: url.absoluteString + "#" + fragment)!
			let source = try XCTUnwrap(plugins.audioSource(for: track))
			XCTAssertTrue(source.open(track))
			let decoder = try XCTUnwrap((NSClassFromString("MP3Decoder") as? NSObject.Type)?.init() as? CogDecoder)
			XCTAssertFalse(decoder.open(source), "fragment '\(fragment)' is not a chapter index")
		}
	}

	// MARK: - Seeking

	func testSeekStaysInsideTheChapter() throws {
		let url = try writeMP3()
		try writeChapters(to: url)
		let decoder = try openMP3Decoder(trackURL(url, 1))
		let span = totalFrames(of: decoder)
		XCTAssertEqual(span, 13230)

		XCTAssertEqual(decoder.seek(1000), 1000)
		XCTAssertEqual(readToEnd(decoder), span - 1000)

		// Back to the start after the chapter's end: it plays again.
		XCTAssertEqual(decoder.seek(0), 0)
		XCTAssertEqual(readToEnd(decoder), span)

		// Past the end clamps to it.
		XCTAssertEqual(decoder.seek(span + 5000), span)
		XCTAssertEqual(readToEnd(decoder), 0)
		decoder.close()
	}

	// MARK: - Carrying on to the next chapter

	func testSetTrackCarriesOnToTheNextChapter() throws {
		let url = try writeMP3()
		try writeChapters(to: url)
		let chapters = try writtenChapters(url)
		let fileDecoder = try openMP3Decoder(url)
		let fileFrames = totalFrames(of: fileDecoder)
		fileDecoder.close()

		let decoder = try openMP3Decoder(trackURL(url, 0))
		XCTAssertEqual(readToEnd(decoder), expectedSpan(chapters[0], fileFrames: fileFrames))

		// The contiguous next chapter, as the feeder hands it over.
		XCTAssertEqual(decoder.setTrack?(trackURL(url, 1)), true)
		XCTAssertEqual(totalFrames(of: decoder), expectedSpan(chapters[1], fileFrames: fileFrames))
		XCTAssertEqual(title(of: decoder), "Second")
		XCTAssertEqual(readToEnd(decoder), expectedSpan(chapters[1], fileFrames: fileFrames))

		// A jump to a later chapter seeks instead.
		XCTAssertEqual(decoder.setTrack?(trackURL(url, 2)), true)
		XCTAssertEqual(title(of: decoder), "Third")
		XCTAssertEqual(readToEnd(decoder), expectedSpan(chapters[2], fileFrames: fileFrames))

		// Another file is not ours to carry.
		let other = try writeMP3(named: "other.mp3")
		XCTAssertEqual(decoder.setTrack?(trackURL(other, 0)), false)
		decoder.close()
	}

	// MARK: - An embedded cue sheet wins over chapters

	func testAnEmbeddedCueSheetIgnoresChapters() throws {
		let url = try writeMP3()
		// A file with both an embedded cue sheet and CHAP frames: the cue
		// rows (priority 16) win, and their bounds come from the whole file,
		// not a chapter span the inner decoder might mistake the fragment for.
		let sheet = """
		FILE "chapters.mp3" MP3
		  TRACK 01 AUDIO
		    INDEX 01 00:00:00
		  TRACK 02 AUDIO
		    INDEX 01 00:00:50
		"""
		let chapters: [[String: Any]] = [
			[CogChapterStart: 0, CogChapterTitle: "First"],
			[CogChapterStart: 300, CogChapterTitle: "Second"],
			[CogChapterStart: 600, CogChapterTitle: "Third"],
		]
		try plugins.writeTags(["title": ["The Book"], "cuesheet": [sheet], CogTagChapters: chapters], to: url, replace: true)

		let fileFrames = 40 * 1152
		let secondStart = 50 * 44100 / 75 // INDEX 01 00:00:50, cue frames are 1/75 s
		for (fragment, span) in [("01", secondStart), ("02", fileFrames - secondStart)] {
			let track = URL(string: url.absoluteString + "#" + fragment)!
			let source = try XCTUnwrap(plugins.audioSource(for: track))
			XCTAssertTrue(source.open(track))
			let decoder = try XCTUnwrap(plugins.audioDecoder(for: source, skipCue: false))
			XCTAssertTrue(decoder.open(source), "cue track \(fragment)")
			XCTAssertEqual(totalFrames(of: decoder), span, "cue track \(fragment) span")
			XCTAssertEqual(readToEnd(decoder), span, "cue track \(fragment) plays its span")
			decoder.close()
		}
	}

	// MARK: - Gapless chapter 0 and iTunSMPB (real LAME fixtures)

	func testChapterZeroStartsAtTheFirstRealSample() throws {
		let url = directory.appendingPathComponent("gapless.mp3")
		try MP3Fixtures.write(MP3Fixtures.cbrStereo, to: url)
		let whole = try openMP3Decoder(url)
		let reference = channel0(whole)
		let fileFrames = totalFrames(of: whole)
		whole.close()
		XCTAssertEqual(fileFrames, 52920, "the LAME tag's delay/padding trim 1.2 s to 52920 samples")
		XCTAssertEqual(reference.count, 52920)

		try writeChapters(to: url)
		let decoder = try openMP3Decoder(trackURL(url, 0))
		let span = totalFrames(of: decoder)
		let got = channel0(decoder)
		decoder.close()
		XCTAssertEqual(got.count, span)
		// Chapter 0 at ms 0 is ex sample 0, the first *real* sample: it must
		// equal the whole file's opening samples, gapless-correct by construction.
		XCTAssertLessThan(maxAbsDiff(got, reference, count: span), 1e-4, "chapter 0 starts at the first real sample")
	}

	func testITunSMPBDurationOverridesTheXingTag() throws {
		let url = directory.appendingPathComponent("itunsmpb_alt.mp3")
		try MP3Fixtures.write(MP3Fixtures.itunSMPBAlt, to: url)
		let decoder = try openMP3Decoder(url)
		// The COMM says 52000; the Xing/LAME tag says 52920. iTunSMPB wins.
		XCTAssertEqual(totalFrames(of: decoder), 52000)
		decoder.close()
	}

	func testITunSMPBAndChaptersCompose() throws {
		let url = directory.appendingPathComponent("itunsmpb.mp3")
		try MP3Fixtures.write(MP3Fixtures.itunSMPB, to: url)
		let whole = try openMP3Decoder(url)
		let reference = channel0(whole)
		let fileFrames = totalFrames(of: whole)
		whole.close()
		XCTAssertEqual(fileFrames, 52920)
		XCTAssertEqual(reference.count, 52920)

		// Merge chapters in without dropping the iTunSMPB COMM frame.
		try writeChapters(to: url, replace: false)
		let bytes = try Data(contentsOf: url)
		XCTAssertNotNil(bytes.range(of: Data("iTunSMPB".utf8)), "the merge kept the iTunSMPB frame")

		let decoder = try openMP3Decoder(trackURL(url, 0))
		let span = totalFrames(of: decoder)
		let got = channel0(decoder)
		decoder.close()
		XCTAssertEqual(got.count, span)
		// The span lives in the same post-padding space iTunSMPB defines, so
		// chapter 0 still opens on the first real sample.
		XCTAssertLessThan(maxAbsDiff(got, reference, count: span), 1e-4, "iTunSMPB and chapters share the gapless space")
	}

	// MARK: - Agreement across reader, container and decoders (both writers)

	/// The editor's chapter N, the container's row N and the decoders' span N
	/// must be the same chapter. Walk the boundaries cumulatively: the running
	/// sum of the spans lands at each chapter's start, and the spans tile the
	/// whole file. The last chapter's tail is not asserted to the frame —
	/// minimp3 trims the gapless padding while FFmpeg reports the raw length.
	private func assertChapterAgreement(_ url: URL, _ label: String) throws {
		let editorChapters = try writtenChapters(url)
		let tracks = try XCTUnwrap(plugins.urls(forContainerURL: url) as? [URL])
		XCTAssertEqual(editorChapters.count, tracks.count, "\(label): chapter count")
		XCTAssertGreaterThanOrEqual(tracks.count, 2, "\(label): need chapters to agree on")

		let whole = try openMP3Decoder(url)
		let fileFrames = totalFrames(of: whole)
		whole.close()

		var cumulative = 0
		for (i, track) in tracks.enumerated() {
			XCTAssertEqual(track.fragment, "\(i)", "\(label): row order")
			let chapter = editorChapters[i]
			let startMs = (chapter[CogChapterStart] as? NSNumber)?.uint64Value ?? 0
			let startFrame = Int((startMs * 44100 + 500) / 1000)
			XCTAssertLessThanOrEqual(abs(cumulative - startFrame), 1, "\(label): chapter \(i) start")

			let mine = try openMP3Decoder(track)
			let span = totalFrames(of: mine)
			XCTAssertEqual(title(of: mine), chapter[CogChapterTitle] as? String, "\(label): chapter \(i) title")

			let source = try XCTUnwrap(plugins.audioSource(for: track))
			XCTAssertTrue(source.open(track))
			let ffmpeg = try XCTUnwrap((NSClassFromString("FFMPEGDecoder") as? NSObject.Type)?.init() as? CogDecoder)
			XCTAssertTrue(ffmpeg.open(source))
			XCTAssertEqual(title(of: mine), title(of: ffmpeg), "\(label): chapter \(i) title vs FFmpeg")

			if i < tracks.count - 1 {
				XCTAssertLessThanOrEqual(abs(span - totalFrames(of: ffmpeg)), 1, "\(label): chapter \(i) span vs FFmpeg")
				let endMs = (chapter[CogChapterEnd] as? NSNumber)?.uint64Value ?? 0
				let editorSpan = Int((endMs * 44100 + 500) / 1000) - startFrame
				XCTAssertLessThanOrEqual(abs(span - editorSpan), 1, "\(label): chapter \(i) span vs editor")
			}
			cumulative += span
			mine.close()
			ffmpeg.close()
		}
		XCTAssertLessThanOrEqual(abs(cumulative - fileFrames), 1, "\(label): the spans tile the file")
	}

	func testTagLibWrittenChaptersAgreeAcrossReaderContainerAndDecoders() throws {
		let url = directory.appendingPathComponent("taglib.mp3")
		try MP3Fixtures.write(MP3Fixtures.cbrStereo, to: url)
		try writeChapters(to: url)
		try assertChapterAgreement(url, "TagLib")
	}

	func testFFmpegWrittenChaptersAgreeAcrossReaderContainerAndDecoders() throws {
		let url = directory.appendingPathComponent("ffmpeg.mp3")
		try MP3Fixtures.write(MP3Fixtures.ffmpegChapters, to: url)
		try assertChapterAgreement(url, "ffmpeg")
	}
}