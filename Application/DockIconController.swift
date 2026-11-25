//
//  DockIconController.swift
//  Cog
//
//  Created by Vincent Spader on 2/28/09.
//  Copyright 2009 Vincent Spader. All rights reserved.
//

import AppKit
import CogAudio

@objc(DockIconController)
class DockIconController: NSObject {
	nonisolated(unsafe) private static let observationContext = UnsafeMutableRawPointer.allocate(byteCount: 1, alignment: 1)
	private static let customDockIconsReloadNotification = Notification.Name("CogCustomDockIconsReloadNotification")

	@IBOutlet var playbackController: PlaybackController?

	// The controller observed, for deinit to let go of; only touched on the
	// main thread.
	nonisolated(unsafe) private var observedPlaybackController: PlaybackController?

	private var dockImage: NSImage?

	private var lastDockCustom = false
	private var lastDockCustomPlaque = false
	private var dockCustomLoaded = false
	private var dockCustomStop: NSImage?
	private var dockCustomPlay: NSImage?
	private var dockCustomPause: NSImage?

	private var lastPlaybackStatus = 0
	private var lastColorfulStatus = -1
	private var lastProgressStatus: NSNumber = -1.0

	private var imageView: NSImageView?
	private var progressIndicator: NSProgressIndicator?

	override func awakeFromNib() {
		dockImage = NSImage(named: "Plaque")?.copy() as? NSImage
		lastColorfulStatus = -1
		lastProgressStatus = -1.0
		imageView = nil
		progressIndicator = nil
		startObserving()
	}

	deinit {
		observedPlaybackController?.removeObserver(self, forKeyPath: "playbackStatus", context: Self.observationContext)
		observedPlaybackController?.removeObserver(self, forKeyPath: "progressOverall", context: Self.observationContext)
		NSUserDefaultsController.shared.removeObserver(self, forKeyPath: "values.colorfulDockIcons", context: Self.observationContext)
		NSUserDefaultsController.shared.removeObserver(self, forKeyPath: "values.customDockIcons", context: Self.observationContext)
		NSUserDefaultsController.shared.removeObserver(self, forKeyPath: "values.customDockIconsPlaque", context: Self.observationContext)
		NotificationCenter.default.removeObserver(self, name: Self.customDockIconsReloadNotification, object: nil)
	}

	private func startObserving() {
		observedPlaybackController = playbackController
		playbackController?.addObserver(self, forKeyPath: "playbackStatus", options: [.new, .initial], context: Self.observationContext)
		playbackController?.addObserver(self, forKeyPath: "progressOverall", options: [.new, .initial, .old], context: Self.observationContext)
		NSUserDefaultsController.shared.addObserver(self, forKeyPath: "values.colorfulDockIcons", options: [], context: Self.observationContext)
		NSUserDefaultsController.shared.addObserver(self, forKeyPath: "values.customDockIcons", options: [], context: Self.observationContext)
		NSUserDefaultsController.shared.addObserver(self, forKeyPath: "values.customDockIconsPlaque", options: [], context: Self.observationContext)
		NotificationCenter.default.addObserver(self, selector: #selector(refreshDockIcons(_:)), name: Self.customDockIconsReloadNotification, object: nil)
	}

	private func startObservingProgress(_ progress: Progress) {
		progress.addObserver(self, forKeyPath: "fractionCompleted", options: [], context: Self.observationContext)
	}

	private func stopObservingProgress(_ progress: Progress) {
		progress.removeObserver(self, forKeyPath: "fractionCompleted", context: Self.observationContext)
	}

	private static func badgeName(_ baseName: String, colorfulIcons: Bool) -> String {
		if #available(macOS 26, *) {
			return baseName
		} else {
			return baseName + (colorfulIcons ? "Colorful" : "Normal")
		}
	}

	private static func customIconName(_ baseName: String) -> String {
		let basePath = NSSearchPathForDirectoriesInDomains(.applicationSupportDirectory, .userDomainMask, true).first ?? ""
		return (((basePath as NSString).appendingPathComponent("Cog") as NSString).appendingPathComponent("Icons") as NSString).appendingPathComponent(baseName) + ".png"
	}

