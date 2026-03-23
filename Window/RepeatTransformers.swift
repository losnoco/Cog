import Foundation
import AppKit

@objc(RepeatModeTransformer)
class RepeatModeTransformer: ValueTransformer {
    private var repeatMode: RepeatMode
    
    @objc
    init(mode: RepeatMode) {
        self.repeatMode = mode
        super.init()
    }
    
    override class func transformedValueClass() -> AnyClass {
        return NSNumber.self
    }
    
    override class func allowsReverseTransformation() -> Bool {
        return true
    }
    
    // Convert from RepeatMode to BOOL
    override func transformedValue(_ value: Any?) -> Any? {
        guard let value = value else { return nil }
        
        let mode = RepeatMode(rawValue: (value as? NSNumber)?.intValue ?? 0) ?? .noRepeat
        return NSNumber(value: repeatMode == mode)
    }
    
    override func reverseTransformedValue(_ value: Any?) -> Any? {
        guard let value = value else { return nil }
        
        let enabled = (value as? NSNumber)?.boolValue ?? false
        if enabled {
            return NSNumber(value: repeatMode.rawValue)
        } else if repeatMode == .noRepeat {
            return NSNumber(value: RepeatMode.repeatAll.rawValue)
        } else {
            return NSNumber(value: RepeatMode.noRepeat.rawValue)
        }
    }
}

@objc(RepeatModeImageTransformer)
class RepeatModeImageTransformer: ValueTransformer {
    override class func transformedValueClass() -> AnyClass {
        return NSImage.self
    }
    
    override class func allowsReverseTransformation() -> Bool {
        return false
    }
    
    // Convert from string to RepeatMode
    override func transformedValue(_ value: Any?) -> Any? {
        guard let value = value else { return nil }
        
        let mode = RepeatMode(rawValue: (value as? NSNumber)?.intValue ?? 0) ?? .noRepeat
        
        switch mode {
        case .noRepeat:
            return NSImage(named: "repeatModeOffTemplate")
        case .repeatOne:
            return NSImage(named: "repeatModeOneTemplate")
        case .repeatAlbum:
            return NSImage(named: "repeatModeAlbumTemplate")
        case .repeatAll:
            return NSImage(named: "repeatModeAllTemplate")
        default:
            return nil
        }
    }
}
