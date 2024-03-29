//
//  AboutWIndowController.swift
//  Cog
//
//  Created by Kevin López Brante on 20-06-22.
//

import Cocoa
import WebKit

class AboutWindowController: NSWindowController, WKNavigationDelegate {
    
    @IBOutlet weak var appName: NSTextField!
    @IBOutlet weak var appVersion: NSTextField!
    @IBOutlet weak var appCopyright: NSTextField!
    
    @IBOutlet weak var creditsView: WKWebView!
    
    override var windowNibName: NSNib.Name? {
        return "AboutWindowController"
    }
    
    @IBOutlet weak var vfxView: NSVisualEffectView!
    
    override func windowDidLoad() {
        super.windowDidLoad()
        
        self.window?.center()
        self.window?.isMovableByWindowBackground = true
        
        creditsView.setValue(false, forKey: "drawsBackground")

        vfxView.wantsLayer = true
        vfxView.layer?.cornerRadius = 4

        // fill up labels
        
        appName.stringValue = Bundle.main.object(forInfoDictionaryKey: "CFBundleName") as! String
        
        let shortVersionString = Bundle.main.object(forInfoDictionaryKey: "CFBundleShortVersionString") as! String
        let fullVersionString = Bundle.main.object(forInfoDictionaryKey: "GitVersion") as! String
        
        appVersion.stringValue = String.localizedStringWithFormat(NSLocalizedString("Version %@ (%@)", comment: "Version string"), shortVersionString, fullVersionString);
        
        appCopyright.stringValue = Bundle.main.object(forInfoDictionaryKey: "NSHumanReadableCopyright") as! String
        
        if let creditsFile = Bundle.main.url(forResource: "Credits", withExtension: "html") {
            let data = try! Data(contentsOf: creditsFile)
            
            creditsView.loadHTMLString(String(data: data, encoding: .utf8) ?? "Could not load credits.", baseURL: nil)
            
        }
        
        creditsView.navigationDelegate = self

    }
    
    func webView(_ webView: WKWebView, decidePolicyFor navigationAction: WKNavigationAction, decisionHandler: @escaping (WKNavigationActionPolicy) -> Void) {
        if navigationAction.navigationType == .linkActivated,
           let url = navigationAction.request.url {
            NSWorkspace.shared.open(url)
            decisionHandler(.cancel)
            return
        }
        decisionHandler(.allow)
    }

    @objc func cancel(_ sender: Any?) {
        close()
    }
}
