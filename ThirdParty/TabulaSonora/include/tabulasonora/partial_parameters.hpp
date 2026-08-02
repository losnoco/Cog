#pragma once

#include <cstdint>
#include <span>
#include <utility>

namespace ts {

/// A zero-copy typed view over one 0x6e-byte partial parameter block inside a tone record.
///
/// A melodic tone holds two of these at `tone_base + 0x24`. The block carries everything the
/// synthesis chain needs: the multisample and key mapping, the pitch/TVF/TVA envelopes and their
/// key-follow rows, the two LFO depth sets, and the velocity window. Offsets are block-relative and
/// match the reverse-engineering notes directly.
///
/// **This is a non-owning view and it does not keep the tone table alive.** It is copied by value
/// all over the DSP, so the table it points into — normally the one owned by `TableSet` — has to
/// outlive every partial taken from it. A default-constructed view is "absent" rather than
/// dangling, which is what `is_present` reports.
class PartialParameters {
public:
    /// Bytes per partial block.
    static constexpr int stride = 0x6E;

    /// Value of `multisample` meaning the partial is absent.
    static constexpr int no_multisample = 0xFFFF;

    /// An absent partial.
    PartialParameters() = default;

    /// Creates a view over a partial block's first byte.
    explicit PartialParameters(const std::uint8_t* block) noexcept : block_(block) {}

    /// The block's raw bytes.
    [[nodiscard]] std::span<const std::uint8_t> raw() const noexcept
    {
        return {block_, static_cast<std::size_t>(stride)};
    }

    /// True when this slot holds a real partial.
    [[nodiscard]] bool is_present() const noexcept
    {
        return block_ != nullptr && multisample() != no_multisample;
    }

    /// Multisample index, or `no_multisample` when the slot is empty. Block +0x02.
    [[nodiscard]] int multisample() const noexcept { return u16(0x02); }

    /// Key centre — the origin the multisample's key zones are measured from. Block +0x04.
    [[nodiscard]] int key_center() const noexcept { return block_[0x04]; }

    /// Per-partial pan, 0x40 centre. Block +0x0f.
    [[nodiscard]] int pan() const noexcept { return block_[0x0F]; }

    /// Key transpose in whole semitones, 0x40 neutral. Block +0x10.
    [[nodiscard]] int key_transpose() const noexcept { return block_[0x10]; }

    /// Raw coarse-tune byte, 0x40 neutral. Block +0x11.
    [[nodiscard]] int coarse_tune_raw() const noexcept { return block_[0x11]; }

    /// Coarse tune in milli-semitones: `(raw - 0x40) * 10`. Block +0x11.
    [[nodiscard]] int coarse_tune_milli_semitones() const noexcept
    {
        return (coarse_tune_raw() - 0x40) * 10;
    }

    /// Pitch key-follow amount: 0x40 is 0%, 0x4a is 100%. Block +0x13.
    [[nodiscard]] int pitch_key_follow() const noexcept { return block_[0x13]; }

    /// TVF cutoff base, scaled by 0x100 into the runtime domain. Block +0x2f.
    [[nodiscard]] int cutoff_base() const noexcept { return block_[0x2F]; }

    /// TVF resonance, 0x40 neutral. Block +0x30.
    [[nodiscard]] int resonance() const noexcept { return block_[0x30]; }

    /// TVF filter type selector. Block +0x31.
    [[nodiscard]] int filter_type() const noexcept { return block_[0x31]; }

    /// Velocity response curve selector for the filter envelope's depth; low nibble only.
    /// Block +0x2e.
    ///
    /// Row 0 is the identity, so a patch that leaves this at zero sees raw MIDI velocity. Roughly a
    /// quarter of the library does not: Brass 1 selects row 1, which maps velocity 100 to 71.
    [[nodiscard]] int velocity_curve() const noexcept { return block_[0x2E] & 0x0F; }

    /// Velocity-crossfade curve selector; only the low nibble is used. Block +0x4e.
    [[nodiscard]] int velocity_crossfade_curve() const noexcept { return block_[0x4E] & 0x0F; }

    /// One edge of the velocity window. Block +0x4f.
    [[nodiscard]] int velocity_low() const noexcept { return block_[0x4F]; }

    /// The other edge of the velocity window. Block +0x51.
    [[nodiscard]] int velocity_high() const noexcept { return block_[0x51]; }

    /// Partial level at the `velocity_low` edge. Block +0x50.
    [[nodiscard]] int level_at_low() const noexcept { return block_[0x50]; }

    /// Partial level at the `velocity_high` edge. Block +0x52.
    [[nodiscard]] int level_at_high() const noexcept { return block_[0x52]; }

    /// TVA base level. Block +0x53.
    [[nodiscard]] int tva_base_level() const noexcept { return block_[0x53]; }

    /// The velocity window as an ordered range.
    ///
    /// The stored edges may be the other way round, which is how a partial fades in from the top of
    /// the window rather than the bottom.
    [[nodiscard]] std::pair<int, int> velocity_window() const noexcept
    {
        return velocity_low() <= velocity_high() ? std::pair{velocity_low(), velocity_high()}
                                                 : std::pair{velocity_high(), velocity_low()};
    }

    /// True when a velocity falls inside this partial's window.
    [[nodiscard]] bool accepts_velocity(int velocity) const noexcept
    {
        const auto [low, high] = velocity_window();
        return velocity >= low && velocity <= high;
    }

    /// The key handed to the multisample zone lookup, clamped to 0–127.
    ///
    /// The engine shifts the voice's key by the partial transpose before the multisample sees it,
    /// and the multisample then adds `0x40 - key_center` itself. Skipping the transpose selects a
    /// wave from the wrong zone, which the pitch chain then stretches to compensate — audible
    /// aliasing.
    [[nodiscard]] int transposed_key(int note) const noexcept;

    /// Whether two views refer to the same block.
    [[nodiscard]] friend bool operator==(const PartialParameters& left,
                                         const PartialParameters& right) noexcept
    {
        return left.block_ == right.block_;
    }

private:
    [[nodiscard]] int u16(int offset) const noexcept
    {
        return block_[offset] | (block_[offset + 1] << 8);
    }

    const std::uint8_t* block_ = nullptr;
};

} // namespace ts
