#pragma once

#include "tabulasonora/engine_noise.hpp"
#include "tabulasonora/part_modifiers.hpp"
#include "tabulasonora/partial_parameters.hpp"
#include "tabulasonora/table_set.hpp"

#include <cstdint>
#include <span>
#include <utility>
#include <vector>

namespace ts {

/// Which parameter an LFO's depth applies to.
enum class LfoDestination {
    /// Pitch, in milli-semitones.
    pitch,
    /// Filter cutoff, in the same units as the runtime cutoff.
    tvf,
    /// Amplitude, as a fraction of 0x7f00 applied multiplicatively.
    tva,
};

/// One LFO's static configuration.
struct LfoConfig {
    /// Waveform index, after the selector nibble is mapped.
    int waveform = 0;
    /// Starting phase, from the top two bits of the waveform byte.
    int initial_phase = 0;
    /// Phase increment per control tick.
    int increment = 0;
    /// Per-tick increment of the delay accumulator; zero means never start.
    int delay_rate = 0;
    /// Per-tick increment of the fade-in accumulator.
    int fade_rate = 0;
    /// Depth applied to pitch, in milli-semitones.
    int pitch_depth = 0;
    /// Depth applied to the filter cutoff.
    int tvf_depth = 0;
    /// Depth applied to amplitude.
    int tva_depth = 0;

    /// The depth for one destination.
    [[nodiscard]] constexpr int depth(LfoDestination destination) const noexcept
    {
        switch (destination) {
        case LfoDestination::pitch:
            return pitch_depth;
        case LfoDestination::tvf:
            return tvf_depth;
        case LfoDestination::tva:
            break;
        }
        return tva_depth;
    }
};

class LfoRunner;

/// The two LFO engines that modulate every partial.
///
/// A partial is modulated by both. LFO1 is tone-common, with its parameters in the tone header and
/// its per-partial depths in the partial block; LFO2 is per-partial throughout. The asymmetry that
/// matters is in how their rate and delay fields are read: LFO1's are *indices* into lookup tables,
/// while LFO2's are used raw as per-tick increments. Verified live — LFO1 rate 43 gives increment
/// 2817, LFO2 rate 917 gives 917.
///
/// Pitch depth is in *milli-semitones*, not cents. The pitch accumulator clamps at 0x1f018, which
/// is 127 x 1000, and that is what fixes the unit.
///
/// Waveform selectors 1, 2 and 3 are the random shapes. They are **not** functions of the phase the
/// way every other shape is — they redraw when the phase *wraps* and hold or walk toward that draw
/// in between — so they live in ts::LfoRunner rather than in `waveform` below, which asks only
/// where in the cycle the LFO is and cannot express them.
class LfoEngine {
public:
    /// Scale that maps a raw waveform value to [-1, 1].
    static constexpr double waveform_scale = 32640.0;

    /// Whether a waveform is one of the three random shapes, which the runner drives.
    [[nodiscard]] static constexpr bool is_random(int waveform) noexcept
    {
        return waveform >= 1 && waveform <= 3;
    }

    /// How far a slewed random shape moves toward its target in one control tick.
    static constexpr int slew_step = 0x50;

    /// Depth clamp and rounding constant for a destination.
    ///
    /// Amplitude rounds by ceiling while pitch and filter round to nearest. Only the constant
    /// differs; the sign-magnitude multiply is otherwise identical.
    struct Limits {
        int clamp = 0;
        int rounding = 0;
    };

    [[nodiscard]] static constexpr Limits destination_limits(LfoDestination destination) noexcept
    {
        switch (destination) {
        case LfoDestination::tva:
            return Limits{0x7F00, 0xFFFF};
        case LfoDestination::tvf:
            return Limits{0x1800, 0x8000};
        case LfoDestination::pitch:
            break;
        }
        return Limits{6000, 0x8000};
    }

