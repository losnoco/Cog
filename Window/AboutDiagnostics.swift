//
//  AboutDiagnostics.swift
//  Cog
//

import Cocoa
import CogAudio

/// A plain-text description of this build, the machine and the registered
/// plugins, for pasting into bug reports. Its labels are English whatever
/// the interface language, since that is where it gets read.
@MainActor
enum AboutDiagnostics {
	static func report() -> String {
		var lines: [String] = []

		func info(_ key: String) -> String {
			Bundle.main.object(forInfoDictionaryKey: key) as? String ?? "unknown"
		}
#if COG_SPARKLE
		let edition = "Direct"
#else
		let edition = "App Store"
#endif
		lines.append("Cog \(info("GitVersion")) (\(edition))")
		lines.append("Build: \(info("CFBundleVersion")), \(info("BuildTime"))")
		lines.append("Commit: \(info("GitHash"))")

		lines.append("")
		lines.append("macOS: \(ProcessInfo.processInfo.operatingSystemVersionString)")
		lines.append("Hardware: \(sysctlString("hw.model") ?? "unknown"), \(architecture)")
		lines.append("Language: \(Locale.preferredLanguages.first ?? "unknown")")

		lines.append("")
		lines.append("Output:")
		// The engine's own description rather than the Signal Inspector's
		// wording of it, which is in the interface language.
		if let status = SignalStatusModel.shared.status, !status.isEmpty {
			let entries = status.map { (key: "\($0.key)", value: describe($0.value)) }
			lines += entries.sorted { $0.key < $1.key }.map { "  \($0.key): \($0.value)" }
		} else {
			lines.append("  Idle")
		}

		if let plugins = PluginController.shared() as? PluginController {
			let sources = classes(from: plugins.sources)
			lines.append("")
			lines.append("Sources (\(sources.count)):")
			lines += sources.map { "  \($0.name): \($0.keys.joined(separator: " "))" }

			let sections: [(String, NSDictionary?)] = [
				("Containers", plugins.containers),
				("Decoders", plugins.decodersByExtension),
				("Metadata readers", plugins.metadataReaders),
				("Properties readers", plugins.propertiesReadersByExtension),
			]
			for (title, dictionary) in sections {
				let entries = classes(from: dictionary)
				lines.append("")
				lines.append("\(title) (\(entries.count)):")
				lines += entries.map { "  \($0.name): \($0.keys.joined(separator: " "))" }
			}
		} else {
			lines.append("")
			lines.append("Plugins: unavailable")
		}

		return lines.joined(separator: "\n")
	}

	/// A value of the output status dictionary, in a form that does not
	/// depend on the interface language or the number format.
	private static func describe(_ value: Any) -> String {
		switch value {
		case let number as NSNumber:
			if CFGetTypeID(number) == CFBooleanGetTypeID() {
				return number.boolValue ? "yes" : "no"
			}
			return number.stringValue
		case let string as String:
			return string
		case let array as [Any]:
			return array.isEmpty ? "none" : array.map(describe).joined(separator: ", ")
		case let dictionary as [AnyHashable: Any]:
			return dictionary.map { "\($0.key)=\(describe($0.value))" }.sorted().joined(separator: ", ")
		case let boxed as NSValue:
			guard let format = SignalFormatting.format(from: boxed) else { return "\(boxed)" }
			let name = SignalFormatting.sampleFormatName(format, isDoP: false)
			return "\(name), \(NSNumber(value: format.mSampleRate).stringValue) Hz, \(format.mChannelsPerFrame) ch"
		default:
			return "\(value)"
		}
	}

	/// Turns PluginController's tables around: they map a URL scheme or an
	/// extension to one class name or a list of them, and the report reads
	/// better with each class followed by what it handles.
	private static func classes(from dictionary: NSDictionary?) -> [(name: String, keys: [String])] {
		var keysByClass: [String: [String]] = [:]
		for (key, value) in dictionary ?? [:] {
			guard let key = key as? String else { continue }
			let names: [String]
			if let list = value as? [String] {
				names = list
			} else if let name = value as? String {
				names = [name]
			} else {
				names = []
			}
			for name in names {
				keysByClass[name, default: []].append(key)
			}
		}
		return keysByClass
			.map { (name: $0.key, keys: $0.value.sorted()) }
			.sorted { $0.name.localizedCaseInsensitiveCompare($1.name) == .orderedAscending }
	}

	private static var architecture: String {
#if arch(arm64)
		return "arm64"
#else
		var translated: Int32 = 0
		var size = MemoryLayout<Int32>.size
		let rosetta = sysctlbyname("sysctl.proc_translated", &translated, &size, nil, 0) == 0 && translated == 1
		return rosetta ? "x86_64 (Rosetta)" : "x86_64"
#endif
	}

	private static func sysctlString(_ name: String) -> String? {
		var size = 0
		guard sysctlbyname(name, nil, &size, nil, 0) == 0, size > 0 else { return nil }
		var buffer = [CChar](repeating: 0, count: size)
		guard sysctlbyname(name, &buffer, &size, nil, 0) == 0 else { return nil }
		return String(decoding: buffer.prefix { $0 != 0 }.map { UInt8(bitPattern: $0) }, as: UTF8.self)
	}
}
