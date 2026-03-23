import Foundation

@objc(RubberbandEngineR3Transformer)
class RubberbandEngineR3Transformer: ValueTransformer {
    override class func transformedValueClass() -> AnyClass {
        return NSNumber.self
    }
    
    override class func allowsReverseTransformation() -> Bool {
        return false
    }
    
    override func transformedValue(_ value: Any?) -> Any? {
        if value == nil {
            return NSNumber(value: true)
        }
        
        if let stringValue = value as? String {
            if stringValue == "disabled" || stringValue == "signalsmith" || stringValue == "finer" {
                return NSNumber(value: false)
            }
        }
        
        return NSNumber(value: true)
    }
}

@objc(RubberbandEngineEnabledTransformer)
class RubberbandEngineEnabledTransformer: ValueTransformer {
    override class func transformedValueClass() -> AnyClass {
        return NSNumber.self
    }
    
    override class func allowsReverseTransformation() -> Bool {
        return false
    }
    
    override func transformedValue(_ value: Any?) -> Any? {
        if value == nil {
            return NSNumber(value: true)
        }
        
        if let stringValue = value as? String {
            if stringValue == "disabled" || stringValue == "signalsmith" {
                return NSNumber(value: false)
            }
        }
        
        return NSNumber(value: true)
    }
}

@objc(RubberbandEngineHiddenTransformer)
class RubberbandEngineHiddenTransformer: ValueTransformer {
    override class func transformedValueClass() -> AnyClass {
        return NSNumber.self
    }
    
    override class func allowsReverseTransformation() -> Bool {
        return false
    }
    
    override func transformedValue(_ value: Any?) -> Any? {
        if value == nil {
            return NSNumber(value: true)
        }
        
        if let stringValue = value as? String {
            if stringValue == "disabled" {
                return NSNumber(value: false)
            }
        }
        
        return NSNumber(value: true)
    }
}
