//
//  AboutDetailsView.swift
//  Cog
//

import SwiftUI

/// The About window's detailed face: who made Cog, what it is built from,
/// and a report of this build and its plugins for bug reports.
struct AboutDetailsView: View {
	@ObservedObject var model: AboutModel

	private let info = AboutInfo.current

	var body: some View {
		VStack(spacing: 0) {
			HStack(spacing: 12) {
				Image(nsImage: NSApp.applicationIconImage)
					.resizable()
					.frame(width: 48, height: 48)
				VStack(alignment: .leading, spacing: 2) {
					Text(verbatim: info.name)
						.font(.title2.bold())
					AboutVersionLabel()
				}
				Spacer()
				Picker("", selection: $model.tab) {
					Text("Credits").tag(AboutTab.credits)
					Text("Acknowledgements").tag(AboutTab.acknowledgements)
					Text("Diagnostics").tag(AboutTab.diagnostics)
				}
				.pickerStyle(.segmented)
				.labelsHidden()
				.fixedSize()
			}
			.padding(.horizontal, 16)
			.padding(.vertical, 10)

			Divider()

			Group {
				switch model.tab {
				case .credits:
					CreditsView()
				case .acknowledgements:
					AcknowledgementsView()
				case .diagnostics:
					AboutDiagnosticsView()
				}
			}
			.frame(maxWidth: .infinity, maxHeight: .infinity)
		}
	}
}

private struct AboutDiagnosticsView: View {
	@State private var report = ""

	var body: some View {
		VStack(spacing: 0) {
			ScrollView {
				Text(verbatim: report)
					.font(.system(.callout, design: .monospaced))
					.textSelection(.enabled)
					.frame(maxWidth: .infinity, alignment: .leading)
					.padding(16)
			}
			// Unlabeled: Cog's own Color(nsColor:) flattens to sRGB, which would
			// lose the light/dark adaptation of this system color.
			.background(Color(NSColor.textBackgroundColor))

			Divider()

			HStack {
				Spacer()
				Button("Copy Diagnostics") {
					refresh()
					AboutInfo.copy(report)
				}
			}
			.padding(10)
		}
		.onAppear(perform: refresh)
		.onReceive(NotificationCenter.default.publisher(for: SignalStatusModel.didChangeNotification)) { _ in
			refresh()
		}
	}

	private func refresh() {
		report = AboutDiagnostics.report()
	}
}

#Preview {
	let model = AboutModel()
	model.mode = .details
	return AboutView(model: model)
		.frame(width: 760, height: 560)
}