    /// Creates the engine over a loaded table set, which must outlive it.
    ///
    /// `noise` is the engine's shared generator, which the random shapes draw from; when null a
    /// private one is used. Sharing it is not a detail — the module has exactly one generator, and
    /// the LFO shapes, the pitch start jitter and the random pan all draw from it, so which voice
    /// draws when is part of what the sequence is.
    explicit LfoEngine(const TableSet& tables, EngineNoise* noise = nullptr);

    /// The generator the random shapes draw from.
    [[nodiscard]] EngineNoise& noise() const noexcept { return *noise_; }

    /// Evaluates a waveform at a 16-bit phase.
    ///
    /// The three random shapes are not evaluable this way and return zero here; ts::LfoRunner
    /// drives them, because what they depend on is the phase having wrapped rather than the phase.
    [[nodiscard]] int waveform(int phase, int waveform_index) const noexcept;

    /// Reads both LFO configurations for one partial: the tone-common LFO1 and the per-partial
    /// LFO2.
    ///
    /// `modifiers` applies the part's vibrato rate, depth and delay. They reach LFO1 alone — LFO2
    /// has no table indices for them to bias.
    [[nodiscard]] std::pair<LfoConfig, LfoConfig>
    configure(int tone_number,
              const PartialParameters& partial,
              const PartModifiers& modifiers = {}) const;

    /// Creates a runner that steps one LFO a control tick at a time.
    [[nodiscard]] LfoRunner create_runner(const LfoConfig& config) const;

    /// Creates runners for both of a partial's LFOs.
    [[nodiscard]] std::pair<LfoRunner, LfoRunner>
    create_runners(int tone_number,
                   const PartialParameters& partial,
                   const PartModifiers& modifiers = {}) const;

    /// Runs one LFO for a number of control ticks.
    [[nodiscard]] std::vector<double>
    run(const LfoConfig& config, int tick_count, LfoDestination destination) const;

    /// The combined LFO1 and LFO2 modulation for a partial, per control tick.
    ///
    /// The first tick is always zero: the LFO object is created after that tick's control update,
    /// so nothing has been applied yet. Aligning to that is what makes the comparison against the
    /// engine's own trace line up.
    [[nodiscard]] std::vector<double> modulation(int tone_number,
                                                 const PartialParameters& partial,
                                                 int tick_count,
                                                 LfoDestination destination) const;

    /// Combined pitch modulation with a per-tick mod-wheel depth folded into LFO1.
    ///
    /// The wheel's contribution is added *after* the fade-in, then the total is clamped to +-6000 —
    /// six semitones, the GS full-scale figure. Only LFO1 takes it; LFO2 is unaffected.
    [[nodiscard]] std::vector<double>
    pitch_modulation_with_wheel(int tone_number,
                                const PartialParameters& partial,
                                int tick_count,
                                std::span<const double> mod_depth_per_tick) const;

    /// The mod wheel's contribution to LFO1 pitch depth, in milli-semitones.
    ///
    /// Summed with the tone's own pitch depth after the cents table, and the total is then clamped
    /// to +-6000.
    [[nodiscard]] static int
    mod_wheel_depth(int controller, int depth = 0x0A, int offset = 0) noexcept;

private:
    [[nodiscard]] std::vector<double> run_pitch_with_wheel(
        const LfoConfig& config, int tick_count, std::span<const double> mod_depth_per_tick) const;

    std::span<const std::uint8_t> half_sine_;
    std::span<const std::uint16_t> rate_table_;
    std::span<const std::uint16_t> cents_table_;
    std::span<const std::uint16_t> delay_table_;
    std::span<const std::uint8_t> wave_map_;
    std::span<const std::int16_t> wave_bank_;
    std::span<const std::uint8_t> tone_;

