//
//  AboutWIndowController.swift
//  Cog
//
//  Created by Kevin López Brante on 20-06-22.
//

import Cocoa

class AboutWindowController: NSWindowController {
    
    @IBOutlet weak var appName: NSTextField!
    @IBOutlet weak var appVersion: NSTextField!
    @IBOutlet weak var appCopyright: NSTextField!
    
    @IBOutlet var creditsView: NSTextView!
    
    override var windowNibName: NSNib.Name? {
        return "AboutWindowController"
    }
    
    @IBOutlet weak var vfxView: NSVisualEffectView!
    
    override func windowDidLoad() {
        super.windowDidLoad()
        
        self.window?.center()
        self.window?.isMovableByWindowBackground = true

        vfxView.wantsLayer = true
        vfxView.canDrawSubviewsIntoLayer = true
        
        vfxView.layer?.cornerRadius = 10.0
        vfxView.layer?.masksToBounds = true

        // fill up labels
        
        appName.stringValue = Bundle.main.object(forInfoDictionaryKey: "CFBundleName") as! String
        
        let shortVersionString = Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as! String
        let fullVersionString = Bundle.main.object(forInfoDictionaryKey: "CFBundleVersion") as! String
        
        appVersion.stringValue = String.localizedStringWithFormat(            NSLocalizedString("Version %@ (%@)", comment: "Version string"), shortVersionString, fullVersionString);
        
        appCopyright.stringValue = Bundle.main.object(forInfoDictionaryKey: "NSHumanReadableCopyright") as! String
        
        if let creditsFile = Bundle.main.url(forResource: "Credits", withExtension: "html") {
            let data = try! Data(contentsOf: creditsFile)
            
            if let attributedString = try? NSAttributedString(data: data, options: [.documentType: NSAttributedString.DocumentType.html], documentAttributes: nil) {
                creditsView.textStorage?.setAttributedString(attributedString)
            }
        }

    }
    
}
