//
//  PlaybackStatusToHiddenTransformer.swift
//  Cog
//
//  Created by Christopher Snowhill on 11/20/20.
//

import Foundation

class PlaybackStatusToHiddenTransformer : ValueTransformer {
    override class func transformedValueClass() -> AnyClass {
        return NSNumber.self
    }

    override class func allowsReverseTransformation() -> Bool {
        return false
    }

    override func transformedValue(_ value: Any?) -> Any? {
        let titleShown = UserDefaults.standard.bool(forKey: "toolbarStyleFull");
        if titleShown {
            return NSNumber(booleanLiteral: true)
        }

        guard let intValue = value as? Int,
              let status = CogStatus(rawValue: intValue) else {
            return NSNumber(booleanLiteral: true)
        }
        switch status {
        case .stopped:
            return NSNumber(booleanLiteral: true)
        case .paused,
             .playing,
             .stopping:
            return NSNumber(booleanLiteral: false)
        @unknown default:
            return NSNumber(booleanLiteral: false)
        }
    }
}
