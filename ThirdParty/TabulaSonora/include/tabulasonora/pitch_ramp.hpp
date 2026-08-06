#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>

namespace ts {

/// The per-voice pitch ramp that glides the sampler's read rate across a control block.
///
/// The engine does not step a voice's pitch once per 10 ms tick. `voice_pitch_block_init` records
/// the pitch *entering* the block at `voice+0xb8` and the pitch *leaving* it at `+0xbc`, and
/// `ramp_env_step_eval` then walks between them while the block renders, writing a fresh sampler
/// increment every **8 samples**. Without that glide a voice whose pitch moves far inside one block
/// reads its wave from a measurably different place forever after: the sub-sample residue the glide
/// leaves behind is frozen into the sampler's phase, which selects a different interpolator row for
/// the rest of the note. Drum tone 1946 is the case that exposed it — its pitch envelope drops
/// 6.671 semitones inside block 0, the engine gains 1.693481 samples across that block, and the
/// resulting phase of 45448 costs about 1.1 dB and a visibly brighter top octave.
///
/// **The ramp domain.** Both endpoints are formed as `0x38000 + (milli_semitones << 9) / 375`, so
/// `0x38000` is unity and one octave is exactly 16384 units — the same octave size `g_ramp_exp_tbl`
/// uses for the TVF's `f`, which is why both stages share the table.
class PitchRamp {
public:
    /// Samples one increment serves; the engine writes four of these per 32-sample chunk.
    static constexpr int samples_per_slot = 8;

    /// The ramp domain's value for unity rate.
    static constexpr int unity = 0x38000;

    /// Units per octave, and the divisor that puts milli-semitones into them.
    static constexpr int units_per_octave = 0x4000;
    static constexpr int milli_semitone_divisor = 375;

    /// Largest value the exponential decode accepts.
    static constexpr int domain_max = 0x3FFFF;

    /// Creates a ramp over the shared exponential table, which must outlive it.
    explicit PitchRamp(std::span<const std::int32_t> ramp_exp) noexcept : table_(ramp_exp) {}

    /// Converts a playback ratio into the ramp domain.
    ///
    /// The engine arrives here from milli-semitones, but the port computes a ratio, and the two
    /// agree exactly because the domain is logarithmic with a known octave size.
    [[nodiscard]] static int domain_of(double ratio) noexcept
    {
        if (!(ratio > 0.0)) {
            return 0;
        }
        const double units = unity + (std::log2(ratio) * units_per_octave);
        return std::clamp(static_cast<int>(std::lround(units)), 0, domain_max) & ~1;
    }

    /// The 16.16 sampler increment a domain value decodes to.
    ///
    /// Transcribed from `ramp_env_step_eval`: interpolate the table, round toward zero on the
    /// negative side, then apply the variable shift that carries the octave.
    [[nodiscard]] std::int32_t increment_of(int value) const noexcept
    {
        if (value == 0) {
            return 0;
        }
        const auto index = static_cast<std::size_t>((value >> 6) & 0xFF);
        const int fraction = value & 0x3F;
        const std::int64_t interpolated =
            (static_cast<std::int64_t>(table_[index]) * (0x40 - fraction))
            + (static_cast<std::int64_t>(table_[index + 1]) * fraction);
        const std::int64_t rounded = (interpolated + ((interpolated >> 63) & 0x3F)) >> 6;
        return static_cast<std::int32_t>(rounded >> (0xF - ((value >> 0xE) & 0xF)));
    }

    /// Arms the ramp to glide from one ratio to another across the coming block.
    ///
    /// The step is `((to - from) * rate) >> 13` exactly as `voice_set_ramp_target_0` forms it, with
    /// the engine's maximum rate of 0xfff — which makes the ramp cover the interval in two steps,
    /// the behaviour measured on every voice examined. A ramp whose endpoints coincide stays
    /// inactive, which is the common case and costs nothing.
    void arm(double from_ratio, double to_ratio) noexcept
    {
        current_ = domain_of(from_ratio);
        target_ = domain_of(to_ratio);
        const auto delta = static_cast<std::int64_t>(target_) - current_;
        step_ = static_cast<int>((delta * max_rate) >> 13);
        if (step_ == 0 && current_ < target_) {
            step_ = 1;
        }
        active_ = current_ != target_ && step_ != 0;
    }

    /// Whether the ramp still has ground to cover.
    [[nodiscard]] bool is_active() const noexcept { return active_; }

    /// Advances one slot and returns the increment that serves the next eight samples.
    [[nodiscard]] std::int32_t next_slot() noexcept
    {
        if (!active_) {
            return increment_of(target_);
        }

        current_ = step_ < 0 ? std::max(target_, current_ + step_)
                             : std::min(target_, current_ + step_);
        if (current_ == target_) {
            active_ = false;
        }
        return increment_of(current_);
    }

private:
    /// `voice_set_ramp_target_0`'s rate field is twelve bits, and every voice measured uses its
    /// maximum, which sizes the step at half the interval.
    static constexpr int max_rate = 0xFFF;

    std::span<const std::int32_t> table_;
    int current_ = unity;
    int target_ = unity;
    int step_ = 0;
    bool active_ = false;
};

} // namespace ts
