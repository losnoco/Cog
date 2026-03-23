import Cocoa

@objc(ColorToValueTransformer)
class ColorToValueTransformer: ValueTransformer {
    override class func transformedValueClass() -> AnyClass {
        return NSColor.self
    }

    override class func allowsReverseTransformation() -> Bool {
        return true
    }

    override func transformedValue(_ value: Any?) -> Any? {
        guard let value = value else {
            return NSColor(red: 0, green: 0, blue: 0, alpha: 1.0)
        }

        if let value = value as? NSData {
            do {
                let unarchiver = try NSKeyedUnarchiver(forReadingFrom: value as Data)
                unarchiver.requiresSecureCoding = true
                let classes = [NSColor.self]
                let ret = unarchiver.decodeObject(of: classes, forKey: "root")
                return ret ?? NSColor(red: 0, green: 0, blue: 0, alpha: 1.0)
            } catch {
                NSLog("Error unarchiving color: \(error)")
                return NSColor(red: 0, green: 0, blue: 0, alpha: 1.0)
            }
        }

        return NSColor(red: 0, green: 0, blue: 0, alpha: 1.0)
    }

    override func reverseTransformedValue(_ value: Any?) -> Any? {
        guard let value = value else {
            return nil
        }

        let data = try? NSKeyedArchiver.archivedData(withRootObject: value, requiringSecureCoding: true)

        return data
    }
}
