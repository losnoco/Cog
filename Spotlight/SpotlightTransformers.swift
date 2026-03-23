import Foundation
import AppKit

// Static variable for PausingQueryTransformer
private var searchController: SpotlightWindowController?

@objc(PausingQueryTransformer)
class PausingQueryTransformer: ValueTransformer {
    private var oldResults: NSArray?

    override class func transformedValueClass() -> AnyClass {
        return NSArray.self
    }

    override class func allowsReverseTransformation() -> Bool {
        return false
    }

    @objc
    class func setSearchController(_ aSearchController: SpotlightWindowController?) {
        searchController = aSearchController
    }

    override func transformedValue(_ value: Any?) -> Any? {
        // Rather unintuitively, this piece of code eliminates the "flicker"
        // when searching for new results, which resulted from a pause when the
        // search query stops gathering and sends an empty results array through KVO.
        if (value as? NSArray)?.count ?? 0 > 0 || searchController?.query.isGathering ?? false {
            oldResults = value as? NSArray
        }
        return oldResults
    }
}

@objc(AuthorToArtistTransformer)
class AuthorToArtistTransformer: ValueTransformer {
    override class func transformedValueClass() -> AnyClass {
        return NSString.self
    }

    override class func allowsReverseTransformation() -> Bool {
        return false
    }

    override func transformedValue(_ value: Any?) -> Any? {
        return (value as? NSArray)?.firstObject
    }
}

@objc(PathToURLTransformer)
class PathToURLTransformer: ValueTransformer {
    override class func transformedValueClass() -> AnyClass {
        return NSURL.self
    }

    override class func allowsReverseTransformation() -> Bool {
        return true
    }

    // Convert from path to NSURL
    override func transformedValue(_ value: Any?) -> Any? {
        guard let path = value as? String else { return nil }
        return NSURL(fileURLWithPath: path)
    }

    // Convert from NSURL to path
    override func reverseTransformedValue(_ value: Any?) -> Any? {
        guard let url = value as? NSURL else { return nil }
        return url.path
    }
}

@objc(StringToSearchScopeTransformer)
class StringToSearchScopeTransformer: ValueTransformer {
    override class func transformedValueClass() -> AnyClass {
        return NSArray.self
    }

    override class func allowsReverseTransformation() -> Bool {
        return false
    }

    // Convert from URL string to Search Scope
    override func transformedValue(_ value: Any?) -> Any? {
        guard let urlString = value as? String else { return nil }
        let scope = NSURL(string: urlString)
        return [scope]
    }
}

@objc(NumberToStringTransformer)
class NumberToStringTransformer: ValueTransformer {
    override class func transformedValueClass() -> AnyClass {
        return NSString.self
    }

    override class func allowsReverseTransformation() -> Bool {
        return false
    }

    // Convert from NSNumber to NSString
    override func transformedValue(_ value: Any?) -> Any? {
        guard let number = value else { return nil }

        // If there's an NS/CFNumber hiding in here...
        if let stringNumber = number as? NSNumber {
            return stringNumber.stringValue
        }

        return number
    }
}
