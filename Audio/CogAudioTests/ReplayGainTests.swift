//
//  ReplayGainTests.swift
//  CogAudioTests
//
//  Created by Christopher Snowhill on 9/30/26.
//

import CogAudio
import XCTest

/// The port of `ConverterNode -refreshVolumeScaling`.
final class ReplayGainTests: XCTestCase {
	private let info: [AnyHashable: Any] = [
		"volume": 0.8,
		"soundcheck": 0.7,
		"replayGainTrackGain": -6.0,
		"replayGainTrackPeak": 0.9,
		"replayGainAlbumGain": -3.0,
		"replayGainAlbumPeak": 1.2,
	]

	private func db(_ value: Float) -> Float { powf(10, value / 20) }

	func testNoInfoMeansUnity() {
		XCTAssertEqual(ReplayGain.linearGain(rgInfo: nil, scaling: "albumGain"), 1)
		XCTAssertEqual(ReplayGain.linearGain(rgInfo: [:], scaling: "albumGain"), 1)
	}

	func testNoScalingIgnoresTheInfo() {
		XCTAssertEqual(ReplayGain.linearGain(rgInfo: info, scaling: "none"), 1)
	}

	func testEachModeUsesItsSource() {
		XCTAssertEqual(ReplayGain.linearGain(rgInfo: info, scaling: "volumeScale"), 0.8, accuracy: 1e-6)
		XCTAssertEqual(ReplayGain.linearGain(rgInfo: info, scaling: "soundcheck"), 0.7, accuracy: 1e-6)
		XCTAssertEqual(ReplayGain.linearGain(rgInfo: info, scaling: "trackGain"), db(-6), accuracy: 1e-6)
		XCTAssertEqual(ReplayGain.linearGain(rgInfo: info, scaling: "albumGain"), db(-3), accuracy: 1e-6)
	}

	func testMoreSpecificModesFallBack() {
		// Album gain requested, only track gain present.
		let trackOnly: [AnyHashable: Any] = ["replayGainTrackGain": -6.0, "volume": 0.8]
		XCTAssertEqual(ReplayGain.linearGain(rgInfo: trackOnly, scaling: "albumGain"), db(-6), accuracy: 1e-6)
		// Only a volume.
		XCTAssertEqual(ReplayGain.linearGain(rgInfo: ["volume": 0.8], scaling: "albumGain"), 0.8, accuracy: 1e-6)
	}

	func testPeakLimitingPreventsClipping() {
		let loud: [AnyHashable: Any] = ["replayGainAlbumGain": 6.0, "replayGainAlbumPeak": 0.9]
		XCTAssertEqual(ReplayGain.linearGain(rgInfo: loud, scaling: "albumGain"), db(6), accuracy: 1e-6)
		XCTAssertEqual(ReplayGain.linearGain(rgInfo: loud, scaling: "albumGainWithPeak"), 1 / 0.9, accuracy: 1e-6)
	}
}
