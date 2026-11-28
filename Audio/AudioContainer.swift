//
//  AudioContainer.swift
//  CogAudio
//
//  Created by Zaphod Beeblebrox on 10/8/07.
//  Copyright 2007 Zaphod Beeblebrox. All rights reserved.
//

import Foundation

@objc(AudioContainer)
public class AudioContainer: NSObject {
	@objc(urlsForContainerURL:)
	public static func urls(forContainerURL url: URL) -> [Any]? {
		autoreleasepool {
			PluginController.shared().urls(forContainerURL: url)
		}
	}

	// With each track's properties and metadata, merged, where the container
	// reads them in the same pass.
	@objc(urlsForContainerURL:trackInfo:)
	public static func urls(forContainerURL url: URL, trackInfo info: AutoreleasingUnsafeMutablePointer<NSDictionary?>?) -> [Any]? {
		// Held outside the pool: what comes back through `info` is autoreleased,
		// and would go with it.
		var trackInfo: NSDictionary?
		let urls: [Any]? = autoreleasepool {
			if info != nil {
				return PluginController.shared().urls(forContainerURL: url, trackInfo: &trackInfo)
			}
			return PluginController.shared().urls(forContainerURL: url, trackInfo: nil)
		}
		info?.pointee = trackInfo
		return urls
	}

	@objc(dependencyUrlsForContainerURL:)
	public static func dependencyUrls(forContainerURL url: URL) -> [Any]? {
		autoreleasepool {
			PluginController.shared().dependencyUrls(forContainerURL: url)
		}
	}
}
