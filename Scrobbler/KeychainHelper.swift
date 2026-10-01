import Foundation
import Security

enum KeychainHelper {
    private static let service = "co.losno.Cog.lastfm"
    private static let account = "sessionKey"

    // MARK: - Last.FM session key

    static func save(sessionKey: String) -> Bool {
        save(sessionKey, service: service, account: account)
    }

    static func loadSessionKey() -> String? {
        load(service: service, account: account)
    }

    @discardableResult
    static func delete() -> Bool {
        delete(service: service, account: account)
    }

    // MARK: - Generic passwords

    static func save(_ secret: String, service: String, account: String) -> Bool {
        guard let data = secret.data(using: .utf8) else { return false }
        delete(service: service, account: account)
        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service,
            kSecAttrAccount as String: account,
            kSecValueData as String: data,
        ]
        return SecItemAdd(query as CFDictionary, nil) == errSecSuccess
    }

    static func load(service: String, account: String) -> String? {
        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service,
            kSecAttrAccount as String: account,
            kSecReturnData as String: true,
            kSecMatchLimit as String: kSecMatchLimitOne,
        ]
        var result: AnyObject?
        guard SecItemCopyMatching(query as CFDictionary, &result) == errSecSuccess,
              let data = result as? Data,
              let secret = String(data: data, encoding: .utf8) else {
            return nil
        }
        return secret
    }

    @discardableResult
    static func delete(service: String, account: String) -> Bool {
        let query: [String: Any] = [
            kSecClass as String: kSecClassGenericPassword,
            kSecAttrService as String: service,
            kSecAttrAccount as String: account,
        ]
        return SecItemDelete(query as CFDictionary) == errSecSuccess
    }
}
