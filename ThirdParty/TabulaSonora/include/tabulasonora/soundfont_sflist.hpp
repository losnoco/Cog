#pragma once

#include "tabulasonora/drum_kit_table.hpp"
#include "tabulasonora/patch_directory.hpp"

#include <string>

namespace ts::sf2 {

/// Knobs the `.sflist.json` generator exposes.
struct SflistOptions {
    /// The bank file name written into the list, resolved relative to the list itself.
    std::string file_name = "sound-canvas.sf2";

    /// Whether to emit the GS capital-tone fallback as explicit mappings.
    ///
    /// A Sound Canvas that is asked for a program its selected variation bank does not fill sounds
    /// the bank-0 capital tone instead. `sflist` has no fallback rule of its own, so reproducing
    /// that means emitting a mapping for every (bank, program) the module would resolve, not only
    /// the ones the variation bank fills itself. Turning this off shrinks the file by roughly an
    /// order of magnitude and makes those programs silent, which is what one honky-tonk part in
    /// `passport.mid` does when it selects bank 5 for a program that bank never filled.
    bool capital_fallback = true;

    /// Whether to include the drum kits.
    bool drums = true;
};

/// How a generated list came out.
struct SflistReport {
    int melodic_mappings = 0;
    int fallback_mappings = 0;
    int drum_mappings = 0;
    int banks = 0;

    [[nodiscard]] int total() const noexcept { return melodic_mappings + drum_mappings; }
};

/// Builds the `.sflist.json` that remaps one vintage's banks onto the shared ROM-aligned bank.
///
/// The direction is the one `ss_filtered_bank_build_one` implements and it is the opposite of what
/// the names suggest at a glance: **`source` selects presets inside the bank file** and
/// **`destination` is where MIDI addresses them**. So `source` carries the ROM-aligned slot the
/// exporter wrote — bank `(tone >> 7) << 8`, program `tone & 0x7f` — and `destination` carries the
/// vintage's own bank and program.
///
/// The two bank layouts are not the same shape, which is the other thing to get right:
///
///  * **GS** puts the variation in the bank **MSB**, and selects the vintage itself with bank LSB
///    1–4. Choosing the vintage is what having five separate lists is *for*, so the destinations
///    here sit at LSB 0 and let the MSB carry the variation as an ordinary GS file expects.
///  * **XG** puts the variation in the bank **LSB**, with the MSB naming a column rather than a
///    variation — MSB 64 is the SFX voice bank, which the module reaches by substituting lookup
///    bank 0x7d, and MSB 126/127 are drums.
///
/// Drum presets keep their percussion flag across the remap — `ss_filtered_bank_build_one` copies
/// `is_gm_gs_drum` and rewrites only the MSB and LSB — so a drum destination needs no flag of its
/// own and lands on the standard percussion bank.
[[nodiscard]] std::string build_sflist(const PatchDirectory& directory,
                                       const DrumKitTable& kits,
                                       ToneMap map,
                                       const SflistOptions& options,
                                       SflistReport& report);

} // namespace ts::sf2
