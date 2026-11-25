//
//  NSNumber+CogSort.swift
//  Cog
//
//  Created by Matthew Grinshpun on 16/02/08.
//  Copyright 2008 Matthew Leon Grinshpun. All rights reserved.
//

import Foundation

extension NSNumber {
	// Sometimes track numbers are CFNumbers
	@objc(compareTrackNumbers:)
	func compareTrackNumbers(_ aNumber: NSNumber) -> ComparisonResult {
		return compare(aNumber)
	}
}