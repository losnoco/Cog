#pragma once

#include "tabulasonora/table_set.hpp"

#include <cstdint>
#include <span>
#include <utility>

namespace ts {

/// The sampler's 4-tap FIR resampler — the single most timbre-defining element of the engine.
///
/// 128 phase rows of four coefficients, indexed by the top seven bits of the fractional phase, with
/// no interpolation between rows. Every row sums to 1.0 — to within 1e-5, measured: the sums span
/// 0.999999999970896 to 1.000009999999747, so a flat input comes back flat to about five decimal
/// places and not to float epsilon.
///
/// The row at zero fractional phase is `[0.174, 0.653, 0.173, 1e-5]` — not a passthrough. That mild
/// lowpass is applied at *every* sample regardless of pitch, and it is why linear interpolation
/// sounds measurably brighter: at a quarter of the sample rate this kernel passes 0.638 where
/// linear passes 0.707.
///
/// Deliberately scalar. Vectorising the tap sum would reassociate float addition, and float
/// addition is not associative — the result would be close, and not the same.
class Interpolator {
public:
    /// Number of phase rows in the kernel.
    static constexpr int phase_count = 128;

    /// Taps per phase row.
    static constexpr int tap_count = 4;

    /// Creates the resampler over a loaded table set, which must outlive it.
    explicit Interpolator(const TableSet& tables) : coefficients_(tables.interp_coef()) {}

    /// Reads one sample at a fractional position.
    ///
    /// The window reaches from `i-1` to `i+2`, so the index is clamped to leave room for all four
    /// taps. Near a loop boundary the caller must supply the samples the loop wraps to, not
    /// whatever follows it in the buffer.
    [[nodiscard]] float sample(std::span<const float> buffer, double position) const noexcept;

    /// Reads one sample from a power-of-two ring buffer at an absolute index.
    ///
    /// For a stream generated as it is read rather than held whole — a ping-pong traversal, whose
    /// predictor keeps accumulating in both directions and so never repeats. The window still
    /// reaches from `i-1` to `i+2`, so the caller must have generated two samples ahead.
    [[nodiscard]] float
    sample_ring(std::span<const float> ring, std::int64_t index, double fraction) const noexcept;

    /// Resamples a buffer at a series of fractional positions.
    void resample(std::span<const float> buffer,
                  std::span<const double> positions,
                  std::span<float> destination) const noexcept;

private:
    std::span<const float> coefficients_;
};

/// The engine's pan law: an exact 128-entry table, not a computed curve.
///
/// Left reads `T[127 - p]` and right reads `T[p - 1]`, so both land on index 63 at the centre and
/// pan 64 is exactly symmetric at 75/127 ≈ 0.5906 — neither the 0.707 of a constant-power law nor
/// the 0.5 of a linear one. Recovered by sweeping the controller through the engine and reading
/// per-channel RMS, and it reproduces that sweep to within 0.00037.
class PanLaw {
public:
    /// Pan value that sounds centred.
    static constexpr int centre = 64;

    /// Creates the pan law over a loaded table set, which must outlive it.
    explicit PanLaw(const TableSet& tables) : table_(tables.pan()) {}

    /// The left and right gains for a pan position, 0 to 127, with 64 centred.
    ///
    /// Pan 0 is fully left: the right channel is silent, not merely attenuated.
    [[nodiscard]] std::pair<double, double> gains(int pan) const noexcept;

private:
    std::span<const std::uint8_t> table_;
};

} // namespace ts
