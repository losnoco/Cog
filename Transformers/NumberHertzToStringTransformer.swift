//
//  NumberHertzToStringTransformer.swift
//  Cog
//
//  Created by Christopher Snowhill on 6/29/22.
//

import Cocoa

@objc(NumberHertzToStringTransformer)
public final class NumberHertzToStringTransformer: ValueTransformer {
	fileprivate static var floatFormatter: NumberFormatter {
		let formatter = NumberFormatter()
		formatter.maximumFractionDigits = 2
		formatter.minimumFractionDigits = 0
		formatter.minimumIntegerDigits = 1
		formatter.roundingIncrement = 0.5
		formatter.usesGroupingSeparator = true
		formatter.groupingSeparator = ","
		formatter.groupingSize = 3
		return formatter
	}

	override public func transformedValue(_ value: Any?) -> Any? {
		if let value = value as? NSNumber {
			return "\(NumberHertzToStringTransformer.floatFormatter.string(from: value)!) Hz"
		}

		return nil
	}
}
