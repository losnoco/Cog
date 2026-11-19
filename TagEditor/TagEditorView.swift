//
//  TagEditorView.swift
//  Cog
//
//  The tag editor window's content: the named fields, then for one file its
//  other tags, cover and chapters, as far as the format holds them.
//

import SwiftUI

struct TagEditorView: View {
	@ObservedObject var model: TagEditorModel
	let save: () -> Void
	let cancel: () -> Void
	let chooseCover: () -> Void
	/// What the window opens at: before macOS 13 the hosting controller
	/// sizes the window to the view, so this keeps it inside the screen.
	var idealHeight: CGFloat = 680

	var body: some View {
		VStack(spacing: 0) {
			if #available(macOS 13.0, *) {
				form.formStyle(.grouped)
			} else {
				ScrollView { legacyLayout.padding(20) }
			}
			Divider()
			buttons.padding(12)
		}
		.frame(minWidth: 480, idealWidth: 560, maxWidth: .infinity,
		       minHeight: min(420, idealHeight), idealHeight: idealHeight, maxHeight: .infinity)
		.disabled(model.isSaving)
	}

	private var form: some View {
		Form {
			Section {
				ForEach(TagEditorModel.Field.allCases.filter { !$0.isLong }) { field in
					fieldRow(field)
				}
			} header: {
				header
			}

			ForEach(TagEditorModel.Field.allCases.filter(\.isLong)) { field in
				Section {
					TextEditor(text: model.binding(for: field))
						.font(.body)
						.frame(minHeight: field == .lyrics ? 96 : 48)
					if let note = fieldNote(field) {
						Text(note).font(.caption).foregroundColor(.secondary)
					}
				} header: {
					Text(field.label).bold()
				}
			}

			if model.supportsFreeRows {
				freeRowsSection
			}
			if model.supportsArt || model.artFromFolder {
				coverSection
			}
			if model.supportsChapters {
				chaptersSection
			}
		}
	}

	// MARK: - macOS 12

	/// The widest field label, measured, for the label column.
	@State private var labelWidth: CGFloat = 0

	/// Before macOS 13, a Form puts its labels' column where the section
	/// headers start, and a long label runs off the window's edge ("Artista
	/// del álbum"). Here the labels get a column as wide as the widest, and
	/// the sections stack beneath under their headings.
	private var legacyLayout: some View {
		VStack(alignment: .leading, spacing: 18) {
			header
			VStack(alignment: .leading, spacing: 8) {
				ForEach(TagEditorModel.Field.allCases.filter { !$0.isLong }) { field in
					HStack(alignment: .firstTextBaseline, spacing: 8) {
						Text(field.label)
							.fixedSize()
							.background(GeometryReader { proxy in
								Color.clear.preference(key: LabelWidthKey.self, value: proxy.size.width)
							})
							.frame(width: labelWidth > 0 ? labelWidth : nil, alignment: .trailing)
						VStack(alignment: .leading, spacing: 2) {
							fieldTextField(field).labelsHidden()
							if let note = fieldNote(field) {
								Text(note).font(.caption).foregroundColor(.secondary)
									.fixedSize(horizontal: false, vertical: true)
							}
						}
					}
				}
			}
			.onPreferenceChange(LabelWidthKey.self) { labelWidth = $0 }

			ForEach(TagEditorModel.Field.allCases.filter(\.isLong)) { field in
				legacySection(field.label) {
					TextEditor(text: model.binding(for: field))
						.font(.body)
						.frame(minHeight: field == .lyrics ? 96 : 48)
						.border(Color(nsColor: .separatorColor))
					if let note = fieldNote(field) {
						Text(note).font(.caption).foregroundColor(.secondary)
					}
				}
			}
			if model.supportsFreeRows {
				legacySection("Other Tags") {
					freeRowsContent
					freeRowsFooter
				}
			}
			if model.supportsArt || model.artFromFolder {
				legacySection("Cover") { coverContent }
			}
			if model.supportsChapters {
				legacySection("Chapters") {
					chaptersContent
					chaptersFooter
				}
			}
		}
	}

	private func legacySection<Content: View>(_ title: LocalizedStringKey, @ViewBuilder content: () -> Content) -> some View {
		VStack(alignment: .leading, spacing: 6) {
			Text(title).bold()
			content()
		}
	}

	// MARK: - Header and fields

	private var header: some View {
		VStack(alignment: .leading, spacing: 2) {
			if model.isBatch {
				Text("Editing \(model.sources.count) files").bold()
				Text("Only the fields you change are saved, to every file.").font(.caption).foregroundColor(.secondary)
			} else if let source = model.sources.first {
				Text(source.file.lastPathComponent).bold().lineLimit(1).truncationMode(.middle)
				if let chapter = source.chapter, model.supportsChapters {
					Text("Chapter \(chapter + 1) of this file").font(.caption).foregroundColor(.secondary)
				}
			}
			if model.leftOut > 0 {
				Text("\(model.leftOut) of the selected files can't have their tags saved and are left out.")
					.font(.caption).foregroundColor(.secondary)
			}
		}
	}

	private func fieldTextField(_ field: TagEditorModel.Field) -> some View {
		TextField(field.label, text: model.binding(for: field),
		          prompt: model.fields[field]?.differs == true ? Text("Multiple values") : nil)
	}

	private func fieldRow(_ field: TagEditorModel.Field) -> some View {
		VStack(alignment: .leading, spacing: 2) {
			fieldTextField(field)
			if let note = fieldNote(field) {
				Text(note).font(.caption).foregroundColor(.secondary)
			}
		}
	}

	private func fieldNote(_ field: TagEditorModel.Field) -> LocalizedStringKey? {
		guard let state = model.fields[field] else { return nil }
		if state.valueCount > 1 && !state.touched {
			return "This field holds several values. Typing over it saves one."
		}
		return nil
	}

	// MARK: - Other tags

	private var freeRowsSection: some View {
		Section {
			freeRowsContent
		} header: {
			Text("Other Tags").bold()
		} footer: {
			freeRowsFooter
		}
	}

	@ViewBuilder private var freeRowsContent: some View {
			ForEach($model.freeRows) { $row in
				VStack(alignment: .leading, spacing: 4) {
					HStack(alignment: .firstTextBaseline) {
						TextField("Name", text: $row.name)
							.frame(width: 150)
							.labelsHidden()
						VStack(spacing: 4) {
							ForEach(row.values.indices, id: \.self) { index in
								HStack {
									TextField("Value", text: valueBinding($row, index))
										.labelsHidden()
									if row.values.count > 1 {
										Button {
											row.values.remove(at: index)
										} label: {
											Image(systemName: "minus.circle")
										}
										.buttonStyle(.borderless)
										.help("Remove this value")
									}
								}
							}
						}
						if model.keepsRepeatedValues {
							Button {
								row.values.append("")
							} label: {
								Image(systemName: "plus.circle")
							}
							.buttonStyle(.borderless)
							.help("Add a value")
						}
						Button {
							model.freeRows.removeAll { $0.id == row.id }
						} label: {
							Image(systemName: "trash")
						}
						.buttonStyle(.borderless)
						.help("Remove this tag")
					}
					if let note = model.note(for: row) {
						Text(note).font(.caption).foregroundColor(.secondary)
					}
				}
			}
			Button("Add Tag") { model.addFreeRow() }
	}

	@ViewBuilder private var freeRowsFooter: some View {
		if model.foldsNames {
			Text("Names this format has no field of its own for are saved as custom fields.")
				.font(.caption).foregroundColor(.secondary)
				.fixedSize(horizontal: false, vertical: true)
		}
	}

	private func valueBinding(_ row: Binding<TagEditorModel.FreeRow>, _ index: Int) -> Binding<String> {
		Binding(get: { index < row.wrappedValue.values.count ? row.wrappedValue.values[index] : "" },
		        set: { if index < row.wrappedValue.values.count { row.wrappedValue.values[index] = $0 } })
	}

	// MARK: - Cover

	private var shownCover: Data? {
		switch model.art {
			case .keep: return model.embeddedArt
			case .remove: return nil
			case let .replace(data): return data
		}
	}

	private var coverSection: some View {
		Section {
			coverContent
		} header: {
			Text("Cover").bold()
		}
	}

	private var coverContent: some View {
			HStack(alignment: .top, spacing: 12) {
				Group {
					if let data = shownCover, let image = NSImage(data: data) {
						Image(nsImage: image).resizable().aspectRatio(contentMode: .fit)
					} else {
						Image(systemName: "photo").font(.largeTitle).foregroundColor(.secondary)
					}
				}
				.frame(width: 120, height: 120)

				VStack(alignment: .leading, spacing: 6) {
					if model.supportsArt {
						Button("Choose…", action: chooseCover)
						Button("Remove") { model.art = .remove }
							.disabled(shownCover == nil)
						if model.art != .keep {
							Button("Keep Current Cover") { model.art = .keep }
						}
					}
					if model.artFromFolder && model.art == .keep {
						Text("The playlist shows a cover from the file's folder. It isn't saved into the file.")
							.font(.caption).foregroundColor(.secondary)
							.fixedSize(horizontal: false, vertical: true)
					}
				}
			}
	}

	// MARK: - Chapters

	private var chaptersSection: some View {
		Section {
			chaptersContent
		} header: {
			Text("Chapters").bold()
		} footer: {
			chaptersFooter
		}
	}

	private var chaptersFooter: some View {
		Text("Each chapter ends where the next one starts, and the last where the file ends.")
			.font(.caption).foregroundColor(.secondary)
			.fixedSize(horizontal: false, vertical: true)
	}

	@ViewBuilder private var chaptersContent: some View {
			ForEach($model.chapters) { $chapter in
				let index = model.chapters.firstIndex { $0.id == chapter.id }
				HStack {
					Text(verbatim: "\((index ?? 0) + 1)")
						.monospacedDigit()
						.frame(width: 24, alignment: .trailing)
						.foregroundColor(index == model.highlightedChapter ? .accentColor : .secondary)
					ChapterTimeField(milliseconds: $chapter.start)
						.frame(width: 110)
					TextField("Chapter Title", text: $chapter.title)
						.labelsHidden()
					Button {
						model.chapters.removeAll { $0.id == chapter.id }
					} label: {
						Image(systemName: "trash")
					}
					.buttonStyle(.borderless)
					.help("Remove this chapter")
				}
			}
			Button("Add Chapter") { model.addChapter() }
	}

	// MARK: - Buttons

	private var buttons: some View {
		HStack {
			if model.isSaving {
				ProgressView().controlSize(.small)
				Text("Saving…").foregroundColor(.secondary)
			}
			Spacer()
			Button("Cancel", action: cancel)
				.keyboardShortcut(.cancelAction)
			Button("Save", action: save)
				.keyboardShortcut(.defaultAction)
				.disabled(!model.hasChanges || model.isSaving)
		}
	}
}

private struct LabelWidthKey: PreferenceKey {
	static let defaultValue: CGFloat = 0
	static func reduce(value: inout CGFloat, nextValue: () -> CGFloat) {
		value = max(value, nextValue())
	}
}

/// A chapter's start, typed as a time and taken when it reads as one.
private struct ChapterTimeField: View {
	@Binding var milliseconds: Int
	@State private var text = ""
	@FocusState private var focused: Bool

	var body: some View {
		TextField("Start", text: $text)
			.labelsHidden()
			.monospacedDigit()
			.focused($focused)
			.onAppear { text = TagEditorModel.timeText(milliseconds) }
			.onSubmit(commit)
			.onChange(of: focused) { isFocused in
				if !isFocused { commit() }
			}
			.onChange(of: milliseconds) { value in
				if !focused { text = TagEditorModel.timeText(value) }
			}
	}

	private func commit() {
		if let value = TagEditorModel.milliseconds(from: text) {
			milliseconds = value
		}
		text = TagEditorModel.timeText(milliseconds)
	}
}
