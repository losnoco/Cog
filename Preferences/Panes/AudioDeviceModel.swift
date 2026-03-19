import Foundation
import CoreAudio

final class AudioDeviceModel: ObservableObject {
    struct Device: Identifiable, Equatable {
        let id: Int      // AudioDeviceID stored as Int for UserDefaults compatibility
        let name: String
    }

    private var isActive = true

    @Published var devices: [Device] = []
    @Published var selectedDeviceID: Int = -1 {
        didSet { guard isActive else { return }; saveSelection() }
    }

    deinit { isActive = false }

    init() {
        loadDevices()
    }

    private var elementMain: AudioObjectPropertyElement {
        if #available(macOS 12.0, *) {
            return kAudioObjectPropertyElementMain
        } else {
            return kAudioObjectPropertyElementMaster  // deprecated but needed for <12
        }
    }

    func loadDevices() {
        var result: [Device] = [Device(id: -1, name: NSLocalizedString("System Default Device", comment: ""))]

        // Get all device IDs
        var addr = AudioObjectPropertyAddress(
            mSelector: kAudioHardwarePropertyDevices,
            mScope: kAudioObjectPropertyScopeGlobal,
            mElement: elementMain
        )
        var propSize: UInt32 = 0
        guard AudioObjectGetPropertyDataSize(
            AudioObjectID(kAudioObjectSystemObject), &addr, 0, nil, &propSize) == noErr else {
            devices = result; loadSelection(from: result); return
        }

        let count = Int(propSize) / MemoryLayout<AudioDeviceID>.size
        var deviceIDs = [AudioDeviceID](repeating: 0, count: count)
        guard AudioObjectGetPropertyData(
            AudioObjectID(kAudioObjectSystemObject), &addr, 0, nil, &propSize, &deviceIDs) == noErr else {
            devices = result; loadSelection(from: result); return
        }

        for deviceID in deviceIDs {
            guard let name = deviceName(deviceID) else { continue }
            guard hasOutputStreams(deviceID) else { continue }
            result.append(Device(id: Int(deviceID), name: name))
        }

        devices = result
        loadSelection(from: result)
    }

    private func deviceName(_ deviceID: AudioDeviceID) -> String? {
        var nameSize = UInt32(MemoryLayout<CFString?>.size)
        var nameAddr = AudioObjectPropertyAddress(
            mSelector: kAudioDevicePropertyDeviceNameCFString,
            mScope: kAudioDevicePropertyScopeOutput,
            mElement: elementMain
        )
        var cfName: CFString? = nil
        let status = withUnsafeMutablePointer(to: &cfName) { ptr in
            AudioObjectGetPropertyData(deviceID, &nameAddr, 0, nil, &nameSize,
                                       UnsafeMutableRawPointer(ptr))
        }
        guard status == noErr, let name = cfName as String? else { return nil }
        return name
    }

    private func hasOutputStreams(_ deviceID: AudioDeviceID) -> Bool {
        var bufSize: UInt32 = 0
        var addr = AudioObjectPropertyAddress(
            mSelector: kAudioDevicePropertyStreamConfiguration,
            mScope: kAudioDevicePropertyScopeOutput,
            mElement: elementMain
        )
        guard AudioObjectGetPropertyDataSize(deviceID, &addr, 0, nil, &bufSize) == noErr,
              bufSize >= MemoryLayout<UInt32>.size else { return false }

        let ptr = UnsafeMutableRawPointer.allocate(byteCount: Int(bufSize),
                                                   alignment: MemoryLayout<AudioBufferList>.alignment)
        defer { ptr.deallocate() }
        guard AudioObjectGetPropertyData(deviceID, &addr, 0, nil, &bufSize, ptr) == noErr else { return false }
        return ptr.bindMemory(to: AudioBufferList.self, capacity: 1).pointee.mNumberBuffers > 0
    }

    private func loadSelection(from deviceList: [Device]) {
        let stored = UserDefaults.standard.dictionary(forKey: "outputDevice")
        let storedID = (stored?["deviceID"] as? NSNumber)?.intValue ?? -1
        let storedName = stored?["name"] as? String ?? ""

        if storedID == -1 {
            selectedDeviceID = -1
            return
        }
        if deviceList.contains(where: { $0.id == storedID }) {
            selectedDeviceID = storedID
        } else if let match = deviceList.first(where: { $0.name == storedName }) {
            selectedDeviceID = match.id
            saveSelection(deviceID: match.id, name: match.name)
        } else {
            selectedDeviceID = -1
        }
    }

    private func saveSelection() {
        guard !devices.isEmpty else { return }
        let name = devices.first(where: { $0.id == selectedDeviceID })?.name ?? ""
        saveSelection(deviceID: selectedDeviceID, name: name)
    }

    private func saveSelection(deviceID: Int, name: String) {
        UserDefaults.standard.set(["name": name, "deviceID": deviceID], forKey: "outputDevice")
    }
}
