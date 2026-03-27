//
//  CreditsView.swift
//  Cog
//

import SwiftUI
import WebKit

// MARK: - macOS 10.15–15

private struct LegacyCreditsWebView: NSViewRepresentable {
    let url: URL

    func makeNSView(context: Context) -> WKWebView {
        let webView = WKWebView()
        webView.navigationDelegate = context.coordinator
        webView.setValue(false, forKey: "drawsBackground")
        return webView
    }

    func updateNSView(_ nsView: WKWebView, context: Context) {
        nsView.load(URLRequest(url: url))
    }

    func makeCoordinator() -> Coordinator {
        Coordinator()
    }

    @MainActor final class Coordinator: NSObject, WKNavigationDelegate {
        func webView(
            _ webView: WKWebView,
            decidePolicyFor navigationAction: WKNavigationAction,
            decisionHandler: @escaping @MainActor @Sendable (WKNavigationActionPolicy) -> Void
        ) {
            if navigationAction.navigationType == .linkActivated,
               let url = navigationAction.request.url {
                NSWorkspace.shared.open(url)
                decisionHandler(.cancel)
            } else {
                decisionHandler(.allow)
            }
        }
    }
}

// MARK: - macOS 26+

@available(macOS 26.0, *)
private struct ModernCreditsWebView: View {
    let url: URL

    @State private var page = WebPage()

    @Environment(\.openURL) var openURL


    var body: some View {
        WebView(page)
            .webViewContentBackground(.hidden)
            .onAppear(perform: loadCredits)
            .onChange(of: page.url) { newURL in
                if let url = newURL, url.lastPathComponent != "Credits.html" {
                    openURL(url)
                    page.stopLoading()
                }
            }
    }

    private func loadCredits() {
        page.load(URLRequest(url: url))
    }
}

// MARK: - Public

struct CreditsView: View {
    private let url = Bundle.main.url(forResource: "Credits", withExtension: "html")

    var body: some View {
        if let url {
            if #available(macOS 26.0, *) {
                ModernCreditsWebView(url: url)
            } else {
                LegacyCreditsWebView(url: url)
            }
        }
    }
}

#Preview {
    CreditsView()
        .frame(width: 400, height: 250)
}
