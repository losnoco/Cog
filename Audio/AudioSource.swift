//
//  AudioSource.swift
//  CogAudio
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 Vincent Spader. All rights reserved.
//

import Foundation

@objc(AudioSource)
public class AudioSource: NSObject {
	@objc(audioSourceForURL:)
	public static func audioSource(for url: URL) -> (any CogSource)? {
		PluginController.shared().audioSource(for: url)
	}
}
