//
//  NSArray+CogSort.swift
//  Cog
//
//  Created by Matthew Grinshpun on 16/02/08.
//  Copyright 2008 Matthew Leon Grinshpun. All rights reserved.
//

import Foundation

extension NSArray {
	// Hack to sort search results by artist
	@objc(compareFirstString:)
	func compareFirstString(_ anArray: [Any]) -> ComparisonResult {
		let mine = (self.firstObject as? String) ?? ""
		let theirs = (anArray.first as? String) ?? ""
		return mine.caseInsensitiveCompare(theirs)
	}
}