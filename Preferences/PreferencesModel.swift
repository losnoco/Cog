import Cocoa

final class PreferencesModel: ObservableObject {
    // Not @Published — objectWillChange is sent manually after the value is set,
    // so onReceive sees the new value immediately.
    private(set) var selectedTitle: String?
    let panes: [PreferencePaneDescriptor]

    init(panes: [PreferencePaneDescriptor]) {
        self.panes = panes
    }

    var selectedPane: PreferencePaneDescriptor? {
        guard let t = selectedTitle else { return nil }
        return panes.first { $0.title == t }
    }

    func select(named name: String) {
        guard panes.contains(where: { $0.title == name }) else { return }
        selectedTitle = name
        UserDefaults.standard.set(name, forKey: "LastPreferencePane")
        objectWillChange.send()
    }
}
