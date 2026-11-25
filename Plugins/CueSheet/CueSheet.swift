//
//  CueSheet.swift
//  CueSheet
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

import CogAudio
import Foundation

@objc(CueSheet)
class CueSheet: NSObject {
	private(set) var tracks: [CueSheetTrack] = []

	init(file filename: String) {
		super.init()
		parseFile(filename)
	}

	init(string cuesheet: String, withFilename filename: String) {
		super.init()
		parseString(cuesheet, withFilename: filename)
	}

	// The cuesheet tag, whether a reader gives it as a string or as an array
	// of them, as the FLAC and Vorbis readers do.
	static func sheetString(in metadata: [AnyHashable: Any]?) -> String? {
		guard let sheet = metadata?["cuesheet"] else { return nil }
		if let sheetContainer = sheet as? [Any] {
			return sheetContainer.first as? String
		}
		return sheet as? String
	}

	private func urlForPath(_ path: String, relativeTo baseFilename: String) -> URL? {
		if path.range(of: "://") != nil {
			return URL(string: path)
		}

		var unixPath = path

		// Get the fragment
		var fragment = ""
		let scanner = Scanner(string: path)
		let characterSet = CharacterSet(charactersIn: "#1234567890")
		while !scanner.isAtEnd {
			_ = scanner.scanUpToString("#")

			if let possibleFragment = scanner.scanCharacters(from: characterSet), scanner.isAtEnd {
				fragment = possibleFragment
				unixPath.removeLast(possibleFragment.count)
				break
			}
		}

		if !unixPath.hasPrefix("/") {
			// Only relative paths would have windows backslashes.
			unixPath = unixPath.replacingOccurrences(of: "\\", with: "/")

			let basePath = ((baseFilename as NSString).standardizingPath as NSString).deletingLastPathComponent + "/"

			unixPath = basePath + unixPath
		}

		// Append the fragment
		return URL(string: URL(fileURLWithPath: unixPath).absoluteString + fragment)
	}

	private func parseFile(_ filename: String) {
		let broker = (NSClassFromString("SandboxBroker") as? CogSandboxBroker.Type)?.shared()
		let sbHandle = broker?.beginFolderAccess(URL(fileURLWithPath: filename))
		var data: Data?
		var readError: Error?
		if let file = FileHandle(forReadingAtPath: filename) {
			do {
				data = try file.readToEnd()
			} catch {
				readError = error
			}
		} else {
			readError = NSError(domain: NSPOSIXErrorDomain, code: Int(ENOENT))
		}
		broker?.endFolderAccess(sbHandle)

		var contents: String?
		if var data {
			data.append(0)
			contents = data.withUnsafeBytes {
				guess_encoding_of_string($0.bindMemory(to: CChar.self).baseAddress!)
			}
		}

		if readError != nil || contents == nil {
			NSLog("Could not open file...\(filename) \(contents ?? "(null)") \(readError.map { "\($0)" } ?? "(null)")")
			return
		}

		parseString(contents!, withFilename: filename)
	}

