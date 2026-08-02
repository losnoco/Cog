#pragma once

#include "tabulasonora/table_set.hpp"
#include "tabulasonora/tone.hpp"
#include "tabulasonora/wave_descriptor.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ts {

/// Which vintage's tone map a program change resolves against.
enum class ToneMap {
    sc55 = 1,
    sc88 = 2,
    sc88pro = 3,
    sc8820 = 4,
};

/// One key zone of a tone, expressed in MIDI note numbers.
struct ToneZone {
    int partial_index = 0;
    int velocity_low = 0;
    int velocity_high = 0;
    int key_low = 0;
    int key_high = 0;
    int wave = 0;
    WaveDescriptor descriptor;
};

/// A (map, bank, program) triple that resolves through the same three-level lookup.
struct ProgramReference {
    int map = 0;
    int bank = 0;
    int program = 0;
};

/// An entry of the alternate-articulation table.
struct AlternateEntry {
    /// The entry's ASCII name.
    std::string name;
    /// Inter-note timing threshold that gates the alternate.
    int threshold = 0;
    /// The tone reference that normally sounds.
    ProgramReference primary;
    /// The alternate articulation, reachable only in mono/solo mode.
    ProgramReference alternate;
};

/// The static patch directory: resolves a program change and a played note to the waves that
/// sound, entirely from ROM tables with no engine involved.
///
/// The chain is `(map, bank, program) -> tone# -> partial -> multisample -> key/velocity zone ->
/// wave# -> ROM coordinates`. The program lookup is three levels deep and its result carries three
/// distinct tone spaces, which is the part most likely to be got wrong.
class PatchDirectory {
public:
    /// Highest tone number in the melodic space, exclusive.
    static constexpr int melodic_space_end = 0x4000;

    /// First tone number in the drum space.
    static constexpr int drum_space_start = 0x4000;

    /// First tone number in the alternate-articulation space.
    static constexpr int alternate_space_start = 0x6000;

    /// Tone numbers at or above this are reachable only indirectly.
    static constexpr int indirect_only_flag = 0x8000;

    /// Bytes per multisample record.
    static constexpr int multisample_stride = 0x8C;

    /// Bytes per alternate-articulation record.
    static constexpr int alternate_stride = 0x18;

    /// Value meaning "unassigned" in the third lookup level.
    static constexpr int unassigned = 0xFFFF;

    /// Creates a directory over a loaded table set, which must outlive it.
    explicit PatchDirectory(const TableSet& tables);

    /// Number of tone records in the melodic table.
    [[nodiscard]] int tone_count() const noexcept { return tone_count_; }

    /// Number of multisample records.
    [[nodiscard]] int multisample_count() const noexcept { return multisample_count_; }

    /// Number of wave descriptors.
    [[nodiscard]] int wave_count() const noexcept { return wave_count_; }

    /// Number of alternate-articulation records.
    [[nodiscard]] int alternate_count() const noexcept { return alternate_count_; }

    /// Reads a wave descriptor, or nothing if the number is out of range.
    [[nodiscard]] std::optional<WaveDescriptor> wave(int wave_number) const;

    /// Reads a tone record, or nothing if the number is out of range.
    [[nodiscard]] std::optional<Tone> tone(int tone_number) const;

    /// Reads a tone's master level from its header.
    [[nodiscard]] int tone_level(int tone_number) const;

    /// Whether a tone responds to half-damper (continuous CC64).
    ///
    /// Tone header byte 0x0d bit 2, copied to the part on tone select. Set on exactly 57 of the
    /// 2363 tones — the piano family — matching the hardware, where half-pedal is a piano feature.
    /// Every other tone's pedal is quantised to fully up or fully down.
    [[nodiscard]] bool half_damper(int tone_number) const;

    /// Reads a partial block by its *slot* rather than its position among the present partials.
    ///
    /// `Tone::partials` drops empty slots, so its indices are not slot numbers. Anything
    /// correlating against the reference or a voice dump needs the slot.
    [[nodiscard]] PartialParameters partial_by_slot(int tone_number, int slot) const;

