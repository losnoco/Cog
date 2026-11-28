//
//  NSString+FinderCompare.swift
//  Cog
//
//  Created by Pablo Gomez Basanta on 23/7/05.
//  From: http://neop.gbtopia.com/?p=27
//
//  Based on:
//  http://developer.apple.com/qa/qa2004/qa1159.html
//

import CoreServices
import Foundation

extension NSString {
	/// Finder's order: case, width and composition aside, digits as numbers.
	@objc(finderCompare:)
	func finderCompare(_ aString: String) -> ComparisonResult {
		let left = Array((self as String).utf16)
		let right = Array(aString.utf16)
		var result: Int32 = 0
		let options = UInt32(kUCCollateComposeInsensitiveMask | kUCCollateWidthInsensitiveMask | kUCCollateCaseInsensitiveMask |
		                     kUCCollateDigitsOverrideMask | kUCCollateDigitsAsNumberMask | kUCCollatePunctuationSignificantMask)
		_ = UCCompareTextDefault(options, left, left.count, right, right.count, nil, &result)
		// It orders by sign, not by -1 and 1 (it gives 2 and -2).
		return result < 0 ? .orderedAscending : result > 0 ? .orderedDescending : .orderedSame
	}
}
