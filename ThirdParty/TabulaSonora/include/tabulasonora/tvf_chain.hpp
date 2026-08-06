#pragma once

#include "tabulasonora/control_ramp.hpp"
#include "tabulasonora/envelope_machine.hpp"
#include "tabulasonora/part_modifiers.hpp"
#include "tabulasonora/partial_parameters.hpp"
#include "tabulasonora/segment_envelope.hpp"
#include "tabulasonora/state_variable_filter.hpp"
#include "tabulasonora/table_set.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace ts {

/// The filter side of a voice: the cutoff chain that turns static bytes into the two coefficients
/// the state-variable filter needs, and the envelope that moves the cutoff over time.
///
/// The cutoff runs through four stages. A derives the resonance byte, B sums the base with the
/// envelope, C warps the sum and clamps it to a resonance-dependent ceiling, and D produces the Q
/// trim. The warped result then passes through an exponential table before it reaches the filter,
/// which is the step an earlier attempt missed: feeding linear cutoff units to a Chamberlin filter
/// drives `f` to about 1.9, where the state matrix diverges.
///
/// Cutoff is note-independent. There is no key-follow on it; brightness tracks the keyboard through
/// the multisample instead.
class TvfChain {
public:
    /// Samples between coefficient refreshes — the 100 Hz control tick.
    static constexpr int control_block_samples = 320;

    /// The envelope's peak and its five stage offsets, in cutoff units relative to that peak.
    struct Offsets {
        int peak = 0;
        std::array<int, 4> segments{};
        int release = 0;
    };

    /// A cutoff envelope and the base cutoff its offsets are added to.
    struct Envelope {
        SegmentEnvelope offsets;
        int base_cutoff = 0;
    };

    /// The pair of coefficients the filter runs on for one control block.
    struct Coefficients {
        double frequency = 0.0;
        double damping = 0.0;
    };

    /// Creates the chain over a loaded table set and the shared machine, both of which must outlive
    /// it.
    TvfChain(const TableSet& tables, const EnvelopeMachine& envelope);

    /// Which filter response a partial takes, or `FilterTap::bypass`.
    ///
    /// Only types 0, 1, 2, 4, 5 and 6 are valid; 3 and 7 bypass. The response comes from bits 10–11
    /// of the type's coefficient word, so the mapping is not the obvious one — type 1 is highpass
    /// and type 2 is bandpass.
    [[nodiscard]] FilterTap tap(int filter_type) const noexcept;

    /// The resonance byte — stage A. Floored at 4.
    ///
    /// `part_resonance` is the part's CC#71 modify byte (`part+0x3e7`) and `part_resonance_default`
    /// its user-tone sibling (`part+0x456`), which a standard file leaves neutral. Both subtract, so
    /// raising CC#71 *lowers* the byte and, since the byte is reciprocal-Q, raises the resonance.
    ///
    /// The engine recomputes this every control tick, not once per note: `FUN_1800845c0` reads both
    /// part bytes through the voice's part pointer each time stage A runs.
    [[nodiscard]] static int resonance_byte(const PartialParameters& partial,
                                            int part_resonance = 0x40,
                                            int part_resonance_default = 0x40) noexcept;

    /// The same, from a partial's already-extracted resonance byte.
    ///
    /// A voice keeps that one byte rather than the whole partial, because it has to redo this on
    /// every block against a CC#71 that may have moved since the note started.
    [[nodiscard]] static int resonance_byte_of(int partial_resonance,
                                               int part_resonance = 0x40,
                                               int part_resonance_default = 0x40) noexcept;

    /// Warps a 15-bit cutoff sum and clamps it to the resonance-dependent ceiling — stage C.
    ///
    /// The ceiling at neutral resonance times four is 245,760 — the "fully open" constant an
    /// earlier calibration measured empirically. It is a ceiling, not a saturation of the sum.
    [[nodiscard]] int cutoff_units(double cutoff15, int resonance_byte) const noexcept;

    /// The integer `f` is carried as between the exponential table and the filter.
    ///
    /// This is what `voice_ctrl_ramp_c` accumulates, so it is the quantity a ramp has to walk in:
    /// gliding the decoded double instead would be a different curve, because the decode truncates
    /// three bits off the bottom.
    [[nodiscard]] int frequency_accumulator(int units) const noexcept;

    /// The filter's frequency coefficient `f`.
    ///
    /// Every ramp target passes through the exponential table before it reaches the filter, so what
    /// the filter sees is exponential in the cutoff units. This collapses to `f = 2^(C/16384 -
    /// 15)`, and Chamberlin's `f = 2*sin(pi*fc/fs)` then gives the cutoff in Hz with no fitted
    /// constant at all. The table entries reach 2^18, so the interpolation must be widened before
    /// the shift.
    [[nodiscard]] double frequency_coefficient(int units) const noexcept;

    /// The filter's damping coefficient `q` — stage D.
    ///
    /// This is reciprocal-Q, so the neutral resonance byte 0x40 yields exactly 1.0 and smaller
    /// values are more resonant — effectively `Q = 64 / resonance_byte`.
    [[nodiscard]] double
    damping_coefficient(int units, int resonance_byte, int filter_type) const noexcept;

    /// Both coefficients at once, with `f` clamped to the stability ceiling `q` selects.
    ///
    /// The engine couples the two: `voice_ctrl_ramp_d` ramps `q` and, from the same value, clamps
    /// the `f` that `voice_ctrl_ramp_c` has just written — `f = min(f, g_svf_f_ceil[q_raw >> 8])`.
    /// The ceiling is Chamberlin's own stability bound, `sqrt(q^2 + 4) − q`, so the clamp keeps the
    /// filter out of the region where the loop diverges. Anything that runs the filter must take
    /// its coefficients from here rather than from the two accessors separately.
    ///
    /// The clamp is inert over every cutoff and resonance the engine can reach: `cutoff_units`
    /// already holds `f` below the ceiling for all three `q` branches, though only just — filter
    /// type 6 at resonance byte 4 comes within 0.78%. It is implemented because it is a real stage,
    /// not because it changes a render.
    [[nodiscard]] Coefficients
    coefficients(int units, int resonance_byte, int filter_type) const noexcept;

