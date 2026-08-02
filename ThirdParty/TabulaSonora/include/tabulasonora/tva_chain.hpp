#pragma once

#include "tabulasonora/envelope_machine.hpp"
#include "tabulasonora/partial_parameters.hpp"
#include "tabulasonora/segment_envelope.hpp"
#include "tabulasonora/table_set.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace ts {

/// The amplitude side of a voice: the level chain that turns static bytes into a gain, and the
/// four-segment envelope that moves it.
///
/// Levels are logarithmic and, composed through the amplitude curves, work out to amplitude
/// *squared* — `g_level_curve` through `g_amp_curve` is `(l / 127)^2` to within 6e-05. The chain
/// subtracts four attenuations from a base, clamping to 1 after each: the partial level, the
/// crossfaded velocity level, the multisample zone level, and the tone's master level.
///
/// Dropping the last two is what once left a 1.15–2.06x per-voice residual against the engine's own
/// gain word. With all four applied the reproduction is exact to the harness's print precision.
class TvaChain {
public:
    /// The engine hands the sampler twice the amplitude the level chain yields.
    static constexpr double amp_scale = 2.0;

    /// The control-tick rate, which is the grid note-off is acted on.
    static constexpr int control_tick_hz = 100;

    /// Creates the chain over a loaded table set and the shared machine, both of which must outlive
    /// it.
    TvaChain(const TableSet& tables, const EnvelopeMachine& envelope);

    /// The shared segment-rate machine.
    [[nodiscard]] const EnvelopeMachine& envelope() const noexcept { return *envelope_; }

    /// Converts a 16-bit logarithmic level to a linear gain in [0, 1].
    ///
    /// The two table entries multiply to more than 32 bits before the shift, so the product must be
    /// widened. Note also that `g_amp_curve_hi[0]` is 4 rather than 0, so this returns 4.6e-05 at
    /// level zero — a level that has decayed past the floor must be forced to true silence rather
    /// than clamped into the table.
    [[nodiscard]] double amp_of(int level16) const noexcept;

    /// The partial's velocity-crossfaded level — the byte the engine keeps at `voice+0x164`.
    ///
    /// Returns nothing when the velocity window gates the partial off. This is *not* the MIDI
    /// velocity, which lives at `voice+0x166`. Everything downstream — the second attenuation in
    /// `base_level` and both envelope rate multipliers — reads this instead.
    [[nodiscard]] std::optional<int> partial_level(const PartialParameters& partial,
                                                   int velocity) const noexcept;

    /// The 16-bit logarithmic base level for a voice, before the envelope moves it.
    [[nodiscard]] int base_level(const PartialParameters& partial,
                                 int partial_level,
                                 int key,
                                 int zone_level,
                                 int tone_level) const noexcept;

    /// Builds the amplitude envelope for one note, ready to be evaluated at any sample position.
    ///
    /// The envelope does not need to know when the note ends: call `SegmentEnvelope::note_off` when
    /// it does. That is what lets the real-time voice loop and the offline renderer share one
    /// implementation.
    ///
    /// `attack_milliseconds` is optional attack softening; zero is faithful, since the engine's
    /// amplitude attack is instant.
    ///
    /// `rate_key` indexes the two rate key-follow tables when it differs from `key`. The engine
    /// keeps it at `voice+0x161`, which `voice_trigger_partials` fills from the note plus a
    /// transpose; on a drum part that works out to the kit's coarse-pitch plane, so it is not the
    /// MIDI key. Nothing uses `key`, which is what a melodic note wants.
    [[nodiscard]] SegmentEnvelope create_envelope(const PartialParameters& partial,
                                                  int velocity,
                                                  int key,
                                                  int zone_level = 127,
                                                  int tone_level = 127,
                                                  int sample_rate = 32000,
                                                  double attack_milliseconds = 0.0,
                                                  std::optional<int> rate_key = std::nullopt) const;

    /// Renders the amplitude envelope for one note, or an empty vector when it cannot sound.
    ///
    /// The gain is deliberately left smooth per sample rather than stepped per control tick. A
    /// stepwise model looks more faithful to a control-rate gain word, but it measures worse
    /// against the engine — consistent with the envelope block's own anti-zipper ramp interpolating
    /// across the block.
    [[nodiscard]] std::vector<float> render(const PartialParameters& partial,
                                            int velocity,
                                            int key,
                                            double hold_seconds,
                                            double tail_seconds,
                                            int zone_level = 127,
                                            int tone_level = 127,
                                            int sample_rate = 32000,
                                            double attack_milliseconds = 0.0,
                                            std::optional<int> rate_key = std::nullopt) const;

    /// The part-level volume multiplier, relative to everything at 127.
    ///
    /// Volume and expression enter symmetrically as their product, and the result is squared. The
    /// intermediate exceeds 32 bits, so it is computed widened.
    [[nodiscard]] static double
    part_volume_scale(int volume = 127, int expression = 127, int master = 127) noexcept;

private:
    const EnvelopeMachine* envelope_;
    std::span<const std::uint16_t> level_curve_;
    std::span<const std::uint16_t> amp_high_;
    std::span<const std::uint16_t> amp_low_;
    std::span<const std::uint8_t> velocity_curve_;
    std::span<const std::uint8_t> scale_curve_;
    std::span<const std::uint8_t> level_key_follow_;
    std::span<const std::uint8_t> rate_key_follow0_;
    std::span<const std::uint8_t> rate_key_follow1_;
    std::span<const std::uint8_t> velocity_crossfade_;
};

} // namespace ts
