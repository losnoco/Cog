//
//  ReplayGain.swift
//  CogAudio
//
//  Created by Christopher Snowhill on 9/29/26.
//

import Foundation

/// Cog's volume scaling, as `ConverterNode -refreshVolumeScaling` computes it.
///
/// The `volumeScaling` setting names the most specific source to use; each
/// less specific source is tried first and overridden by the next, so a
/// track without album gain still gets its track gain, and so on. A
/// `WithPeak` suffix then limits the gain so the track's peak cannot clip.
public enum ReplayGain {
	/// Where a track's gain came from, as the output status names it.
	public enum Source: String {
		case volume
		case soundcheck
		case track
		case album
	}

	/// A gain and how it was arrived at.
	public struct Resolution: Equatable {
		public var linear: Float = 1
		/// The source used, nil if none applied.
		public var source: Source?
		/// The gain was lowered so the peak cannot clip.
		public var peakLimited = false
	}

	public static func linearGain(rgInfo: [AnyHashable: Any]?, scaling: String? = UserDefaults.standard.string(forKey: "volumeScaling")) -> Float {
		resolve(rgInfo: rgInfo, scaling: scaling).linear
	}

	public static func resolve(rgInfo: [AnyHashable: Any]?, scaling: String? = UserDefaults.standard.string(forKey: "volumeScaling")) -> Resolution {
		guard let rgInfo else { return Resolution() }
		let scaling = scaling ?? ""
		let useAlbum = scaling.hasPrefix("albumGain")
		let useTrack = useAlbum || scaling.hasPrefix("trackGain")
		let useSoundcheck = useAlbum || useTrack || scaling == "soundcheck"
		let useVolume = useAlbum || useTrack || useSoundcheck || scaling == "volumeScale"
		let usePeak = scaling.hasSuffix("WithPeak")

		func float(_ key: String) -> Float? {
			(rgInfo[key] as? NSNumber)?.floatValue
		}
		func fromDecibels(_ db: Float) -> Float {
			powf(10, db / 20)
		}

		var resolution = Resolution()
		var peak: Float = 0
		if useVolume, let volume = float("volume") {
			resolution.linear = volume
			resolution.source = .volume
		}
		if useSoundcheck, let soundcheck = float("soundcheck") {
			resolution.linear = soundcheck
			resolution.source = .soundcheck
		}
		if useTrack {
			if let gain = float("replayGainTrackGain") {
				resolution.linear = fromDecibels(gain)
				resolution.source = .track
			}
			if let trackPeak = float("replayGainTrackPeak") { peak = trackPeak }
		}
		if useAlbum {
			if let gain = float("replayGainAlbumGain") {
				resolution.linear = fromDecibels(gain)
				resolution.source = .album
			}
			if let albumPeak = float("replayGainAlbumPeak") { peak = albumPeak }
		}
		if usePeak && resolution.linear * peak > 1 {
			resolution.linear = 1 / peak
			resolution.peakLimited = true
		}
		return resolution
	}
}

public extension EngineTrack {
	/// A track whose gain comes from its ReplayGain info and the user's
	/// volume scaling setting.
	@objc convenience init(url: URL, userInfo: Any?, rgInfo: [AnyHashable: Any]?) {
		self.init(url: url, userInfo: userInfo)
		update(rgInfo: rgInfo)
	}
}
