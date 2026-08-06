#pragma once

#include "tabulasonora/partial_parameters.hpp"
#include "tabulasonora/table_set.hpp"

#include <array>
#include <cstdint>
#include <limits>

namespace ts {

/// The segment-rate and segment-shape machine shared by the TVA, TVF and pitch envelopes.
///
/// An envelope is a chain of segments, each with a target level and a rate byte. The rate byte
/// selects a base duration from `g_rate_curve`, which two 8.8 multipliers then scale: one from
/// key-follow and one from velocity. A segment completes when a 16-bit phase accumulator wraps,
/// which makes the duration `0x10000 / (rate * speed) * 10 ms` at the 100 Hz control tick.
///
/// Everything here is integer arithmetic with deliberate 16-bit truncation. The control path of the
/// original is exclusively 16-bit fixed point, and several of these expressions depend on wrapping
/// or on truncation toward zero.
class EnvelopeMachine {
public:
    /// Neutral value of an 8.8 multiplier, meaning 1.0.
    static constexpr int unity_multiplier = 0x100;

    /// A rate-curve entry below this yields a zero-length segment.
    static constexpr int minimum_segment_ticks = 9;

    /// A hold that never expires: the envelope machine stays at its note-on state.
    static constexpr std::int64_t hold_forever = std::numeric_limits<std::int64_t>::max();

    /// Creates the machine over a loaded table set, which must outlive it.
    explicit EnvelopeMachine(const TableSet& tables);

    /// True when a segment interpolates linearly rather than through the fast-approach curve.
    ///
    /// Bit 7 of the rate byte is the segment *shape* flag, not part of the rate. The TVF and pitch
    /// envelopes hardcode linear; the TVA is the only one where the shape is data-driven.
    [[nodiscard]] static constexpr bool is_linear_segment(int rate_byte) noexcept
    {
        return (rate_byte & 0x80) != 0;
    }

    /// Converts a base rate byte and a 0x40-neutral modifier into an 8.8 rate multiplier.
    [[nodiscard]] int rate_scale(int base_rate, int modifier) const noexcept;

    /// Converts a level byte and a 0x40-neutral modifier into an 8.8 multiplier.
    [[nodiscard]] int level_scale(int level, int modifier) const noexcept;

    /// The duration of one envelope segment in milliseconds, or zero for an instant segment.
    ///
    /// `rate_byte`'s bit 7 is the shape flag and is masked off. `bias` is a part-level rate offset,
    /// zero when both patch rate parameters are neutral.
    [[nodiscard]] double segment_milliseconds(int rate_byte,
                                              int rate_multiplier,
                                              int velocity_multiplier = unity_multiplier,
                                              int bias = 0) const noexcept;

    /// Interpolates one segment's value at a normalised position, 0 to 1.
    ///
    /// The fast-approach curve must be *interpolated* between adjacent entries using the low byte
    /// of the phase. A bare 256-level lookup steps across the segment, and on a long decay those
    /// steps are amplitude discontinuities that inject a broadband noise floor — roughly 80 dB of
    /// sustain signal-to-noise against the engine's 96 dB.
    [[nodiscard]] double
    segment_curve(double position, double start, double target, bool linear) const noexcept;

    /// How long a partial's envelope machine stays held at its note-on state, in samples.
    ///
    /// Zero for a normal partial; a sample count for a delayed start; `hold_forever` for a
    /// one-shot.
    ///
    /// Partial block byte 0x00 arms a one-shot clock at note-on. While it runs, none of the voice's
    /// envelopes or LFOs advance — every control value stays where the note-on compute left it,
    /// which for an ordinary attack envelope is silence. When it fires, the machine simply starts.
    /// It fires once; it is a delayed start, not a tremolo. `0xff` is the one-shot form: held for
    /// the voice's whole life, so the sample plays at its note-on control levels (the `.o`
    /// variation tones), and note-off takes the fast fade instead of the release.
    /// The envelope hold clock's delay, in samples.
    ///
    /// `delay_bias` is the sum of the part's two hold-clock biases (`40 1x 38` and `40 4x 38`),
    /// each `0x40` for no change, so `0x80` is neutral. It is not a trim: the rate curve's first
    /// entry is zero, so a bias above neutral **arms** the clock on a partial whose own clock byte
    /// is zero, delaying every voice on the part by up to 24 seconds.
    [[nodiscard]] std::int64_t hold_samples(const PartialParameters& partial,
                                            int velocity,
                                            int delay_bias = 0x80) const noexcept;

private:
    std::span<const std::uint16_t> rate_curve_;
    std::span<const std::uint16_t> rate_out_;
    std::span<const std::uint8_t> scale_curve_;

    /// Only the first 256 entries of `g_env_shape` are used; the cache over-reads.
    /// 257, not 256: `segment_curve` interpolates from entry `k` up to `k + 1`, and `k` reaches 255.
    std::array<double, 257> shape_{};
};

} // namespace ts
