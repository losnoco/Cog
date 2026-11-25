//
//  NSComparisonPredicate+CogPredicate.swift
//  Cog
//
//  Created by Matthew Grinshpun on 14/02/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

import Foundation

extension NSComparisonPredicate {
	// Ignore case and diacritics
	@objc(predicateForMdKey:withString:exactString:)
	class func predicate(forMdKey key: String, with aString: String, exactString: Bool) -> NSPredicate {
		// We don't want an exact string, so wrap it in wildcards
		let pattern = exactString ? aString : "*\(aString)*"

		return NSComparisonPredicate(leftExpression: NSExpression(forKeyPath: key),
		                             rightExpression: NSExpression(forConstantValue: pattern),
		                             modifier: .direct,
		                             type: .like,
		                             options: [.caseInsensitive, .diacriticInsensitive])
	}
}