//
//  PreferencesController.swift
//  Cog
//
//  Created by Vincent Spader on 9/4/06.
//  Copyright 2006 Vincent Spader. All rights reserved.
//

import AppKit

@objc(PreferencesController)
class PreferencesController: NSObject {
	private var window: PreferencesWindow?

	private func initWindow() {
		if window == nil {
			window = PreferencesWindow()
		}
	}

	@IBAction func showPreferences(_ sender: Any?) {
		initWindow()

		// Show the preferences window
		window?.show()
	}

	@IBAction func showPathSuggester(_ sender: Any?) {
		initWindow()

		// Show the path suggester
		window?.showPathSuggester()
	}

	@IBAction func showRubberbandSettings(_ sender: Any?) {
		initWindow()

		window?.showRubberbandSettings()
	}
}