	private func loadCustomDockIcons() -> Bool {
		guard let dataStopIcon = try? Data(contentsOf: URL(fileURLWithPath: Self.customIconName("Stop")), options: .mappedIfSafe),
		      let dataPlayIcon = try? Data(contentsOf: URL(fileURLWithPath: Self.customIconName("Play")), options: .mappedIfSafe),
		      let dataPauseIcon = try? Data(contentsOf: URL(fileURLWithPath: Self.customIconName("Pause")), options: .mappedIfSafe)
		else {
			return false
		}
		dockCustomStop = NSImage(data: dataStopIcon)
		dockCustomPlay = NSImage(data: dataPlayIcon)
		dockCustomPause = NSImage(data: dataPauseIcon)
		return dockCustomStop != nil && dockCustomPlay != nil && dockCustomPause != nil
	}

	private func refreshDockIcon(_ playbackStatus: Int, withProgress progressStatus: Double) {
		var playbackStatus = playbackStatus
		var progressStatus = progressStatus

		// Really weird crash user experienced because the plaque image didn't load?
		guard let dockImage, dockImage.size.width != 0, dockImage.size.height != 0 else { return }

		var displayChanged = false
		var drawIcon = false
		var removeProgress = false

		var useCustomDockIcons = UserDefaults.standard.bool(forKey: "customDockIcons")
		let useCustomDockIconsPlaque = UserDefaults.standard.bool(forKey: "customDockIconsPlaque")

		if useCustomDockIcons && !dockCustomLoaded {
			dockCustomLoaded = loadCustomDockIcons()
			if !dockCustomLoaded {
				useCustomDockIcons = false
			}
		}

		if useCustomDockIcons != lastDockCustom ||
			useCustomDockIconsPlaque != lastDockCustomPlaque {
			lastDockCustom = useCustomDockIcons
			lastDockCustomPlaque = useCustomDockIconsPlaque
			drawIcon = true

			if !useCustomDockIcons {
				dockCustomLoaded = false
				dockCustomStop = nil
				dockCustomPlay = nil
				dockCustomPause = nil
			}
		}

		if playbackStatus < 0 {
			playbackStatus = lastPlaybackStatus
		} else {
			lastPlaybackStatus = playbackStatus
			drawIcon = true
		}

		if progressStatus < -2 {
			progressStatus = lastProgressStatus.doubleValue
		} else {
			if progressStatus < 0 && lastProgressStatus.doubleValue >= 0 {
				removeProgress = true
			}
			lastProgressStatus = NSNumber(value: progressStatus)
		}

		let displayProgress = progressStatus >= 0.0

		var badgeImage: NSImage?

		var colorfulIcons = UserDefaults.standard.bool(forKey: "colorfulDockIcons")
		var glassIcons = false
		if #available(macOS 26, *) {
			colorfulIcons = false
			glassIcons = !useCustomDockIcons
			drawIcon = glassIcons
		}

		if (colorfulIcons && lastColorfulStatus < 1) ||
			(!colorfulIcons && lastColorfulStatus != 0) {
			lastColorfulStatus = colorfulIcons ? 1 : 0
			drawIcon = true
		}

		let dockTile = NSApp.dockTile

