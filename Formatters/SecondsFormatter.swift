/*
 *  SecondsFormatter.swift
 *  Cog
 *
 *  Copyright (C) 2006 - 2007 Stephen F. Booth <me@sbooth.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

import Foundation

@objc(SecondsFormatter)
class SecondsFormatter: Formatter {
	override func string(for obj: Any?) -> String? {
		guard let number = obj as? NSNumber, !number.doubleValue.isNaN else {
			return ""
		}

		let value = UInt32(truncatingIfNeeded: Int64(number.doubleValue))

		let seconds = value % 60
		var minutes = value / 60
		var hours = minutes / 60
		minutes %= 60
		let days = hours / 24
		hours %= 24

		if days > 0 {
			return String(format: "%u:%02u:%02u:%02u", days, hours, minutes, seconds)
		} else if hours > 0 {
			return String(format: "%u:%02u:%02u", hours, minutes, seconds)
		} else if minutes > 0 {
			return String(format: "%u:%02u", minutes, seconds)
		} else {
			return String(format: "0:%02u", seconds)
		}
	}

	override func getObjectValue(_ obj: AutoreleasingUnsafeMutablePointer<AnyObject?>?, for string: String, errorDescription error: AutoreleasingUnsafeMutablePointer<NSString?>?) -> Bool {
		let scanner = Scanner(string: string)
		var result = false
		var seconds: UInt32 = 0

		while !scanner.isAtEnd {
			// Grab a value
			if let value = scanner.scanInt() {
				seconds = seconds &* 60 &+ UInt32(truncatingIfNeeded: value)
				result = true
			}

			// Grab the separator, if present
			_ = scanner.scanString(":")
		}

		if result {
			obj?.pointee = NSNumber(value: seconds)
		} else {
			error?.pointee = "Couldn't convert value to seconds"
		}

		return result
	}

	override func attributedString(for obj: Any, withDefaultAttributes attrs: [NSAttributedString.Key: Any]? = nil) -> NSAttributedString? {
		return NSAttributedString(string: string(for: obj) ?? "", attributes: attrs)
	}
}

@objc(SecondsFractionFormatter)
class SecondsFractionFormatter: Formatter {
	override func string(for obj: Any?) -> String? {
		guard let number = obj as? NSNumber, !number.doubleValue.isNaN else {
			return ""
		}

		let value = number.doubleValue

		let seconds = Float(fmod(value, 60.0))
		var minutes = UInt32(truncatingIfNeeded: Int64(floor(value / 60.0)))
		var hours = minutes / 60
		minutes %= 60
		let days = hours / 24
		hours %= 24

		if days > 0 {
			return String(format: "%u:%02u:%02u:%06.3f", days, hours, minutes, seconds)
		} else if hours > 0 {
			return String(format: "%u:%02u:%06.3f", hours, minutes, seconds)
		} else if minutes > 0 {
			return String(format: "%u:%06.3f", minutes, seconds)
		} else {
			return String(format: "0:%06.3f", seconds)
		}
	}

	override func getObjectValue(_ obj: AutoreleasingUnsafeMutablePointer<AnyObject?>?, for string: String, errorDescription error: AutoreleasingUnsafeMutablePointer<NSString?>?) -> Bool {
		let scanner = Scanner(string: string)
		var result = false
		var seconds = 0.0

		while !scanner.isAtEnd {
			// Grab a value
			if let value = scanner.scanDouble() {
				seconds = seconds * 60.0 + value
				result = true
			}

			// Grab the separator, if present
			_ = scanner.scanString(":")
		}

		if result {
			obj?.pointee = NSNumber(value: seconds)
		} else {
			error?.pointee = "Couldn't convert value to seconds"
		}

		return result
	}

	override func attributedString(for obj: Any, withDefaultAttributes attrs: [NSAttributedString.Key: Any]? = nil) -> NSAttributedString? {
		return NSAttributedString(string: string(for: obj) ?? "", attributes: attrs)
	}
}