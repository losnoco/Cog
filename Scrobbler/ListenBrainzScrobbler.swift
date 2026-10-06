//
//  ListenBrainzScrobbler.swift
//  Cog
//

import CogPlaylist
import CoreData
import Foundation

/// Sends listens to ListenBrainz alongside the Last.FM scrobbler.
///
/// Listens go through a queue in the Core Data store first, so a play made
/// offline, or while the server is down, is sent later rather than lost:
/// - transport errors, 429 and 5xx back off from 30 s, doubling up to 15 min;
/// - any other refusal drops that batch, since sending it again won't help;
/// - a rejected token is forgotten, but the queue is kept for the next one;
/// - listens older than two weeks are dropped unsent.
///
/// "Playing now" is not queued: only the latest one matters.
@MainActor
@objc(CogListenBrainzScrobbler)
final class ListenBrainzScrobbler: NSObject {
    @objc static let shared = ListenBrainzScrobbler()

    static let enabledKey = "enableListenBrainz"
    static let rootKey = "listenBrainzUrl"
    static let usernameKey = "listenBrainzUsername"
    static let tokenRejected = Notification.Name("CogListenBrainzTokenRejected")
    static let queueChanged = Notification.Name("CogListenBrainzQueueChanged")

    private static let keychainService = "co.losno.Cog.listenbrainz"
    private static let keychainAccount = "token"
    private static let maxAge: TimeInterval = 14 * 24 * 60 * 60
    private static let firstRetry: TimeInterval = 30
    private static let lastRetry: TimeInterval = 15 * 60

    private var nowPlaying: (track: AudioScrobblerTrack, start: Date)?
    private var lastScrobbledStart: Date?
    private var sending = false
    private var retryDelay = ListenBrainzScrobbler.firstRetry
    private var retryTimer: Timer?

    // MARK: - Account

    var token: String? {
        KeychainHelper.load(service: Self.keychainService, account: Self.keychainAccount)
    }

    @objc var isEnabled: Bool {
        UserDefaults.standard.bool(forKey: Self.enabledKey) && token != nil
    }

    private func api(token: String) -> ListenBrainzAPI {
        ListenBrainzAPI(root: UserDefaults.standard.string(forKey: Self.rootKey) ?? ListenBrainzAPI.defaultRoot, token: token)
    }

    /// Checks `token` with the server and keeps it. Returns the user name.
    func connect(token: String) async throws -> String {
        let name = try await api(token: token).validateToken()
        guard KeychainHelper.save(token, service: Self.keychainService, account: Self.keychainAccount) else {
            throw ListenBrainzError.transport(NSLocalizedString("Could not save credentials to Keychain.", comment: "Last.FM keychain save error"))
        }
        UserDefaults.standard.set(name, forKey: Self.usernameKey)
        UserDefaults.standard.set(true, forKey: Self.enabledKey)
        retryDelay = Self.firstRetry
        flush()
        return name
    }

    /// Forgets the token; listens still waiting stay for the next one.
    func disconnect() {
        KeychainHelper.delete(service: Self.keychainService, account: Self.keychainAccount)
        UserDefaults.standard.removeObject(forKey: Self.usernameKey)
    }

    // MARK: - Playback

    @objc func updateNowPlaying(_ track: AudioScrobblerTrack) {
        // Kept even while off, so a play is counted from its real start if
        // scrobbling is turned on halfway through.
        guard nowPlaying.map({ !$0.track.isEqual(track) }) ?? true else { return }
        nowPlaying = (track, Date())
        guard isEnabled, let token, let listen = ListenBrainzListen(track: track, listenedAt: nil) else { return }
        let api = api(token: token)
        Task {
            do {
                try await api.submit([listen], as: .playingNow)
            } catch ListenBrainzError.unauthorized {
                self.tokenWasRejected()
            } catch {
                // Not retried: by the time it could be, it is stale.
            }
        }
    }

    @objc func scrobble(_ track: AudioScrobblerTrack) {
        guard isEnabled, let nowPlaying, nowPlaying.track.isEqual(track), lastScrobbledStart != nowPlaying.start else { return }
        lastScrobbledStart = nowPlaying.start
        guard let listen = ListenBrainzListen(track: track, listenedAt: nowPlaying.start),
              let payload = try? ListenBrainzListen.encoder.encode(listen) else { return }
        let row = PendingListen(context: context)
        row.listenedAt = nowPlaying.start
        row.createdAt = Date()
        row.payload = payload
        save()
        NotificationCenter.default.post(name: Self.queueChanged, object: self)
        flush()
    }

    // MARK: - Queue

    private var context: NSManagedObjectContext {
        PlaylistController.sharedPersistentContainer().viewContext
    }

    private func save() {
        do {
            try context.save()
        } catch {
            NSLog("ListenBrainz: could not save the queue: %@", error.localizedDescription)
        }
    }

    var pendingCount: Int {
        (try? context.count(for: PendingListen.fetchRequest())) ?? 0
    }

    private func pruneOld() {
        let request = PendingListen.fetchRequest()
        request.predicate = NSPredicate(format: "listenedAt < %@", Date(timeIntervalSinceNow: -Self.maxAge) as NSDate)
        guard let old = try? context.fetch(request), !old.isEmpty else { return }
        old.forEach(context.delete)
        save()
    }

    /// Sends what is waiting, one batch at a time.
    @objc func flush() {
        guard !sending, retryTimer == nil, UserDefaults.standard.bool(forKey: Self.enabledKey), let token else { return }
        pruneOld()

        let request = PendingListen.fetchRequest()
        request.sortDescriptors = [NSSortDescriptor(key: "listenedAt", ascending: true)]
        request.fetchLimit = ListenBrainzAPI.maxBatch
        guard let rows = try? context.fetch(request), !rows.isEmpty else { return }

        var batch: [PendingListen] = []
        var listens: [ListenBrainzListen] = []
        for row in rows {
            if let payload = row.payload, let listen = try? ListenBrainzListen.decoder.decode(ListenBrainzListen.self, from: payload) {
                batch.append(row)
                listens.append(listen)
            } else {
                context.delete(row)
            }
        }
        guard !listens.isEmpty else {
            save()
            return
        }

        sending = true
        let api = api(token: token)
        Task {
            defer { self.sending = false }
            do {
                try await api.submit(listens, as: listens.count == 1 ? .single : .importListens)
                self.retryDelay = Self.firstRetry
                self.remove(batch)
                // Any more waiting go straight after.
                DispatchQueue.main.async { self.flush() }
            } catch let error as ListenBrainzError {
                switch error {
                case .unauthorized:
                    self.tokenWasRejected()
                case .rejected(let status, let message):
                    NSLog("ListenBrainz: dropped %d listens (%d: %@)", batch.count, status, message)
                    self.remove(batch)
                default:
                    self.retryLater()
                }
            } catch {
                self.retryLater()
            }
        }
    }

    private func remove(_ rows: [PendingListen]) {
        rows.forEach(context.delete)
        save()
        NotificationCenter.default.post(name: Self.queueChanged, object: self)
    }

    private func retryLater() {
        let delay = retryDelay
        retryDelay = min(retryDelay * 2, Self.lastRetry)
        retryTimer = Timer.scheduledTimer(withTimeInterval: delay, repeats: false) { _ in
            Task { @MainActor in
                let scrobbler = ListenBrainzScrobbler.shared
                scrobbler.retryTimer = nil
                scrobbler.flush()
            }
        }
    }

    private func tokenWasRejected() {
        disconnect()
        NotificationCenter.default.post(name: Self.tokenRejected, object: self)
    }
}
