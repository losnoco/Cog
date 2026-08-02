#pragma once

#include <cstdint>

namespace ts {

/// The engine's shared pseudo-random generator.
///
/// The original keeps two 16-bit registers, seeded at engine reset to `0xEFA6` and `0x9C23`. The
/// first is a right-shifting Fibonacci LFSR whose new bit 15 is bit 5 XOR bit 15; the second shifts
/// left and inserts bit 9 when bit 2 is clear, or the complement of bit 13 when bit 2 is set. A
/// draw steps both and returns their XOR.
///
/// The sequence is therefore deterministic from engine reset; what polyphony changes is only the
/// order in which voices consume draws. The generator is shared by the random LFO waveforms, the
/// pitch start jitter, and the random detune redraw — so anything that consumes a draw it should
/// not shifts every later one.
class EngineNoise {
public:
    /// Draws the next 16-bit value.
    [[nodiscard]] std::uint16_t next() noexcept
    {
        auto step = static_cast<std::uint16_t>(shift_ >> 1);
        if (((shift_ & 0x20) != 0) != ((shift_ & 0x8000) != 0)) {
            step = static_cast<std::uint16_t>(step | 0x8000);
        }
        shift_ = step;

        const bool insert_one = (walk_ & 4) != 0 ? (walk_ & 0x2000) == 0 : (walk_ & 0x200) != 0;
        walk_ = static_cast<std::uint16_t>((walk_ << 1) | (insert_one ? 1 : 0));

        return static_cast<std::uint16_t>(walk_ ^ shift_);
    }

    /// Draws a random pan position, 0 to 127, on the same scale as CC#10.
    ///
    /// The engine takes the top seven bits of a draw. This is what GS random pan resolves to — a
    /// part whose CC#10 is zero, or a drum key whose pan NRPN is zero, is repositioned per note.
    [[nodiscard]] int next_pan() noexcept { return next() >> 9; }

    /// Returns the generator to the engine's reset state.
    void reset() noexcept
    {
        shift_ = 0xEFA6;
        walk_ = 0x9C23;
    }

private:
    std::uint16_t shift_ = 0xEFA6;
    std::uint16_t walk_ = 0x9C23;
};

} // namespace ts
