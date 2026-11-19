//
//  AboutView.swift
//  Cog
//
//  Created by Kevin Doncam Demian López Brante on 18-03-26.
//

import SwiftUI

/// The About window's two faces: "About Cog" shows the card, and its Option
/// alternate in the application menu shows the details.
enum AboutMode {
	case card
	case details
}

enum AboutTab: Hashable {
	case credits
	case acknowledgements
	case diagnostics
}

@MainActor
final class AboutModel: ObservableObject {
	@Published var mode: AboutMode = .card
	@Published var tab: AboutTab = .credits

	/// Set by the window controller, which has to resize the window with
	/// the change.
	var showDetails: (AboutTab) -> Void = { _ in }
}

/// What the bundle says about this build.
struct AboutInfo {
	let name: String
	/// The full `git describe` version where the build recorded one.
	let version: String
	let copyright: String

	static let current = AboutInfo()

	private init() {
		func string(_ key: String) -> String? {
			guard let value = Bundle.main.object(forInfoDictionaryKey: key) as? String, !value.isEmpty else { return nil }
			return value
		}
		name = string("CFBundleName") ?? "Cog"
		version = string("GitVersion") ?? string("CFBundleShortVersionString") ?? "?"
		copyright = string("NSHumanReadableCopyright") ?? ""
	}

	static func copy(_ string: String) {
		let pasteboard = NSPasteboard.general
		pasteboard.clearContents()
		pasteboard.setString(string, forType: .string)
	}
}

struct AboutView: View {
	@ObservedObject var model: AboutModel

	var body: some View {
		// Sized here as well as by the window: before macOS 13 the hosting
		// controller sizes the window to the view's fitting size, which for
		// the card's wrapping text could be taller than the screen.
		switch model.mode {
		case .card:
			AboutCardView(model: model)
				.frame(width: AboutWindowController.cardSize.width, height: AboutWindowController.cardSize.height)
		case .details:
			AboutDetailsView(model: model)
				.frame(minWidth: AboutWindowController.detailsMinSize.width, maxWidth: .infinity,
				       minHeight: AboutWindowController.detailsMinSize.height, maxHeight: .infinity)
		}
	}
}

/// The version, with a button and a context menu that copy it.
struct AboutVersionLabel: View {
	private let version = AboutInfo.current.version

	var body: some View {
		HStack(spacing: 4) {
			Text("Version \(version)")
				.foregroundColor(.secondary)
				.textSelection(.enabled)
			Button(action: copy) {
				Image(systemName: "doc.on.doc")
			}
			.buttonStyle(.borderless)
			.help("Copy Version")
		}
		.contextMenu {
			Button(action: copy) {
				Label("Copy Version", systemImage: "doc.on.clipboard")
			}
		}
	}

	private func copy() {
		AboutInfo.copy(version)
	}
}

/// The LoSnoCo wordmark, which opens losno.co.
struct LoSnoCoWordmark: View {
	let height: CGFloat

	var body: some View {
		if let url = URL(string: "https://losno.co") {
			Link(destination: url) {
				image
			}
			.buttonStyle(.plain)
			.help(Text(verbatim: "losno.co"))
		} else {
			image
		}
	}

	private var image: some View {
		Image("LoSnoCoWordmark")
			.resizable()
			.scaledToFit()
			.frame(height: height)
			.accessibilityLabel(Text(verbatim: "LoSnoCo"))
	}
}

struct AboutCardView: View {
	@ObservedObject var model: AboutModel

	private let info = AboutInfo.current

	var body: some View {
		VStack(spacing: 10) {
			Image(nsImage: NSApp.applicationIconImage)
				.resizable()
				.frame(width: 128, height: 128)

			Text(verbatim: info.name)
				.font(.system(size: 30, weight: .bold))

			AboutVersionLabel()

			VStack(spacing: 8) {
				HStack(spacing: 16) {
					Button("Credits") { model.showDetails(.credits) }
					Button("Acknowledgements") { model.showDetails(.acknowledgements) }
				}
				HStack(spacing: 16) {
					if let url = URL(string: "https://github.com/losnoco/Cog") {
						Link(destination: url) {
							Text(verbatim: "GitHub")
						}
					}
#if COG_SPARKLE
					DonateMenu()
#endif
				}
			}
			.buttonStyle(.link)
			.padding(.top, 10)

			Spacer(minLength: 0)

			LoSnoCoWordmark(height: 22)

			Spacer(minLength: 0)

			Text(verbatim: info.copyright)
				.font(.footnote)
				.foregroundColor(.secondary)
				.multilineTextAlignment(.center)
				.fixedSize(horizontal: false, vertical: true)
		}
		.padding(.horizontal, 24)
		.padding(.bottom, 20)
	}
}

#if COG_SPARKLE
/// The same links as the application menu's Donate submenu, through the
/// actions AppController already has for them.
private struct DonateMenu: View {
	var body: some View {
		Menu(String(localized: "Donate", table: "Direct")) {
			Button { send("openLiberapayPage:") } label: { Text(verbatim: "LiberaPay") }
			Divider()
			Button { send("openPaypalPage:") } label: { Text(verbatim: "PayPal") }
			Divider()
			Button { send("openKofiPage:") } label: { Text(verbatim: "Ko-fi") }
			Button { send("openPatreonPage:") } label: { Text(verbatim: "Patreon") }
		}
		.menuStyle(.borderlessButton)
		.fixedSize()
	}

	private func send(_ action: String) {
		NSApp.sendAction(Selector(action), to: NSApp.delegate, from: nil)
	}
}
#endif

#Preview {
	AboutView(model: AboutModel())
		.frame(width: 320, height: 420)
}
