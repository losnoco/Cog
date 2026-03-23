import Foundation

@objc(MaybeSecureValueDataTransformer)
class MaybeSecureValueDataTransformer: ValueTransformer {
    override class func transformedValueClass() -> AnyClass {
        return NSData.self
    }

    override class func allowsReverseTransformation() -> Bool {
        return true
    }

    override func transformedValue(_ value: Any?) -> Any? {
        if value == nil {
            return nil
        }

        if !(value is NSDictionary) {
            return nil
        }

        do {
            let ret = try NSKeyedArchiver.archivedData(withRootObject: value!, requiringSecureCoding: true)
            return ret
        } catch {
            NSLog("Error archiving secure data: \(error)")
            return nil
        }
    }

    override func reverseTransformedValue(_ value: Any?) -> Any? {
        if value == nil {
            return nil
        }

        if !(value is NSData) {
            return nil
        }

        if let value = value as? NSData {
            do {
                let unarchiver = try NSKeyedUnarchiver(forReadingFrom: value as Data)
                unarchiver.requiresSecureCoding = true
                let classes = [NSDictionary.self, NSArray.self, NSString.self, NSNumber.self, NSDate.self]
                let ret = unarchiver.decodeObject(of: classes, forKey: "root")
                if ret == nil {
                    NSLog("Error decoding dictionary from data")
                }
                return ret
            } catch {
                NSLog("Error initializing unarchiver for data: \(error)")
                return nil
            }
        }

        return nil
    }
}
