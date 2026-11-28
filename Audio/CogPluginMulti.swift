//
//  CogPluginMulti.swift
//  CogAudio
//
//  Created by Christopher Snowhill on 10/21/13.
//  Copyright 2013 __MyCompanyName__. All rights reserved.
//

import Foundation

private func priority(ofClassName className: String) -> Float {
	guard let cls = NSClassFromString(className) else { return 0 }
	if let d = cls as? CogDecoder.Type { return d.priority() }
	if let c = cls as? CogContainer.Type { return c.priority() }
	if let m = cls as? CogMetadataReader.Type { return m.priority() }
	if let p = cls as? CogPropertiesReader.Type { return p.priority() }
	return 0
}

// Kept as a C symbol: PluginController.mm declares it extern "C".
@_cdecl("sortClassesByPriority")
public func sortClassesByPriority(_ theClasses: NSArray) -> NSArray {
	return (theClasses as? [String] ?? []).sorted { a, b in
		let p1 = priority(ofClassName: a)
		let p2 = priority(ofClassName: b)
		if p1 == p2 { return false }
		return p1 > p2
	} as NSArray
}

@objc(CogDecoderMulti)
public class CogDecoderMulti: NSObject, CogDecoder {
	private static let observerContext = UnsafeMutableRawPointer.allocate(byteCount: 1, alignment: 1)

	private var theDecoders: [String]
	// @objc so the feeder/tests can look inside this wrapper with KVC, as they
	// could when it was an Objective-C ivar.
	@objc private var theDecoder: CogDecoder?
	private var observersAdded = false

	@objc public class func mimeTypes() -> [Any] { [] }
	@objc public class func fileTypes() -> [Any] { [] }
	@objc public class func priority() -> Float { -1.0 }
	@objc public class func fileTypeAssociations() -> [Any] { [] }

	@objc public init(decoders: [Any]) {
		theDecoders = sortClassesByPriority(decoders as NSArray) as? [String] ?? []
		theDecoder = nil
		super.init()
	}

	@objc public func properties() -> [AnyHashable: Any]? {
		theDecoder?.properties()
	}

	@objc public func metadata() -> [AnyHashable: Any]? {
		theDecoder?.metadata() ?? [:]
	}

	@objc public func readAudio() -> AudioChunk? {
		theDecoder?.readAudio()
	}

	@objc public func open(_ source: CogSource) -> Bool {
		for classString in theDecoders {
			guard let cls = NSClassFromString(classString) as? NSObject.Type, let decoder = cls.init() as? CogDecoder else { continue }
			theDecoder = decoder
			registerObservers()
			if decoder.open(source) {
				return true
			}
			removeObservers()
			// HTTP reader supports limited rewinding
			source.seek(0, whence: SEEK_SET)
		}
		theDecoder = nil
		return false
	}

	@objc public func seek(_ frame: Int) -> Int {
		theDecoder?.seek(frame) ?? -1
	}

	@objc public func close() {
		if let theDecoder {
			removeObservers()
			theDecoder.close()
			self.theDecoder = nil
		}
	}

	@objc public func interrupt() {
		theDecoder?.interrupt?()
	}

	deinit {
		close()
	}

	private func registerObservers() {
		if !observersAdded, let theDecoder = theDecoder as? NSObject {
			theDecoder.addObserver(self, forKeyPath: "properties", options: .new, context: Self.observerContext)
			theDecoder.addObserver(self, forKeyPath: "metadata", options: .new, context: Self.observerContext)
			observersAdded = true
		}
	}

	private func removeObservers() {
		if observersAdded, let theDecoder = theDecoder as? NSObject {
			observersAdded = false
			theDecoder.removeObserver(self, forKeyPath: "properties", context: Self.observerContext)
			theDecoder.removeObserver(self, forKeyPath: "metadata", context: Self.observerContext)
		}
	}

	@objc public func setTrack(_ track: URL) -> Bool {
		theDecoder?.setTrack?(track) ?? false
	}

	public override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		if context == Self.observerContext, let keyPath {
			willChangeValue(forKey: keyPath)
			didChangeValue(forKey: keyPath)
		} else {
			super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
		}
	}
}

@objc(CogContainerMulti)
public class CogContainerMulti: NSObject {
	@objc public class func urls(forContainerURL url: URL, containers: [Any]) -> [Any] {
		let sortedContainers = sortClassesByPriority(containers as NSArray) as? [String] ?? []
		for classString in sortedContainers {
			guard let container = NSClassFromString(classString) as? CogContainer.Type else { continue }
			let urls = container.urls(forContainerURL: url) ?? []
			if !urls.isEmpty {
				return urls
			}
		}
		return []
	}

	@objc public class func dependencyUrls(forContainerURL url: URL, containers: [Any]) -> [Any] {
		let sortedContainers = sortClassesByPriority(containers as NSArray) as? [String] ?? []
		for classString in sortedContainers {
			guard let container = (NSClassFromString(classString) as? NSObject.Type) else { continue }
			if container.responds(to: Selector(("dependencyUrlsForContainerURL:"))) {
				let urls = container.perform(Selector(("dependencyUrlsForContainerURL:")), with: url)?.takeUnretainedValue() as? [Any] ?? []
				if !urls.isEmpty {
					return urls
				}
			}
		}
		return []
	}
}

@objc(CogMetadataReaderMulti)
public class CogMetadataReaderMulti: NSObject {
	@objc public class func metadata(forURL url: URL, readers: [Any]) -> [AnyHashable: Any] {
		let sortedReaders = sortClassesByPriority(readers as NSArray) as? [String] ?? []
		for classString in sortedReaders {
			guard let reader = NSClassFromString(classString) as? CogMetadataReader.Type else { continue }
			let data = reader.metadata(for: url) ?? [:]
			if !data.isEmpty {
				return data
			}
		}
		return [:]
	}
}

@objc(CogPropertiesReaderMulti)
public class CogPropertiesReaderMulti: NSObject {
	@objc public class func properties(forSource source: CogSource, readers: [Any]) -> [AnyHashable: Any] {
		let sortedReaders = sortClassesByPriority(readers as NSArray) as? [String] ?? []
		for classString in sortedReaders {
			guard let reader = NSClassFromString(classString) as? CogPropertiesReader.Type else { continue }
			let data = reader.properties(for: source) ?? [:]
			if !data.isEmpty {
				return data
			}
			if source.seekable() {
				source.seek(0, whence: SEEK_SET)
			}
		}
		return [:]
	}
}
