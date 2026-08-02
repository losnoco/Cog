#pragma once

#include "tabulasonora/partial_parameters.hpp"
#include "tabulasonora/wave_descriptor.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace ts {

/// One melodic tone record: an ASCII name, a master level, and up to two partial parameter blocks.
class Tone {
public:
    /// Bytes per tone record.
    static constexpr int stride = 0x100;

    /// Bytes of header before the first partial block.
    static constexpr int header_size = 0x24;

    /// Partial slots in a melodic tone record. The drum tone table has four.
    static constexpr int partial_slots = 2;

    /// Length of the ASCII name field.
    static constexpr int name_length = 12;

    /// Reads a tone record out of the tone table.
    ///
    /// The returned partials hold pointers into `tone_table`, which must outlive them.
    [[nodiscard]] static Tone read(std::span<const std::uint8_t> tone_table, int number);

    /// The tone number this record was read from.
    [[nodiscard]] int number() const noexcept { return number_; }

    /// The tone's ASCII name, trailing padding removed.
    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    /// Per-tone master level, header +0x0c.
    ///
    /// The engine copies it to `voice+0x167`, where it becomes the third attenuation in the TVA
    /// base-level chain.
    [[nodiscard]] int level() const noexcept { return level_; }

    /// The partials that are actually present, in slot order.
    ///
    /// Empty slots are dropped rather than represented, so an index into this list is not
    /// necessarily the slot index — that matters when correlating against a voice dump.
    [[nodiscard]] const std::vector<PartialParameters>& partials() const noexcept
    {
        return partials_;
    }

    /// Whether this record holds a real tone.
    ///
    /// Unused records read back as short or empty names, so a name of fewer than two characters is
    /// treated as absent — the same rule the reference resolver applies.
    [[nodiscard]] bool is_defined() const noexcept { return name_.size() >= 2; }

private:
    int number_ = 0;
    std::string name_;
    int level_ = 0;
    std::vector<PartialParameters> partials_;
};

/// One partial that sounds for a given note and velocity.
struct ResolvedPartial {
    /// Index within the tone's present partials.
    int partial_index = 0;
    /// Selected wave number.
    int wave = 0;
    /// The wave's ROM coordinates.
    WaveDescriptor descriptor;
};

/// The waves that sound for one tone at a given note and velocity.
struct ResolvedTone {
    /// The tone's name, or a placeholder when it is unassigned.
    std::string name;
    /// The sounding partials.
    std::vector<ResolvedPartial> partials;
};

} // namespace ts