    /// Selects the wave a multisample yields for a transposed key, or nothing when silent.
    ///
    /// An empty *bottom* zone means the note is below the multisample's sampled range, and the
    /// partial is silent — it must not fall through to the neighbouring wave. The engine does
    /// statically down-extrapolate, but the resulting voice is terminated, so the audible result is
    /// silence. Slap Bass 2 below note 40 is the case that proved it.
    [[nodiscard]] std::optional<int> multisample_wave(int multisample, int transposed_key) const;

    /// Reads the per-key-zone level, the second attenuation in the TVA base-level chain.
    [[nodiscard]] int zone_level(int multisample, int key, int key_center) const;

    /// Lists a multisample's key zones in transposed-key space, low to high.
    [[nodiscard]] std::vector<MultisampleZone> multisample_zones(int multisample) const;

    /// Builds the full static zone map for a tone, with key bounds converted back to MIDI notes.
    ///
    /// Returns the tone's name, or nothing when the tone is undefined. This is the static map, so
    /// it applies only the key centre — not the partial's key transpose, which belongs to the
    /// played-note path in `resolve`.
    [[nodiscard]] std::optional<std::string> tone_zones(int tone_number,
                                                        std::vector<ToneZone>& zones) const;

    /// The raw unsigned word the three-level program lookup yields.
    ///
    /// It must be read as *unsigned*. Reading it signed and bailing on the sign bit is what once
    /// made four string patches look unassigned — the 0x8000 bit marks a tone reachable only
    /// through an alternate-articulation entry, not a missing one.
    [[nodiscard]] std::optional<int> lut3_raw(int program, ToneMap map, int bank) const;

    /// The three-level lookup with the GS capital-tone fallback applied.
    ///
    /// A bank select picks a variation. When the selected variation has no entry for the program, a
    /// Sound Canvas does not fall silent — it sounds the capital tone at bank 0 instead.
    /// Reproducing that is what lets a file play a note on a variation bank the ROM never filled
    /// in, which is exactly what one honky-tonk part in passport.mid does: it selects bank 5, whose
    /// program-3 slot is empty, and every note would otherwise be dropped.
    [[nodiscard]] std::optional<int> lut3_resolved(int program, ToneMap map, int bank) const;

    /// Decodes an entry of the alternate-articulation table, indexed by `tone# - 0x6000`.
    [[nodiscard]] std::optional<AlternateEntry> alternate(int index) const;

    /// Resolves a program change to the tone numbers that sound for an ordinary poly note.
    ///
    /// Returned as a list because the caller walks it, but an alternate-articulation entry's two
    /// references are *not* layers played together: the second is a conditional articulation that
    /// only the mono/solo path can reach.
    [[nodiscard]] std::vector<int> program_tones(int program, ToneMap map, int bank) const;

    /// The mono/solo alternate-articulation tone for a program, or -1.
    ///
    /// Not rendered: reproducing it needs the part's mono-mode flag and the inter-note timing test.
    [[nodiscard]] int alternate_tone(int program, ToneMap map, int bank) const;

    /// Resolves a program change to a single tone number, or -1 when unassigned.
    [[nodiscard]] int program_to_tone(int program, ToneMap map, int bank) const;

    /// Resolves a played note against a tone.
    [[nodiscard]] ResolvedTone resolve(int tone_number, int note, int velocity) const;

    /// Resolves a MIDI program change and played note in one step.
    [[nodiscard]] ResolvedTone
    resolve_midi(int program, int note, int velocity, ToneMap map, int bank) const;

private:
    [[nodiscard]] int dereference(const ProgramReference& reference) const;

    const TableSet* tables_;
    std::span<const std::uint8_t> tone_;
    std::span<const std::uint8_t> multisample_;
    std::span<const std::uint8_t> wavedesc_;
    std::span<const std::uint8_t> layered_;

    int tone_count_ = 0;
    int multisample_count_ = 0;
    int wave_count_ = 0;
    int alternate_count_ = 0;
};

} // namespace ts
