//
//  NSString+CogSort.swift
//  Cog
//
//  Created by Matthew Grinshpun on 16/02/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

import Foundation

extension NSString {
	// Compares track numbers separated by slashes, ie "01/17", properly when
	// sorting incoming search results.
	@objc(compareTrackNumbers:)
	func compareTrackNumbers(_ aString: NSString) -> ComparisonResult {
		return compare(aString as String, options: .numeric)
	}
}

extension NSURL {
	@objc(compareTrackNumbers:)
	func compareTrackNumbers(_ aURL: NSURL) -> ComparisonResult {
		return ((absoluteString ?? "") as NSString).compareTrackNumbers((aURL.absoluteString ?? "") as NSString)
	}
}