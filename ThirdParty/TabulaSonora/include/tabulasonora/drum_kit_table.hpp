#pragma once

#include "tabulasonora/patch_directory.hpp"
#include "tabulasonora/rom_image.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace ts {

/// One drum key's settings within a kit.
struct DrumKey {
    /// Tone number. Drum sounds are ordinary melodic tones.
    int tone = 0;
    /// Kit level for this key. Applies downstream as `(level / 127)^2`.
    int level = 0;
    /// Coarse pitch, 60 being the tone's natural pitch.
    int pitch = 0;
    /// Mute/assign group; keys sharing a non-zero group cut each other off.
    int group = 0;
    /// Pan position, per key.
    int pan = 0;
};

/// The drum kit records: for each of 128 keys, which tone sounds and how it is levelled, tuned,
/// panned and grouped.
///
/// Three things about drums are easy to get wrong, and all three are settled by measurement:
///
///  * Drum sounds are ordinary *melodic* tones. They do not use the separate drum tone table
///    (stride 0x1e8), which is why that table's remaining unreversed state does not block GM kits.
///  * They do not resolve through the three-level melodic lookup; a program change on the drum part
///    selects a kit through its own pair of lookups.
///  * The note does not transpose the sample. The kit's coarse-pitch plane supplies the key
///    instead, pivoting on 60, and the tone's own key-follow decides what a step of it is worth.
class DrumKitTable {
public:
    /// Bytes per kit record.
    static constexpr int kit_stride = 0x50C;

    /// Keys per kit.
    static constexpr int key_count = 128;

    /// Rows in the drum program map.
    ///
    /// Six, not the two this read for a long time. Each row maps a program to a kit, and program 0
    /// resolves to kit 0, 38, 47, 59, 68 and 79 across rows 0 to 5. Reading two rows put every kit
    /// from 47 up out of reach entirely.
    ///
    /// Measured against the DLL: driven with the SC-55 tone map on program 0 of the drum part, the
    /// engine resolves a tone for all 61 sounding keys that *kit 59* reproduces exactly, 61 of 61 —
    /// and kit 59 is only reachable through row 3. The best any kit the two-row read could see
    /// managed was 30 of 61.
    static constexpr int map_row_count = 6;

    /// File offset of the table mapping a level-two index to a kit record index.
    ///
    /// Unlike the other three drum regions this one is not recorded in the manifest, so it is
    /// pinned here. It is `DAT_1819f31b0` in the decompile.
    static constexpr std::int64_t kit_index_offset = 0x19F21B0;

    /// Creates the table by reading the drum regions out of the ROM image.
    explicit DrumKitTable(const RomImage& rom);

    /// Number of kit records reachable through the program map.
    [[nodiscard]] int kit_count() const noexcept { return kit_count_; }

    /// The drum map row a vintage's tone map selects, or nothing where no measurement pins one.
    ///
    /// The module takes this from the part's internal bank code, and that translation is not
    /// reversed — but which row each vintage ends up on is observable. Driving the DLL with each
    /// tone map in turn and reading back the tone it resolves for program 0 on the drum part:
    /// SC-55 selects row 3 (kit 59), SC-88 row 2 (kit 47), SC-88Pro row 1 and SC-8820 row 0.
    ///
    /// Rows 4 and 5 exist and no vintage selects them; a host that wants them sets the row itself.
    [[nodiscard]] static std::optional<int> row_for_map(ToneMap map) noexcept;

    /// Maps an internal bank code to a drum map row; standard GM/GS drum parts use 0x04.
    [[nodiscard]] int map_row(int internal_bank) const noexcept;

    /// Resolves a drum program to its kit record index, or nothing for an undefined program.
    ///
    /// The engine leaves the kit unchanged rather than silencing the part, which is why this is an
    /// absence rather than a zero.
    [[nodiscard]] std::optional<int> kit_for_program(int program, int row = 0) const noexcept;

    /// Reads one key's settings from a kit; kit 0 is GM Standard.
    ///
    /// Throws `std::out_of_range` if the note is outside 0–127.
    [[nodiscard]] DrumKey key(int note, int kit = 0) const;

    /// The playback ratio the kit's coarse-pitch plane implies.
    ///
    /// **Only correct for a tone that key-follows at 50%**, which is what the SC-88 and SC-8820
    /// standard kits use and what this constant was measured from. The general rule is per-tone:
    /// the SC-55 kits and the Electronic family follow at 100% and move a whole semitone per unit,
    /// so they come out half as far under this reading. The engine paths use
    /// `PitchChain::drum_pitch_milli_semitones`; this remains for callers that want the old scalar.
    [[nodiscard]] static double coarse_pitch_ratio(int pitch) noexcept;

    /// The gain the kit level contributes, as amplitude.
    ///
    /// The kit level is *not* part of the per-voice amplitude — that matches the engine exactly
    /// without it. It enters downstream in the part-volume computation, where the result is
    /// squared, so it acts as `(level / 127)^2`.
    [[nodiscard]] static constexpr double level_gain(int level) noexcept
    {
        return (level / 127.0) * (level / 127.0);
    }

private:
    // Plane offsets within a kit record.
    static constexpr int tone_plane = 0x000; // 128 x u16
    static constexpr int level_plane = 0x100;
    static constexpr int pitch_plane = 0x180;
    static constexpr int group_plane = 0x200;
    static constexpr int pan_plane = 0x280;

    std::int64_t kit_base_ = 0;
    std::vector<std::uint8_t> bank_row_;
    std::vector<std::uint8_t> program_map_;
    std::array<std::uint16_t, 256> kit_index_{};
    std::vector<std::uint8_t> kits_;
    int kit_count_ = 0;
};

} // namespace ts
