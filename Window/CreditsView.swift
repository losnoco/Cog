//
//  CreditsView.swift
//  Cog
//

import SwiftUI

/// The people behind Cog, from the bundled Credits.json
/// (Acknowledgements/Credits.json in the repository). Third-party code and
/// data are listed in Acknowledgements instead.
struct Credits: Decodable {
	let sections: [Section]

	struct Section: Decodable, Identifiable {
		let id: String
		let people: [Person]
	}

	struct Person: Decodable, Hashable {
		let name: String
		let url: String?
		/// For translators: the language code they translated into.
		let language: String?
	}

	static func load() -> Credits? {
		guard let url = Bundle.main.url(forResource: "Credits", withExtension: "json"),
		      let data = try? Data(contentsOf: url) else { return nil }
		return try? JSONDecoder().decode(Credits.self, from: data)
	}
}

struct CreditsView: View {
	private let credits = Credits.load()

	/// Sections with this many people or more wrap into columns.
	private let columnThreshold = 6

	/// INAPI (Chile) accepted the LoSnoCo mark for registration on 2026-11-11
	/// (application 1678417, classes 9 and 42). It is registered once the final
	/// fees are paid: put the registration number here then, and the notice
	/// below the wordmark appears.
	private let trademarkRegistration: String? = nil

	var body: some View {
		ScrollView {
			VStack(spacing: 22) {
				Text("Now with 92% more future!")
					.font(.title3)
					.italic()

				ForEach(credits?.sections ?? []) { section in
					VStack(spacing: 6) {
						Text(title(forSection: section.id))
							.font(.subheadline.weight(.semibold))
							.foregroundColor(.secondary)
							.textCase(.uppercase)
						if section.people.count >= columnThreshold {
							LazyVGrid(columns: [GridItem(.adaptive(minimum: 170), spacing: 12)], spacing: 4) {
								ForEach(section.people, id: \.self, content: PersonView.init)
							}
						} else {
							ForEach(section.people, id: \.self, content: PersonView.init)
						}
					}
				}

				VStack(spacing: 8) {
					Text("This program has been made possible through contributions from users like you.")
						.italic()
					Text("Cog is licensed under the [GPL](https://www.gnu.org/licenses/old-licenses/gpl-2.0.txt). Code and data from third parties are under their own licenses, which Acknowledgements lists.")
						.foregroundColor(.secondary)
				}
				.font(.callout)
				.multilineTextAlignment(.center)
				.fixedSize(horizontal: false, vertical: true)

				VStack(spacing: 8) {
					LoSnoCoWordmark(height: 24)
					if let trademarkRegistration {
						Text("LoSnoCo and the LoSnoCo logo are registered trademarks of Kevin López Brante in Chile (INAPI registration no. \(trademarkRegistration)).")
							.font(.caption)
							.foregroundColor(.secondary)
							.multilineTextAlignment(.center)
							.fixedSize(horizontal: false, vertical: true)
					}
				}
				.padding(.top, 6)
			}
			.padding(24)
			.frame(maxWidth: .infinity)
		}
	}

	private func title(forSection id: String) -> LocalizedStringKey {
		switch id {
		case "created": return "Created by"
		case "maintained": return "Maintained by"
		case "development": return "Development"
		case "contributors": return "Code Contributions"
		case "translation": return "Translation"
		case "patrons": return "Patrons"
		default: return LocalizedStringKey(id)
		}
	}
}

private struct PersonView: View {
	let person: Credits.Person

	var body: some View {
		VStack(spacing: 1) {
			if let string = person.url, let url = URL(string: string) {
				Link(destination: url) {
					Text(verbatim: person.name)
				}
			} else {
				Text(verbatim: person.name)
					.textSelection(.enabled)
			}
			if let code = person.language,
			   let language = Locale.current.localizedString(forLanguageCode: code) {
				Text(verbatim: language.capitalized(with: .current))
					.font(.caption)
					.foregroundColor(.secondary)
			}
		}
		.frame(maxWidth: .infinity)
	}
}

#Preview {
	CreditsView()
		.frame(width: 640, height: 480)
}
