//
//  AudioDecoder.swift
//  CogAudio
//
//  Created by Vincent Spader on 2/21/07.
//  Copyright 2007 Vincent Spader. All rights reserved.
//

import Foundation

@objc(AudioDecoder)
public class AudioDecoder: NSObject {
	@objc(audioDecoderForSource:)
	public static func audioDecoder(for source: any CogSource) -> (any CogDecoder)? {
		PluginController.shared().audioDecoder(for: source, skipCue: false)
	}

	@objc(audioDecoderForSource:skipCue:)
	public static func audioDecoder(for source: any CogSource, skipCue skip: Bool) -> (any CogDecoder)? {
		PluginController.shared().audioDecoder(for: source, skipCue: skip)
	}
}
