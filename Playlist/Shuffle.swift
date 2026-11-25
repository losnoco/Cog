//
//  Shuffle.swift
//  Cog
//
//  Created by Vincent Spader on 1/14/06.
//  Revised by Eric Hanneken on 2/14/08.
//  Copyright 2008 Vincent Spader. All rights reserved.
//

import Foundation

@objc(Shuffle)
class Shuffle: NSObject {
	@objc class func shuffleList(_ l: [Any]) -> NSMutableArray {
		// Pair each entry with a random long, sort the pairs by the randoms
		// ascending, and peel the entries off in their new, random order.
		let pairs = l.map { (Int.random(in: 0 ... Int(Int32.max)), $0) }
		let shuffledPairs = pairs.sorted { $0.0 < $1.0 }
		return NSMutableArray(array: shuffledPairs.map { $0.1 })
	}
}