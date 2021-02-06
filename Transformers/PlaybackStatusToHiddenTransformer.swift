//
//  PlaybackStatusToHiddenTransformer.swift
//  Cog
//
//  Created by Christopher Snowhill on 11/20/20.
//

import Foundation

class PlaybackStatusToHiddenTransformer : ValueTransformer {
    override class func transformedValueClass() -> AnyClass {
        return NSObject.self
    }
    
    override class func allowsReverseTransformation() -> Bool {
        return false
    }
    
    override func transformedValue(_ value: Any?) -> Any? {
        guard let intValue = value as? Int,
              let status = CogStatus(rawValue: intValue) else {
            return true;
        }
        switch status {
        case .stopped:
            return true
        case .paused,
             .playing,
             .stopping:
            return false
        @unknown default:
            return false;
        }
    }
}
