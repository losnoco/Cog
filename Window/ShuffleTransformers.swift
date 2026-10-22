import Foundation
import AppKit

@objc(ShuffleImageTransformer)
class ShuffleImageTransformer: ValueTransformer {
    override class func transformedValueClass() -> AnyClass {
        return NSImage.self
    }

    override class func allowsReverseTransformation() -> Bool {
        return false
    }

    // Convert from ShuffleMode to NSImage
    override func transformedValue(_ value: Any?) -> Any? {
        guard let value = value else { return nil }

        let mode = ShuffleMode(rawValue: (value as? NSNumber)?.intValue ?? 0) ?? .off
        let shuffleMode = mode as ShuffleMode

        switch shuffleMode {
        case .off:
            return NSImage(systemSymbolName: "arrow.right", accessibilityDescription: nil)
        case .albums:
            return NSImage(named: "shuffle.album")
        case .all:
            return NSImage(systemSymbolName: "shuffle", accessibilityDescription: nil)
        default:
            return NSImage(systemSymbolName: "arrow.right", accessibilityDescription: nil)
        }
    }
}

@objc(ShuffleModeTransformer)
class ShuffleModeTransformer: ValueTransformer {
    private var shuffleMode: ShuffleMode

    @objc
    init(mode: ShuffleMode) {
        self.shuffleMode = mode
        super.init()
    }

    override class func transformedValueClass() -> AnyClass {
        return NSNumber.self
    }

    override class func allowsReverseTransformation() -> Bool {
        return true
    }

    // Convert from ShuffleMode to BOOL
    override func transformedValue(_ value: Any?) -> Any? {
        guard let value = value else { return nil }

        let mode = ShuffleMode(rawValue: (value as? NSNumber)?.intValue ?? 0) ?? .off

        if shuffleMode == mode {
            return NSNumber(value: true)
        }

        return NSNumber(value: false)
    }

    override func reverseTransformedValue(_ value: Any?) -> Any? {
        guard let value = value else { return nil }

        let enabled = (value as? NSNumber)?.boolValue ?? false
        if enabled {
            return NSNumber(value: shuffleMode.rawValue)
        } else if shuffleMode == .off {
            return NSNumber(value: ShuffleMode.all.rawValue)
        } else {
            return NSNumber(value: ShuffleMode.off.rawValue)
        }
    }
}
