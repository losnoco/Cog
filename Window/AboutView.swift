//
//  AboutView.swift
//  Cog
//
//  Created by Kevin Doncam Demian López Brante on 18-03-26.
//

import SwiftUI

struct AboutView: View {

    @State private var iconImage: NSImage?

    @State private var appName = "CO_LOSNO_COG_APPNAME"
    @State private var appCopyright = "CO_LOSNO_COG_APPCOPYRIGHT"
    @State private var appFullVersion = "CO_LOSNO_COG_APPFULLVERSION"


    var body: some View {
        VStack(alignment: .leading) {
            HStack(alignment: .center) {
                if let iconImage = iconImage {
                    Image(nsImage: iconImage)
                }
                Text(appName).font(.system(size: 32)).bold()
                Spacer()
            }.padding(.horizontal, 10)

            Text("Version \(appFullVersion)").padding(.horizontal, 10)
                .contextMenu {
                    Button {
                        let pasteboard = NSPasteboard.general
                        pasteboard.clearContents() // Essential for macOS
                        pasteboard.setString(appFullVersion, forType: .string)
                    } label: {
                        if #available(macOS 11.0, *) {
                            Label(
                                "Copy Version",
                                systemImage: "doc.on.clipboard"
                            )
                        } else {
                            Text("Copy Version")
                        }
                    }
                }

            Divider()
            CreditsView()
            Divider()

            Text(appCopyright).padding([.horizontal, .bottom], 10)
        }
        .onAppear(perform: loadBundleInfo)
        .frame(minWidth: 400, minHeight: 400)
    }

    private func loadBundleInfo() {
        iconImage = NSWorkspace.shared.icon(forFile: Bundle.main.bundlePath)
        iconImage?.size = NSSize(width: 64, height: 64)
        appName = Bundle.main.object(forInfoDictionaryKey: "CFBundleName") as! String
        appFullVersion = Bundle.main.object(forInfoDictionaryKey: "GitVersion") as! String
        appCopyright = Bundle.main.object(forInfoDictionaryKey: "NSHumanReadableCopyright") as! String
    }
}

#Preview {
    AboutView()
}

@objc(AboutViewHostingController)
class AboutViewHostingController: NSHostingController<AboutView> {

    @objc required init() {
        super.init(rootView: AboutView())
    }

    @objc required init?(coder: NSCoder) {
        super.init(coder: coder, rootView: AboutView())
    }
}


