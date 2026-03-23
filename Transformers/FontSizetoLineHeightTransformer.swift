//
//  FontSizetoLineHeightTransformer.swift
//  Cog
//
//  Created by Matthew Grinshpun on 18/02/08.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

import Foundation
import AppKit

@objc(FontSizetoLineHeightTransformer)
class FontSizetoLineHeightTransformer: ValueTransformer {
    override class func transformedValueClass() -> AnyClass {
        return NSNumber.self
    }

    override class func allowsReverseTransformation() -> Bool {
        return false
    }

    // Convert from font size to height in playlist view
    override func transformedValue(_ value: Any?) -> Any? {
        guard let fontSize = value as? NSNumber else {
            return nil
        }

        let font = NSFont.systemFont(ofSize: CGFloat(fontSize.floatValue))
        let layoutManager = NSLayoutManager()
        let lineHeight = layoutManager.defaultLineHeight(for: font)

        return NSNumber(value: lineHeight)
    }
}
