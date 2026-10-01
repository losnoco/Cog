//
//  ContentTypes.swift
//  Cog
//
//  Created by Christopher Snowhill on 9/30/26.
//

import Foundation
import UniformTypeIdentifiers

/// Open and save panels filter by UTType now, not by filename extension.
@objc(CogContentTypes)
final class ContentTypes: NSObject {
	/// Every type registered for each extension. An extension can be
	/// claimed by several apps under different types, and a file may be
	/// typed as any of them, so taking only the preferred one could grey out
	/// files the extension list accepts. An extension nothing has declared
	/// gets a dynamic type, so no extension goes unmatched.
	@objc(typesForExtensions:)
	static func types(forExtensions extensions: [String]) -> [UTType] {
		var seen = Set<UTType>()
		return extensions.flatMap { UTType.types(tag: $0, tagClass: .filenameExtension, conformingTo: nil) }
			.filter { seen.insert($0).inserted }
	}
}
