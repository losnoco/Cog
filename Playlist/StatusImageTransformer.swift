import Foundation
import AppKit

@objc(StatusImageTransformer)
class StatusImageTransformer: ValueTransformer {
    var playImage: NSImage?
    var queueImage: NSImage?
    var errorImage: NSImage?
    var stopAfterImage: NSImage?

    override class func transformedValueClass() -> AnyClass {
        return NSImage.self
    }

    override class func allowsReverseTransformation() -> Bool {
        return false
    }

    override init() {
        super.init()
        if #available(macOS 11.0, *) {
            self.playImage = NSImage(systemSymbolName: "play.fill", accessibilityDescription: NSLocalizedString("PlayingTrackTooltip", comment: ""))
            self.queueImage = NSImage(systemSymbolName: "plus", accessibilityDescription: NSLocalizedString("QueuedTrackTooltip", comment: ""))
            self.errorImage = NSImage(systemSymbolName: "nosign", accessibilityDescription: NSLocalizedString("ErrorTrackTooltip", comment: ""))
            self.stopAfterImage = NSImage(systemSymbolName: "stop.fill", accessibilityDescription: NSLocalizedString("StopAfterTrackTooltip", comment: ""))
        } else {
            self.playImage = NSImage(named: "playTemplate")
            self.queueImage = NSImage(named: "NSAddTemplate")
            self.errorImage = NSImage(named: "NSStopProgressTemplate")
            self.stopAfterImage = NSImage(named: "stopTemplate")
        }
    }

    // Convert from string to image
    override func transformedValue(_ value: Any?) -> Any? {
        if value == nil {
            return nil
        }

        if let stringValue = value as? String {
            switch stringValue {
            case "playing":
                return self.playImage
            case "queued":
                return self.queueImage
            case "error":
                return self.errorImage
            case "stopAfter":
                return self.stopAfterImage
            default:
                return nil
            }
        }

        return nil
    }
}
