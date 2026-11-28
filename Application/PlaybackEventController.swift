//
//  PlaybackEventController.swift
//  Cog
//
//  Created by Vincent Spader on 3/5/09.
//  Copyright 2009 Vincent Spader. All rights reserved.
//

import AppKit
import CogPlaylist
import UserNotifications

// The playback actions run on a serial queue off the main thread, and the
// notification-center callbacks arrive on its own queues, as in the ObjC
// original; the mutable state is nonisolated(unsafe) to match.
@objc(PlaybackEventController)
final class PlaybackEventController: NSObject, UNUserNotificationCenterDelegate, @unchecked Sendable {
	@IBOutlet nonisolated(unsafe) var playbackController: PlaybackController?

	@IBOutlet nonisolated(unsafe) var mainWindow: NSWindow?
	@IBOutlet nonisolated(unsafe) var miniWindow: NSWindow?
	@IBOutlet nonisolated(unsafe) var miniPlusWindow: NSWindow?

	private let queue: OperationQueue

	nonisolated(unsafe) private var entry: PlaylistEntry?

	nonisolated(unsafe) private var didGainUN = false

	private func initDefaults() {
		let defaultsDictionary: [String: Any] = [
			"enableAudioScrobbler": true,
			"enableListenBrainz": false,
			"listenBrainzUrl": "https://api.listenbrainz.org",
			"notifications.enable": true,
			"notifications.itunes-style": true,
			"notifications.show-album-art": true
		]

		UserDefaults.standard.register(defaults: defaultsDictionary)
	}

	override init() {
		queue = OperationQueue()
		super.init()

		initDefaults()

		let center = UNUserNotificationCenter.current()
		center.requestAuthorization(options: .alert) { [self] granted, _ in
			didGainUN = granted

			if granted {
				let skipAction = UNNotificationAction(
					identifier: "skip",
					title: NSLocalizedString("SkipAction", comment: ""),
					options: [])

				let playCategory = UNNotificationCategory(
					identifier: "play",
					actions: [skipAction],
					intentIdentifiers: [],
					options: [])

				UNUserNotificationCenter.current().setNotificationCategories([playCategory])
			}
		}

		center.delegate = self

		queue.maxConcurrentOperationCount = 1
	}

	deinit {
		NotificationCenter.default.removeObserver(self)
	}

	func userNotificationCenter(_ center: UNUserNotificationCenter,
	                            willPresent notification: UNNotification,
	                            withCompletionHandler completionHandler: @escaping (UNNotificationPresentationOptions) -> Void) {
		completionHandler([.banner, .list])
	}

	func userNotificationCenter(_ center: UNUserNotificationCenter,
	                            didReceive response: UNNotificationResponse,
	                            withCompletionHandler completionHandler: @escaping () -> Void) {
		if response.actionIdentifier == "skip" {
			playbackController?.next(self)
		} else if response.actionIdentifier == UNNotificationDefaultActionIdentifier {
			DispatchQueue.main.async { [self] in
				showPlayerWindow()
			}
		}
		completionHandler()
	}

	// Clicking a notification brings forward whichever player window is in use.
	private func showPlayerWindow() {
		let defaults = UserDefaults.standard
		let window: NSWindow?
		if defaults.bool(forKey: "miniPlusMode") {
			window = miniPlusWindow
		} else if defaults.bool(forKey: "miniMode") {
			window = miniWindow
		} else {
			window = mainWindow
		}

		NSApp.activate(ignoringOtherApps: true)
		window?.makeKeyAndOrderFront(self)
	}

	private func performPlaybackDidBeginActions(_ pe: PlaylistEntry) {
		guard !pe.error else { return }

		entry = pe

		let defaults = UserDefaults.standard

		guard defaults.bool(forKey: "notifications.enable"), didGainUN else { return }

		let center = UNUserNotificationCenter.current()

		let content = UNMutableNotificationContent()
		content.title = "Now Playing"

		let artist = pe.artist.flatMap { $0.isEmpty ? nil : $0 }
		let album = pe.album.flatMap { $0.isEmpty ? nil : $0 }
		let subtitle: String
		if let artist, let album {
			subtitle = "\(artist) - \(album)"
		} else if let artist {
			subtitle = artist
		} else if let album {
			subtitle = album
		} else {
			subtitle = ""
		}

		content.body = "\(pe.title)\n\(subtitle)"
		content.sound = nil
		content.categoryIdentifier = "play"

		if defaults.bool(forKey: "notifications.show-album-art"), let image = pe.albumArt {
			let tmpSubFolderURL = URL(fileURLWithPath: NSTemporaryDirectory())
				.appendingPathComponent("cog-artworks-cache", isDirectory: true)
			// A directory that cannot be created skips the attachment, as before.
			if (try? FileManager.default.createDirectory(at: tmpSubFolderURL, withIntermediateDirectories: true)) != nil {
				let tmpFileName = ProcessInfo.processInfo.globallyUniqueString + ".jpg"
				let fileURL = tmpSubFolderURL.appendingPathComponent(tmpFileName)
				if let cgRef = image.cgImage(forProposedRect: nil, context: nil, hints: nil) {
					let newRep = NSBitmapImageRep(cgImage: cgRef)
					let jpgData = newRep.representation(using: .jpeg, properties: [.compressionFactor: 0.5])
					try? jpgData?.write(to: fileURL, options: .atomic)

					do {
						let icon = try UNNotificationAttachment(identifier: "art", url: fileURL, options: nil)
						content.attachments = [icon]
					} catch {
						// We have size limit of 10MB per image attachment.
						NSLog("%@", error.localizedDescription)
					}
				}
			}
		}

		let request = UNNotificationRequest(identifier: "PlayTrack", content: content, trigger: nil)

		center.add(request) { error in
			NSLog("%@", error?.localizedDescription ?? "(null)")
		}
	}

	private func performPlaybackDidPauseActions() {
	}

	private func performPlaybackDidResumeActions() {
	}

	private func performPlaybackDidStopActions() {
		entry = nil
	}

	override func awakeFromNib() {
		let center = NotificationCenter.default
		center.addObserver(self, selector: #selector(playbackDidBegin(_:)), name: .CogPlaybackDidBeginNotificiation, object: nil)
		center.addObserver(self, selector: #selector(playbackDidPause(_:)), name: .CogPlaybackDidPauseNotificiation, object: nil)
		center.addObserver(self, selector: #selector(playbackDidResume(_:)), name: .CogPlaybackDidResumeNotificiation, object: nil)
		center.addObserver(self, selector: #selector(playbackDidStop(_:)), name: .CogPlaybackDidStopNotificiation, object: nil)
	}

	@objc private func playbackDidBegin(_ notification: Notification) {
		queue.addOperation { [self] in
			if let pe = notification.object as? PlaylistEntry {
				performPlaybackDidBeginActions(pe)
			}
		}
	}

	@objc private func playbackDidPause(_ notification: Notification) {
		queue.addOperation { [self] in
			performPlaybackDidPauseActions()
		}
	}

	@objc private func playbackDidResume(_ notification: Notification) {
		queue.addOperation { [self] in
			performPlaybackDidResumeActions()
		}
	}

	@objc private func playbackDidStop(_ notification: Notification) {
		queue.addOperation { [self] in
			performPlaybackDidStopActions()
		}
	}
}