    EngineNoise* noise_;
    EngineNoise owned_noise_;
};

/// One LFO's running state: phase, delay and fade-in accumulators, stepped a control tick at a
/// time.
///
/// All three destinations share one runner. The accumulators do not depend on which depth is being
/// applied, so stepping once and reading three values is the same trajectory as running the LFO
/// three times — and it is what the engine does, since a partial has one LFO object, not one per
/// destination.
///
/// `tick` advances; the value is read afterwards, so the first tick already carries a phase
/// increment.
class LfoRunner {
public:
    LfoRunner(const LfoEngine& engine, const LfoConfig& config)
        : engine_(&engine), config_(config), phase_(config.initial_phase & 0xFFFF)
    {
    }

    /// The configuration this runner steps.
    [[nodiscard]] const LfoConfig& config() const noexcept { return config_; }

    /// Whether the last tick ran an update at all, so the LFO has an output to give.
    ///
    /// This is **not** "the delay has elapsed" — the delay holds the fade-in at zero but does not
    /// stop the LFO being applied, since a matrix depth is summed past the fade. What turns it off
    /// is an increment of nothing, which is the one case the module skips outright.
    [[nodiscard]] bool is_applied() const noexcept { return applied_; }

    /// Advances one control tick.
    ///
    /// `rate_offset` is the control matrix's LFO rate destination, in the same per-tick increment
    /// units as the configured rate — the engine adds it to the rate straight out of the table (or,
    /// for LFO2, to the raw rate) and clamps the total to 0x28f6.
    ///
    /// A total of zero or less does not merely stop the phase: the engine skips the whole update,
    /// so the depths are not applied either and the LFO's last output is held rather than decaying.
    /// Deep enough negative rate modulation is therefore an off switch, which is why this shares
    /// its early return with a configured rate of zero.
    void tick(int rate_offset = 0) noexcept;

    /// The modulation for one destination, after the last tick.
    ///
    /// `matrix_depth` is the control matrix's depth destination for this LFO, in the destination's
    /// own units. It is summed *after* the fade-in — the fade scales the patch's depth, not the
    /// controller's — and the total is then clamped to the destination's limit. Those limits are
    /// exactly the matrix's own full-scale figures, so a matrix depth alone can reach the rail but
    /// never exceed it.
    [[nodiscard]] double value(LfoDestination destination, int matrix_depth = 0) const noexcept;

    /// The pitch modulation with a mod-wheel depth folded in, in milli-semitones.
    ///
    /// The offline path's spelling of the same thing: the wheel reaches LFO1 pitch depth through
    /// the matrix, so this is `value(LfoDestination::pitch, ...)` with the wheel's contribution
    /// standing in for the whole matrix sum.
    [[nodiscard]] double pitch_value(double wheel_depth) const noexcept;

private:
    /// Evaluates the waveform for the tick just taken, into `output_`.
    void advance_waveform(bool wrapped) noexcept;

    [[nodiscard]] int faded(int depth) const noexcept;
    [[nodiscard]] double apply(int effective, int rounding) const noexcept;

    const LfoEngine* engine_;
    LfoConfig config_;

    int phase_ = 0;
    int delay_ = 0;
    int fade_ = 0;
    bool applied_ = false;

    /// The waveform value for the last tick — `g_lfo_out`.
    ///
    /// Kept rather than recomputed from the phase because the random shapes cannot be recomputed:
    /// a slewed one that has arrived at its target leaves this standing untouched, so the previous
    /// tick's value *is* this tick's answer. For every other shape it is the same number
    /// `LfoEngine::waveform` would return, evaluated at the tick that produced the phase.
    int output_ = 0;

    /// The random shapes' two registers, which the module keeps per voice next to the phase: the
    /// value last drawn from the generator, and the value walking toward it.
    ///
    /// Both start at zero, which is the one part of this not read off the engine — its per-voice
    /// block is loaded from voice state that a note-on has already cleared. A sample-and-hold
    /// therefore sits at zero until its first wrap rather than opening on a random step.
    int held_ = 0;
    int slewed_ = 0;
};

} // namespace ts
