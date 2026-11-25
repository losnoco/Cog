//
//  MediaKeysApplication.swift
//  Cog
//
//  Created by Vincent Spader on 10/3/07.
//  Copyright 2007 Vincent Spader. All rights reserved.
//

import AppKit
import MediaPlayer

@objc(MediaKeysApplication)
class MediaKeysApplication: NSApplication {
	private var appController: AppController?

	override func finishLaunching() {
		super.finishLaunching()
		appController = delegate as? AppController

		let remoteCommandCenter = MPRemoteCommandCenter.shared()

		remoteCommandCenter.playCommand.isEnabled = true
		remoteCommandCenter.pauseCommand.isEnabled = true
		remoteCommandCenter.togglePlayPauseCommand.isEnabled = true
		remoteCommandCenter.stopCommand.isEnabled = true
		remoteCommandCenter.changePlaybackPositionCommand.isEnabled = true
		remoteCommandCenter.nextTrackCommand.isEnabled = true
		remoteCommandCenter.previousTrackCommand.isEnabled = true

		remoteCommandCenter.playCommand.addTarget(self, action: #selector(clickPlay))
		remoteCommandCenter.pauseCommand.addTarget(self, action: #selector(clickPause))
		remoteCommandCenter.togglePlayPauseCommand.addTarget(self, action: #selector(clickPlay))
		remoteCommandCenter.stopCommand.addTarget(self, action: #selector(clickStop))
		remoteCommandCenter.changePlaybackPositionCommand.addTarget(self, action: #selector(clickSeek(_:)))
		remoteCommandCenter.nextTrackCommand.addTarget(self, action: #selector(clickNext))
		remoteCommandCenter.previousTrackCommand.addTarget(self, action: #selector(clickPrev))
	}

	@objc private func clickPlay() -> MPRemoteCommandHandlerStatus {
		DispatchQueue.main.async {
			self.appController?.clickPlay()
		}
		return .success
	}

	@objc private func clickPause() -> MPRemoteCommandHandlerStatus {
		DispatchQueue.main.async {
			self.appController?.clickPause()
		}
		return .success
	}

	@objc private func clickStop() -> MPRemoteCommandHandlerStatus {
		DispatchQueue.main.async {
			self.appController?.clickStop()
		}
		return .success
	}

	@objc private func clickNext() -> MPRemoteCommandHandlerStatus {
		DispatchQueue.main.async {
			self.appController?.clickNext()
		}
		return .success
	}

	@objc private func clickPrev() -> MPRemoteCommandHandlerStatus {
		DispatchQueue.main.async {
			self.appController?.clickPrev()
		}
		return .success
	}

	@objc private func clickSeek(_ event: MPChangePlaybackPositionCommandEvent) -> MPRemoteCommandHandlerStatus {
		let positionTime = event.positionTime
		DispatchQueue.main.async {
			self.appController?.clickSeek(positionTime)
		}
		return .success
	}
}