import Foundation

@objc(StringToURLTransformer)
class StringToURLTransformer: ValueTransformer {
    override class func transformedValueClass() -> AnyClass {
        return NSURL.self
    }
    
    override class func allowsReverseTransformation() -> Bool {
        return true
    }
    
    override func transformedValue(_ value: Any?) -> Any? {
        guard let value = value else {
            return nil
        }
        
        if let string = value as? String {
            return NSURL(string: string)
        }
        
        return nil
    }
    
    override func reverseTransformedValue(_ value: Any?) -> Any? {
        guard let value = value else {
            return nil
        }
        
        if let url = value as? NSURL {
            return url.absoluteString
        }
        
        return nil
    }
}