    /// The shared `g_ramp_exp_tbl`. The pitch ramp decodes its sampler increment from the same
    /// table and the same octave size, so it is exposed here rather than loaded twice.
    [[nodiscard]] std::span<const std::int32_t> ramp_exp() const noexcept { return ramp_exp_; }

    /// The cutoff in Hz. Diagnostic only — the filter never needs it.
    [[nodiscard]] double
    cutoff_hz(double cutoff15, int resonance_byte, int sample_rate = 32000) const noexcept;

    /// The velocity the filter envelope's depth actually responds to.
    ///
    /// Velocity does not reach the depth scaler raw: `block[0x2e]` picks one of sixteen response
    /// curves first. Row 0 is the identity, so the majority of the library is unaffected — Trumpet
    /// selects it and is exact either way. Brass 1 selects row 1, which reads velocity 100 as 71.
    ///
    /// Using raw velocity leaves Brass 1's filter about a third of an octave too open for the whole
    /// note, which measures as +3.5 dB at 4–8 kHz and +6.3 dB above it.
    [[nodiscard]] int effective_velocity(const PartialParameters& partial,
                                         int velocity) const noexcept;

    /// The envelope's peak and its five stage offsets.
    [[nodiscard]] Offsets
    envelope_offsets(const PartialParameters& partial, int key, int velocity) const;

    /// Builds the cutoff envelope for one note, ready to be evaluated at any sample position.
    ///
    /// The running level starts at zero rather than at the release level: all five targets are made
    /// relative to the peak, and the peak itself is folded into the base cutoff instead. TVF
    /// segments are always linear — unlike the TVA, the shape is not data-driven here.
    ///
    /// `modifiers` supplies the part's cutoff offset, which always applies, and its envelope
    /// offsets, which apply only when the partial opts in — see `responds_to_env_modifiers`.
    [[nodiscard]] Envelope create_envelope(const PartialParameters& partial,
                                           int velocity,
                                           int key,
                                           int sample_rate = 32000,
                                           const PartModifiers& modifiers = {}) const;

    /// Whether this partial's filter envelope follows the part's envelope modify offsets.
    ///
    /// Bit 4 of block byte 0x0E. `tvf_compute_env_rates` zeroes its bias outright when the bit is
    /// clear, so on those partials CC#73/75/72 move the amplitude envelope and leave the filter
    /// envelope alone. The amplitude side has no such gate.
    [[nodiscard]] static bool responds_to_env_modifiers(const PartialParameters& partial) noexcept
    {
        return (partial.raw()[0x0E] & 0x10) != 0;
    }

    /// The 15-bit cutoff trajectory over a note, clamped to 15 bits.
    [[nodiscard]] std::vector<double> envelope(const PartialParameters& partial,
                                               int velocity,
                                               int key,
                                               double hold_seconds,
                                               double tail_seconds,
                                               int sample_rate = 32000) const;

    /// Filters a signal in place with a cutoff that moves per control block.
    ///
    /// Coefficients refresh once per control block, matching the engine's 100 Hz rate. The engine
    /// additionally slews them over 2–40 ms through its anti-zipper ramps, which is not modelled.
    ///
    /// The coefficients come from the cutoff's **mean over the block they will serve**, not its
    /// value at the tick: the envelope can cross several segments inside one 10 ms tick, and a
    /// single sample point costs about 1.7% of peak on a piano attack.
    void apply(std::span<float> signal,
               std::span<const double> cutoff15,
               int filter_type,
               int resonance_byte,
               int block_samples = control_block_samples) const noexcept;

private:
    // The env-depth region is addressed by absolute VA in the decompile. Relative to the start of
    // the cached slice (VA 0x1819a2f00) the two sub-tables sit here:
    //   0x1819a3028 - 0x1819a2f00 = 0x128   (depth below neutral)
    //   0x1819a2fa8 - 0x1819a2f00 = 0x0a8   (depth at or above neutral)
    static constexpr int env_depth_low_branch = 0x128;
    static constexpr int env_depth_high_branch = 0x0A8;

    // g_pitch_bias lives inside the pitch-envelope export: 0x1819a2890 - 0x1819a2578 = 0x318.
    static constexpr int pitch_bias_offset = 0x318;

    [[nodiscard]] int env_depth_word(int offset) const;
    [[nodiscard]] int pitch_bias(int magnitude) const;

    /// The damping coefficient before its scaling — the integer the ceiling is indexed by.
    [[nodiscard]] int damping_raw(int units, int resonance_byte, int filter_type) const noexcept;

    const EnvelopeMachine* envelope_machine_;
    std::span<const std::uint8_t> env_depth_;
    std::span<const std::uint8_t> pitch_env_;
    std::span<const std::int16_t> env_depth_key_follow_;
    std::span<const std::int16_t> reso_curve_;
    std::span<const std::uint8_t> rate_key_follow_;
    std::span<const std::uint32_t> filter_type_coef_;
    std::span<const std::uint16_t> warp_;
    std::span<const std::uint16_t> ceiling_;
    std::span<const std::uint16_t> q_low_pass_;
    std::span<const std::uint16_t> q_type6_;
    std::span<const std::int32_t> ramp_exp_;
    std::span<const float> f_ceiling_;
    std::span<const std::uint8_t> velocity_sensitivity_;
};

} // namespace ts
