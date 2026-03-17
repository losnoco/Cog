import SwiftUI
#if canImport(WidgetKit)
import WidgetKit
#endif

/// Displays the app's icon in the current appearance (light, dark, clear, etc).
///
/// - note: The appearance of the icon will only reflect what's currently selected in System Settings,
/// it will not update in SwiftUI previews or if the app is overriding its own appearance.
@available(macOS 15.0, *)
public struct AppIconView: View {
    public var size: CGFloat

    public init(size: CGFloat = 128) {
        self.size = size
    }

    private var image: NSImage {
		let icon = NSImage.appIcon
        icon.size = NSSize(width: size, height: size)
        return icon
    }

    public var body: some View {
        Image(nsImage: image)
            .resizable()
            #if canImport(WidgetKit)
            /// Ensure icon renders correctly in tinted widgets.
            .widgetAccentedRenderingMode(.fullColor)
            #endif
            .frame(width: size, height: size)
    }
}

extension NSImage {
    /// The main bundle app icon in the current system style.
    static var appIcon: NSImage { NSWorkspace.shared.icon(forFile: Bundle.main.bundlePath) }
}

@objcMembers
@available(macOS 15.0, *)
public class SwiftUIIconBridge : NSObject {
	@objc
	public static func create() -> NSView {
		let iconView = AppIconView()
		return NSHostingView(rootView: iconView) as NSView
	}
}
