import Foundation

@objc(ToggleQueueTitleTransformer)
class ToggleQueueTitleTransformer: ValueTransformer {
    override class func transformedValueClass() -> AnyClass {
        return NSNumber.self
    }

    override class func allowsReverseTransformation() -> Bool {
        return false
    }

    // Convert from NSNumber to NSString
    override func transformedValue(_ value: Any?) -> Any? {
        if value == nil {
            return nil
        }

        let queued = (value as? NSNumber)?.boolValue ?? false

        if queued {
            return NSLocalizedString("RemoveFromQueue", comment: "Context menu item name for removing a track from the current queue")
        } else {
            return NSLocalizedString("AddToQueue", comment: "Context menu item name for adding a track to the current queue")
        }
    }
}