	private func parseString(_ contents: String, withFilename filename: String) {
		var entries: [CueSheetTrack] = []

		var track: String?
		var path: String?
		var artist: String?
		var album: String?
		var title: String?
		var genre: String?
		var year: String?

		var albumGain: Float = 0.0
		var albumPeak: Float = 0.0
		var trackGain: Float = 0.0
		var trackPeak: Float = 0.0

		var trackAdded = false

		let whitespace = CharacterSet.whitespacesAndNewlines

		for line in contents.components(separatedBy: "\n") {
			let scanner = Scanner(string: line)

			guard let command = scanner.scanUpToCharacters(from: whitespace) else {
				continue
			}

			// FILE "filename.shn" WAVE
			if command == "FILE" {
				trackAdded = false

				guard scanner.scanString("\"") != nil else {
					continue
				}

				// Read in the path
				guard let filePath = scanner.scanUpToString("\"") else {
					continue
				}
				path = filePath
			}
			// TRACK 01 AUDIO
			else if command == "TRACK" {
				trackAdded = false

				guard let trackNumber = scanner.scanUpToCharacters(from: whitespace) else {
					continue
				}
				track = trackNumber

				guard let type = scanner.scanUpToCharacters(from: whitespace), type == "AUDIO" else {
					continue
				}
			}
			// INDEX 01 00:00:10
			// Note that time is written in Minutes:Seconds:Frames, where frames are 1/75 of a second
			else if command == "INDEX" {
				if trackAdded {
					continue
				}

				guard let path else {
					continue
				}

				guard let index = scanner.scanUpToCharacters(from: whitespace), (index as NSString).intValue == 1 else {
					continue
				}

				_ = scanner.scanCharacters(from: whitespace)

				guard let time = scanner.scanUpToCharacters(from: whitespace) else {
					continue
				}

				let msf = time.components(separatedBy: ":")
				let count = msf.count
				if count != 1 && count != 3 {
					continue
				}

				var seconds: Double
				var timeInSamples = false

				if count == 1 {
					seconds = Double((msf[0] as NSString).floatValue)
					timeInSamples = true
				} else {
					seconds = Double(60 * (msf[0] as NSString).intValue) + Double((msf[1] as NSString).intValue) + Double((msf[2] as NSString).floatValue) / 75
				}

				if track == nil {
					track = "01"
				}

				// Need to add basePath, and convert to URL
				entries.append(CueSheetTrack(url: urlForPath(path, relativeTo: filename),
				                             track: track!,
				                             time: seconds,
				                             timeInSamples: timeInSamples,
				                             artist: artist,
				                             album: album,
				                             title: title,
				                             genre: genre,
				                             year: year,
				                             albumGain: albumGain,
				                             albumPeak: albumPeak,
				                             trackGain: trackGain,
				                             trackPeak: trackPeak))
				trackAdded = true
			} else if command == "PERFORMER" {
				guard scanner.scanString("\"") != nil else {
					continue
				}

				// Read in the artist
				guard let performer = scanner.scanUpToString("\"") else {
					continue
				}
				artist = performer
			} else if command == "TITLE" {
				guard scanner.scanString("\"") != nil else {
					continue
				}

				// Read in the title
				guard let titleDest = scanner.scanUpToString("\"") else {
					continue
				}

				if path == nil { // Have not come across a file yet.
					album = titleDest
				} else {
					title = titleDest
				}
			} else if command == "REM" { // Additional metadata sometimes stored in comments
				guard let type = scanner.scanUpToCharacters(from: whitespace) else {
					continue
				}

				if type == "GENRE" {
					if scanner.scanString("\"") != nil {
						guard let quotedGenre = scanner.scanUpToString("\"") else {
							#if DEBUG
								NSLog("FAILED TO SCAN")
							#endif
							continue
						}
						genre = quotedGenre
					} else {
						guard let unquotedGenre = scanner.scanUpToCharacters(from: whitespace) else {
							continue
						}
						genre = unquotedGenre
					}
				} else if type == "DATE" {
					guard let date = scanner.scanUpToCharacters(from: whitespace) else {
						continue
					}
					year = date
				} else if type.hasPrefix("REPLAYGAIN_") {
					guard let rgTag = scanner.scanUpToCharacters(from: whitespace) else {
						continue
					}
					if type.hasPrefix("REPLAYGAIN_ALBUM_") {
						if type.hasSuffix("GAIN") {
							albumGain = (rgTag as NSString).floatValue
						} else if type.hasSuffix("PEAK") {
							albumPeak = (rgTag as NSString).floatValue
						}
					} else if type.hasPrefix("REPLAYGAIN_TRACK_") {
						if type.hasSuffix("GAIN") {
							trackGain = (rgTag as NSString).floatValue
						} else if type.hasSuffix("PEAK") {
							trackPeak = (rgTag as NSString).floatValue
						}
					}
				}
			}
		}

		tracks = entries
	}

	func track(_ fragment: String) -> CueSheetTrack? {
		for t in tracks {
			if t.track == fragment {
				return t
			}
		}

		return nil
	}
}