import Foundation

@objc(TotalTimeTransformer)
class TotalTimeTransformer: ValueTransformer {
    override class func transformedValueClass() -> AnyClass {
        return NSString.self
    }

    override class func allowsReverseTransformation() -> Bool {
        return false
    }

    override func transformedValue(_ value: Any?) -> Any? {
        guard let value = value else {
            return ""
        }

        if let string = value as? String {
            if !string.isEmpty {
                return String(format: NSLocalizedString("Total duration: %@", comment: "Total duration for status"), string)
            }
        }

        return ""
    }
}
