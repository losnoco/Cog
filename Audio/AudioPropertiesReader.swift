//
//  AudioPropertiesReader.swift
//  CogAudio
//
//  Created by Vincent Spader on 2/24/07.
//  Copyright 2007 Vincent Spader. All rights reserved.
//

import Foundation

@objc(AudioPropertiesReader)
public class AudioPropertiesReader: NSObject {
	@objc(propertiesForURL:)
	public static func properties(for url: URL) -> [AnyHashable: Any]? {
		autoreleasepool {
			PluginController.shared().properties(for: url, skipCue: false)
		}
	}

	@objc(propertiesForURL:skipCue:)
	public static func properties(for url: URL, skipCue skip: Bool) -> [AnyHashable: Any]? {
		autoreleasepool {
			PluginController.shared().properties(for: url, skipCue: skip)
		}
	}
}
