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
        switch value {
        case kCogStatusStopped as Int:
            return true
        case kCogStatusPaused as Int,
             kCogStatusPlaying as Int:
            return false
        case .none:
            return true
        case .some(_):
            return true
        }
    }
}
