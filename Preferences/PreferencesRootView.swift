import SwiftUI
import SwiftUIIntrospect

struct PreferencesRootView: View {
    @ObservedObject var model: PreferencesModel

    // @State owns the List selection so that user taps never touch objectWillChange,
    // which was the source of "Publishing changes from within view updates".
    @State private var selection: String?

    private var selectedPane: PreferencePaneDescriptor? {
        guard let t = selection else { return nil }
        return model.panes.first { $0.title == t }
    }

    // Updates @State and UserDefaults without touching the model's objectWillChange.
    private var listBinding: Binding<String?> {
        Binding(
            get: { selection },
            set: { newValue in
                selection = newValue
                UserDefaults.standard.set(newValue, forKey: "LastPreferencePane")
            }
        )
    }

    var body: some View {
        NavigationView {
            // --- Sidebar ---
            List(model.panes, id: \.title, selection: listBinding) { pane in
                HStack(spacing: 8) {
                    Image(nsImage: pane.icon)
                        .resizable()
                        .scaledToFit()
                        .frame(width: 18, height: 18)
                    Text(pane.localizedTitle)
                }
            }
            .listStyle(SidebarListStyle())
            .frame(minWidth: 180, idealWidth: 180, maxWidth: 180)
            // Sync external model changes (show/showPathSuggester/showRubberbandSettings)
            // into the local @State. objectWillChange fires after selectedTitle is set,
            // so model.selectedTitle already has the new value here.
            .onReceive(model.objectWillChange) { _ in
                selection = model.selectedTitle
            }

            // --- Detail ---
            if let pane = selectedPane {
                pane.body
                    .frame(maxWidth: .infinity, maxHeight: .infinity, alignment: .topLeading)
                    .id(pane.title)
            } else {
                Color.clear
            }
        }
        .navigationViewStyle(DoubleColumnNavigationViewStyle())
        .introspect(.navigationView(style: .columns), on: .macOS(.v10_15, .v11, .v12, .v13, .v14, .v15, .v26)) { splitView in
            guard let splitVC = splitView.delegate as? NSSplitViewController,
                  let sidebarItem = splitVC.splitViewItems.first else { return }
            sidebarItem.minimumThickness = 180
            sidebarItem.maximumThickness = 180
            sidebarItem.canCollapse = false
        }
        .onAppear { selection = model.selectedTitle }
    }
}

