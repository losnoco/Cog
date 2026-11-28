//
//  ContainedNode.swift
//  Cog
//
//  Created by Vincent Spader on 10/15/07.
//  Copyright 2007 Vincent Spader. All rights reserved.
//

import CogAudio
import Foundation

@objc(ContainedNode)
class ContainedNode: PathNode {
	override func isLeaf() -> Bool {
		true
	}

	override var URL: URL? {
		didSet {
			guard let u = URL, let fragment = u.fragment else { return }

			let metadata = AudioMetadataReader.metadata(for: u)
			var title: String?
			var artist: String?
			if let metadata {
				title = Self.stringValue(metadata["title"])
				artist = Self.stringValue(metadata["artist"])
			}

			if let title, !title.isEmpty {
				if let artist, !artist.isEmpty {
					display = fragment + ": \(artist) - \(title)"
				} else {
					display = fragment + ": \(title)"
				}
			} else {
				display = fragment
			}
		}
	}

	private static func stringValue(_ value: Any?) -> String? {
		if let array = value as? [Any] {
			return array.map { "\($0)" }.joined(separator: ", ")
		}
		if let string = value as? String {
			return string
		}
		if let number = value as? NSNumber {
			return "\(number)"
		}
		return nil
	}
}
