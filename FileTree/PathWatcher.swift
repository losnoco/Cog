//
//  PathWatcher.swift
//  Cog
//
//  Created by Vincent Spader on 2/17/08.
//  Copyright 2008 Vincent Spader. All rights reserved.
//

import AppKit
import CoreServices

@objc protocol PathWatcherDelegate {
	func pathDidChange(_ path: String, flags: FSEventStreamEventFlags)
}

private func fsEventCallback(
	_ streamRef: ConstFSEventStreamRef,
	_ clientCallBackInfo: UnsafeMutableRawPointer?,
	_ numEvents: Int,
	_ eventPaths: UnsafeMutableRawPointer,
	_ eventFlags: UnsafePointer<FSEventStreamEventFlags>,
	_ eventIds: UnsafePointer<FSEventStreamEventId>) {
	let pathWatcher = Unmanaged<PathWatcher>.fromOpaque(clientCallBackInfo!).takeUnretainedValue()

	fputs("Callback called\n", stdout)
	let paths = eventPaths.assumingMemoryBound(to: UnsafeMutablePointer<CChar>.self)
	for i in 0..<numEvents {
		let pathString = String(cString: paths[i])
		pathWatcher.delegate?.pathDidChange(pathString, flags: eventFlags[i])
	}
}

@objc(PathWatcher)
class PathWatcher: NSObject {
	private var stream: FSEventStreamRef?
	private var context: UnsafeMutablePointer<FSEventStreamContext>?

	@IBOutlet var delegate: (any PathWatcherDelegate)?

	@objc func cleanUp() {
		if let stream {
			FSEventStreamStop(stream)
			FSEventStreamInvalidate(stream)
			FSEventStreamRelease(stream)
			self.stream = nil
		}
		if let context {
			context.deallocate()
			self.context = nil
		}
	}

	// Set the path to watch
	@objc func setPath(_ path: String?) {
		cleanUp()

		guard let path else { return }

		// Create FSEvent stream
		let pathsToWatch = [path]

		let ctx = UnsafeMutablePointer<FSEventStreamContext>.allocate(capacity: 1)
		ctx.pointee = FSEventStreamContext(version: 0, info: Unmanaged.passUnretained(self).toOpaque(), retain: nil, release: nil, copyDescription: nil)
		context = ctx

		// Create the stream, passing in a callback
		stream = FSEventStreamCreate(nil,
		                             fsEventCallback,
		                             ctx,
		                             pathsToWatch as CFArray,
		                             FSEventStreamEventId(kFSEventStreamEventIdSinceNow), // Or a previous event ID
		                             1.0, // latency in seconds
		                             FSEventStreamCreateFlags(kFSEventStreamCreateFlagFileEvents)) // Watch this and all its subdirectories

		if let stream {
			FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), CFRunLoopMode.defaultMode.rawValue)
			FSEventStreamStart(stream)
		}
	}

	deinit {
		cleanUp()
	}
}
