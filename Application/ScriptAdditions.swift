//
//  ScriptAdditions.swift
//  Cog
//
//  Created by Christopher Snowhill on 2/21/22.
//  Copyright 2022 Christopher Snowhill. All rights reserved.
//

import AppKit

// The AppleScript commands of Cog.sdef, which names these selectors on the
// application class.
extension NSApplication {
	@objc func playbackStart(_ command: NSScriptCommand) -> Any {
		(NSApp.delegate as? AppController)?.clickPlay()
		return true
	}

	@objc func playbackPause(_ command: NSScriptCommand) -> Any {
		(NSApp.delegate as? AppController)?.clickPause()
		return true
	}

	@objc func playbackStop(_ command: NSScriptCommand) -> Any {
		(NSApp.delegate as? AppController)?.clickStop()
		return true
	}

	@objc func playbackPrevious(_ command: NSScriptCommand) -> Any {
		(NSApp.delegate as? AppController)?.clickPrev()
		return true
	}

	@objc func playbackNext(_ command: NSScriptCommand) -> Any {
		(NSApp.delegate as? AppController)?.clickNext()
		return true
	}
}