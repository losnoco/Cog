//
//  FeedbackController.swift
//  Cog
//
//  Created by Vincent Spader on 3/26/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

import AppKit

@objc(FeedbackController)
class FeedbackController: NSWindowController, NSWindowDelegate {
	@IBOutlet private var nameView: NSTextField?
	@IBOutlet private var emailView: NSTextField?
	@IBOutlet private var messageView: NSTextView?

	private var finished = false

	@objc private(set) var name: String?
	@objc private(set) var email: String?
	@objc private(set) var comments: String?

	/// Called once, on the main thread, when the window is done with: YES if
	/// the feedback was sent, NO if it was cancelled or closed.
	@objc var completionHandler: ((Bool) -> Void)?

	convenience init() {
		self.init(windowNibName: "Feedback")
	}

	@IBAction override func showWindow(_ sender: Any?) {
		nameView?.stringValue = ""
		emailView?.stringValue = ""
		messageView?.string = ""

		window?.delegate = self
		super.showWindow(sender)
	}

	/// Reports how the window was done with, once.
	private func finish(_ sent: Bool) {
		if finished { return }
		finished = true
		let handler = completionHandler
		completionHandler = nil
		handler?(sent)
	}

	// Closed with its close button: as cancelled.
	func windowWillClose(_ notification: Notification) {
		finish(false)
	}

	@IBAction func sendFeedback(_ sender: Any?) {
		name = nameView?.stringValue
		email = emailView?.stringValue
		comments = messageView?.string

		finish(true)
		window?.close()
	}

	@IBAction func cancel(_ sender: Any?) {
		finish(false)
		window?.close()
	}
}