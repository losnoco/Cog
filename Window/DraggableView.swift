//
//  DraggableView.swift
//  Cog
//
//  Created by Kevin López Brante on 20-06-22.
//

import Cocoa

class DraggableImageView: NSImageView {

    override func draw(_ dirtyRect: NSRect) {
        super.draw(dirtyRect)

        // Drawing code here.
    }
    
    override func mouseDown(with event: NSEvent) {
        window?.performDrag(with: event)
    }
    
}

class DraggableVFXView: NSVisualEffectView {
    
    override func mouseDown(with event: NSEvent) {
        window?.performDrag(with: event)
    }
    
}
