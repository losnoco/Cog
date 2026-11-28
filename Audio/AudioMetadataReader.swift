//
//  AudioMetadataReader.swift
//  CogAudio
//
//  Created by Vincent Spader on 2/24/07.
//  Copyright 2007 Vincent Spader. All rights reserved.
//

import Foundation

@objc(AudioMetadataReader)
public class AudioMetadataReader: NSObject {
	@objc(metadataForURL:)
	public static func metadata(for url: URL) -> [AnyHashable: Any]? {
		autoreleasepool {
			PluginController.shared().metadata(for: url, skipCue: false)
		}
	}

	@objc(metadataForURL:skipCue:)
	public static func metadata(for url: URL, skipCue skip: Bool) -> [AnyHashable: Any]? {
		autoreleasepool {
			PluginController.shared().metadata(for: url, skipCue: skip)
		}
	}
}