		if drawIcon {
			var badgeLabel: String?
			if !glassIcons {
				switch playbackStatus {
				case CogStatus.playing.rawValue:
					badgeImage = useCustomDockIcons ? dockCustomPlay : NSImage(named: Self.badgeName("Play", colorfulIcons: colorfulIcons))
				case CogStatus.paused.rawValue:
					badgeImage = useCustomDockIcons ? dockCustomPause : NSImage(named: Self.badgeName("Pause", colorfulIcons: colorfulIcons))
				default:
					badgeImage = useCustomDockIcons ? dockCustomStop : NSImage(named: Self.badgeName("Stop", colorfulIcons: colorfulIcons))
				}
			} else {
				if playbackController?.isError() == true {
					badgeLabel = "?"
				} else {
					switch playbackStatus {
					case CogStatus.playing.rawValue:
						badgeLabel = "\u{23f5}"
					case CogStatus.paused.rawValue:
						badgeLabel = "\u{23f8}"
					default:
						badgeLabel = nil
					}
				}
			}

			if !glassIcons {
				let badgeSize = badgeImage?.size ?? .zero

				let newDockImage = (glassIcons || (useCustomDockIcons && !useCustomDockIconsPlaque)) ? NSImage(size: NSSize(width: 1024, height: 1024)) : (dockImage.copy() as? NSImage ?? dockImage)
				newDockImage.lockFocus()

				badgeImage?.draw(in: NSRect(x: 0, y: 0, width: 1024, height: 1024),
				                 from: NSRect(x: 0, y: 0, width: badgeSize.width, height: badgeSize.height),
				                 operation: .sourceOver,
				                 fraction: 1.0)

				newDockImage.unlockFocus()

				let view = NSImageView()
				view.image = newDockImage
				imageView = view
				dockTile.contentView = view
			} else {
				if #available(macOS 26, *) {
					dockTile.badgeLabel = badgeLabel
				}
			}

			let indicator = NSProgressIndicator(frame: NSRect(x: 0, y: 0, width: dockTile.size.width, height: 10))
			indicator.style = .bar
			indicator.isIndeterminate = false
			indicator.isBezeled = true
			indicator.minValue = 0
			indicator.maxValue = 100
			indicator.isHidden = true
			progressIndicator = indicator

			imageView?.addSubview(indicator)

			displayChanged = true
		}

		if displayProgress {
			if imageView == nil && !glassIcons {
				let view = NSImageView()
				view.image = NSApp.applicationIconImage
				imageView = view
				dockTile.contentView = view
			}

			if progressIndicator == nil {
				let indicator = NSProgressIndicator(frame: NSRect(x: 0, y: 0, width: dockTile.size.width, height: 10))
				indicator.isIndeterminate = false
				indicator.isBezeled = true
				indicator.minValue = 0
				indicator.maxValue = 100
				progressIndicator = indicator

				imageView?.addSubview(indicator)
			}

			progressIndicator?.doubleValue = progressStatus
			progressIndicator?.isHidden = false

			displayChanged = true
		}

		if removeProgress {
			progressIndicator?.isHidden = true

			displayChanged = true
		}

		if displayChanged {
			dockTile.display()
		}
	}

	override func observeValue(forKeyPath keyPath: String?, of object: Any?, change: [NSKeyValueChangeKey: Any]?, context: UnsafeMutableRawPointer?) {
		if context == Self.observationContext {
			if keyPath == "playbackStatus" {
				let playbackStatus = (change?[.newKey] as? NSNumber)?.intValue ?? 0

				refreshDockIcon(playbackStatus, withProgress: -10)
			} else if keyPath == "progressOverall" {
				var progressStatus = lastProgressStatus.doubleValue

				let progressNew = change?[.newKey] as? Progress
				let progressOld = change?[.oldKey] as? Progress

				if let progressOld {
					stopObservingProgress(progressOld)
					progressStatus = -1
				}

				if let progressNew {
					startObservingProgress(progressNew)
					progressStatus = progressNew.fractionCompleted * 100.0
				}

				refreshDockIcon(-1, withProgress: progressStatus)
			} else if keyPath == "values.colorfulDockIcons" ||
				keyPath == "values.customDockIcons" ||
				keyPath == "values.customDockIconsPlaque" {
				refreshDockIcon(-1, withProgress: -10)
			} else if keyPath == "fractionCompleted" {
				let progressStatus = (object as? Progress)?.fractionCompleted ?? 0
				refreshDockIcon(-1, withProgress: progressStatus * 100.0)
			}
		} else {
			super.observeValue(forKeyPath: keyPath, of: object, change: change, context: context)
		}
	}

	@objc private func refreshDockIcons(_ notification: Notification) {
		lastDockCustom = false
		dockCustomLoaded = false
		refreshDockIcon(-1, withProgress: -10)
	}
}