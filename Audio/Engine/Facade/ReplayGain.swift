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
	public static func linearGain(rgInfo: [AnyHashable: Any]?, scaling: String? = UserDefaults.standard.string(forKey: "volumeScaling")) -> Float {
		guard let rgInfo else { return 1 }
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

		var scale: Float = 1
		var peak: Float = 0
		if useVolume, let volume = float("volume") {
			scale = volume
		}
		if useSoundcheck, let soundcheck = float("soundcheck") {
			scale = soundcheck
		}
		if useTrack {
			if let gain = float("replayGainTrackGain") { scale = fromDecibels(gain) }
			if let trackPeak = float("replayGainTrackPeak") { peak = trackPeak }
		}
		if useAlbum {
			if let gain = float("replayGainAlbumGain") { scale = fromDecibels(gain) }
			if let albumPeak = float("replayGainAlbumPeak") { peak = albumPeak }
		}
		if usePeak && scale * peak > 1 {
			scale = 1 / peak
		}
		return scale
	}
}

public extension EngineTrack {
	/// A track whose gain comes from its ReplayGain info and the user's
	/// volume scaling setting.
	@objc convenience init(url: URL, userInfo: Any?, rgInfo: [AnyHashable: Any]?) {
		self.init(url: url, userInfo: userInfo, gain: ReplayGain.linearGain(rgInfo: rgInfo))
	}
}
