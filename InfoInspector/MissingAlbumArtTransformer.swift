import Foundation
import AppKit

@objc(MissingAlbumArtTransformer)
class MissingAlbumArtTransformer: ValueTransformer {
    override class func transformedValueClass() -> AnyClass {
        return NSImage.self
    }

    override class func allowsReverseTransformation() -> Bool {
        return false
    }

    // Convert from NSImage to NSImage
    override func transformedValue(_ value: Any?) -> Any? {
        if value == nil {
            return NSImage(named: "missingArt")
        }

        return value
    }
}
