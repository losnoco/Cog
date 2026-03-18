//
//  AboutWIndowController.swift
//  Cog
//
//  Created by Kevin López Brante on 20-06-22.
//

import Cocoa

class AboutWindowController: NSWindowController {

    override func loadWindow() {
        let hostingController = AboutViewHostingController()
        let window = NSWindow(
            contentRect: NSRect(x: 0, y: 0, width: 400, height: 600),
            styleMask: [.titled, .closable, .fullSizeContentView],
            backing: .buffered,
            defer: false
        )
        window.contentViewController = hostingController
        window.title = "About Cog"
        window.titlebarAppearsTransparent = true
        window.titleVisibility = .hidden
        window.isMovableByWindowBackground = true
        window.appearance = NSAppearance(named: .darkAqua)
        self.window = window
    }

    override func showWindow(_ sender: Any?) {
        if window == nil {
            loadWindow()
        }
        window?.center()
        super.showWindow(sender)
    }
}
