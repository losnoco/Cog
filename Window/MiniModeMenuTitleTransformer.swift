import Foundation

@objc(MiniModeMenuTitleTransformer)
class MiniModeMenuTitleTransformer: ValueTransformer {
    override class func transformedValueClass() -> AnyClass {
        return NSString.self
    }

    override class func allowsReverseTransformation() -> Bool {
        return false
    }

    override func transformedValue(_ value: Any?) -> Any? {
        if let number = value as? NSNumber {
            if number.boolValue {
                return NSLocalizedString("SwitchFromMiniPlayer", comment: "Menu item text for switching from mini player to full size window")
            } else {
                return NSLocalizedString("SwitchToMiniPlayer", comment: "Menu item text for switching from full size to the mini player")
            }
        }
        return nil
    }
}
