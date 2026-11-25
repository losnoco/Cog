//
//  CueSheetTrack.swift
//  CueSheet
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

import Foundation

@objc(CueSheetTrack)
class CueSheetTrack: NSObject {
	let track: String
	let url: URL?

	let artist: String?
	let album: String?
	let title: String?
	let genre: String?
	let year: String?

	let albumGain: Float
	let albumPeak: Float
	let trackGain: Float
	let trackPeak: Float

	let time: Double // Starting time for the track
	let timeInSamples: Bool

	init(url u: URL?, track t: String, time s: Double, timeInSamples tis: Bool, artist a: String?, album b: String?, title l: String?, genre g: String?, year y: String?, albumGain: Float, albumPeak: Float, trackGain: Float, trackPeak: Float) {
		track = t
		url = u
		artist = a
		album = b
		title = l
		genre = g
		year = y
		time = s
		timeInSamples = tis
		self.albumGain = albumGain
		self.albumPeak = albumPeak
		self.trackGain = trackGain
		self.trackPeak = trackPeak
		super.init()
	}

	override var description: String {
		return "CueSheetTrack{track: \(track),  url: \(url?.absoluteURL.description ?? "(null)"), artist: \(artist ?? "(null)"), album: \(album ?? "(null)"), title: \(title ?? "(null)"), genre: \(genre ?? "(null)"), year: \(year ?? "(null)")}"
	}
}