#pragma once

#include "tabulasonora/engine_noise.hpp"
#include "tabulasonora/envelope_machine.hpp"
#include "tabulasonora/partial_parameters.hpp"
#include "tabulasonora/table_set.hpp"
#include "tabulasonora/wave_descriptor.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace ts {

/// The pitch envelope's decoded segment targets and timings.
struct PitchEnvelope {
    /// Starting offset, in milli-semitones.
    int start = 0;
    /// The four segment targets.
    std::array<int, 4> targets{};
    /// The release target.
    int release = 0;
    /// Segment durations in milliseconds.
    std::array<double, 4> times{};
    /// Release duration in milliseconds.
    double release_ms = 0.0;
};

/// Walks a pitch envelope one control tick at a time.
///
/// Unlike the TVA and TVF envelopes, this one is not a function of elapsed time: a segment
/// completes when a 16-bit phase accumulator reaches 0xffff — not exceeds it — and the next segment
/// starts with a fresh phase rather than carrying the remainder, so the trajectory depends on the
/// accumulator's history. It therefore has to be stepped rather than evaluated.
///
/// The value is stepwise, held for the whole control block, and the value returned by `tick` is the
/// state *after* that tick's update.
class PitchEnvelopeRunner {
public:
    /// Creates a runner over a decoded envelope.
    ///
    /// One-shot mode is bit 7 of partial block byte 0x00: the engine's note-off handler skips
    /// engaging the pitch release while the envelope machine still runs. It is what the `.o`
    /// variation tones use.
    explicit PitchEnvelopeRunner(const PitchEnvelope& envelope, bool ignore_note_off = false);

    /// A runner that holds one level forever, in milli-semitones.
    ///
    /// A partial with random start jitter but a disabled envelope still gets the jitter.
    [[nodiscard]] static PitchEnvelopeRunner constant(double level);

    /// The offset in milli-semitones after the last tick.
    [[nodiscard]] double level() const noexcept { return level_; }

    /// Sets the half-damper pedal value the release will be scaled by, 1–0x3f or zero.
    ///
    /// The engine writes the pedal value into the release ramp's rate-scale byte; the effective
    /// rate is `rate * (0xffff - (v<<9)) >> 16`. Only meaningful before the release engages.
    void set_release_damper(int damper) noexcept;

    /// Advances one control tick and returns the offset in milli-semitones.
    double tick(bool released) noexcept;

private:
    PitchEnvelopeRunner() = default;

    [[nodiscard]] int scaled_release_rate() const noexcept;

    std::vector<int> targets_;
    std::vector<int> rates_;
    int release_ = 0;
    int release_rate_ = 0;
    bool ignore_note_off_ = false;

    double segment_start_ = 0.0;
    double level_ = 0.0;
    int segment_ = 0;
    int phase_ = 0;
    bool released_ = false;
    int release_damper_ = 0;
};

/// The pitch side of a voice: key-follow, transposition, the pitch envelope, and bend.
///
/// Pitch is absolute, in milli-semitones, and the engine clamps the accumulator to `[0, 0x1f018]` —
/// 127 semitones x 1000, which is what fixes the unit. Working in offsets from a base cannot
/// express that clamp, and at least one patch sits exactly on it: Jetplane's first partial has a
/// base of 24000 with an envelope starting at -24000, landing on zero.
class PitchChain {
public:
    /// Upper clamp of the pitch accumulator: 127 semitones in milli-semitones.
    static constexpr int max_pitch_milli_semitones = 0x1F018;

    /// The key a drum's coarse-pitch plane pivots around: 60 is its natural pitch.
    static constexpr int drum_key_centre = 60;

    /// The effective pitch key and its fractional crossfade weight.
    struct KeyFollow {
        int key = 0;
        int weight = 0;
    };

    /// Creates the chain over a loaded table set and machine, both of which must outlive it.
    ///
    /// `noise` is the engine's shared generator; when null a private one is used, which keeps a
    /// standalone chain usable but gives it its own draw sequence.
    PitchChain(const TableSet& tables,
               const EnvelopeMachine& envelope,
               EngineNoise* noise = nullptr);

    /// The effective pitch key and its fractional crossfade weight.
    ///
    /// `block[0x13]` is the follow amount: 0x40 is no follow (the key centre sounds regardless of
    /// the note), 0x4a is full follow, and values between scale the note's distance from the
    /// centre. A reduced-follow patch like the Seashore effect plays note 33 as key 55, which keeps
    /// its noise bright instead of an octave-too-low rumble.
    [[nodiscard]] static KeyFollow
    key_follow_key(const PartialParameters& partial, int note, int key_center = 0x3C) noexcept;

    /// The note's absolute base pitch in milli-semitones.
    ///
    /// `key × 1000 + weight + g_kf_pitch[row 2][key] + coarse`, which is `partial_compute_pitch @
    /// 18005fc20` term for term. Comparable against the module exactly: `tabula-sonora pitch` turns
    /// this and `native` into the module's own ramp word, and `scdec postrace` reads the module's.
    [[nodiscard]] int base_pitch_milli_semitones(const PartialParameters& partial,
                                                 int note,
                                                 int key_center = 0x3C) const noexcept;

    /// A drum key's absolute pitch in milli-semitones, from its coarse-pitch plane value.
    ///
    /// A drum note does not transpose its sample; the coarse-pitch plane supplies the key instead,
    /// pivoting on `drum_key_centre`. What a step of that plane is *worth* is then the tone's own
    /// business: it runs through the same `block[0x13]` key-follow ladder as a melodic note, so a
    /// 100%-follow tone moves a whole semitone per unit and a 50%-follow tone half of one.
    ///
    /// Unlike the melodic chain this omits the `g_kf_pitch` key-follow curve: the engine gates that
    /// term off for drums, and the measured pitches are exact multiples of 1000 where the curve
    /// would have contributed -11 or +3.
    [[nodiscard]] static int drum_pitch_milli_semitones(const PartialParameters& partial,
                                                        int coarse_pitch) noexcept;

    /// The portamento glide step for a CC#5 time byte, in milli-semitones per control tick.
    [[nodiscard]] int portamento_step(int time) const noexcept;

    /// The glide offset a note starts with, in milli-semitones, decaying to zero as it completes.
    ///
    /// The engine takes the source key at a flat `key * 1000` rather than running it back through
    /// the key-follow chain, so a glide always departs from equal temperament.
    [[nodiscard]] static int portamento_offset(int from_key, int target_pitch) noexcept;

    /// Pitch-bend offset in milli-semitones, from a 14-bit bend with 8192 centre.
    [[nodiscard]] static double bend_offset_milli_semitones(int bend = 8192,
                                                            double semitone_range = 2.0) noexcept;

    /// Decodes the pitch envelope's targets and timings, or nothing when it is disabled.
    [[nodiscard]] std::optional<PitchEnvelope>
    envelope_offsets(const PartialParameters& partial, int key, int velocity) const noexcept;

    /// The segment machine's rate word for a duration.
    ///
    /// 0xffff wraps the 16-bit phase in exactly one control tick, so even a zero-length segment
    /// still occupies one tick.
    [[nodiscard]] static int segment_rate_word(double milliseconds) noexcept;

    /// The pitch envelope's offset at each control tick, or nothing when it is inactive.
    ///
    /// Tick `i` is the state *after* that tick's update.
    [[nodiscard]] std::optional<std::vector<double>>
    envelope_ticks(const PartialParameters& partial,
                   int key,
                   int velocity,
                   double hold_seconds,
                   int tick_count) const;

    /// The random start-pitch jitter for one note-on, in milli-semitones.
    ///
    /// Bit 14 of the draw picks the sign. The magnitude slice is asymmetric — 7 bits on the
    /// positive side, 8 on the negative — so the range is about [-10*d, +5*d], and that asymmetry
    /// is the hardware's, not an artefact.
    [[nodiscard]] static int start_jitter_milli_semitones(int depth, std::uint16_t draw) noexcept;

    /// Creates a runner for a partial's pitch envelope, or nothing when it does nothing.
    ///
    /// An envelope whose start, four targets and release are all zero is not merely flat — it is
    /// absent, and skipping it saves the pitch chain a per-tick update on most patches.
    ///
    /// A non-zero block byte 0x1a draws start jitter here, one draw per partial voice, matching the
    /// engine (which skips the draw entirely when the byte is zero, so unaffected patches consume
    /// nothing from the generator).
    [[nodiscard]] std::optional<PitchEnvelopeRunner>
    create_envelope_runner(const PartialParameters& partial, int key, int velocity) const;

    /// The playback ratio for a partial, from its absolute pitch against the sample's own root.
    [[nodiscard]] static double ratio(const PartialParameters& partial,
                                      const WaveDescriptor& descriptor,
                                      double pitch_milli_semitones) noexcept;

    /// Clamps an absolute pitch the way the engine's accumulator does.
    [[nodiscard]] static double clamp(double milli_semitones) noexcept;

private:
    const EnvelopeMachine* envelope_;
    EngineNoise* noise_;
    EngineNoise owned_noise_;

    std::span<const std::uint16_t> portamento_step_;
    std::span<const std::uint8_t> pitch_bias_;
    std::span<const std::uint16_t> depth_velocity_sensitivity_;
    std::span<const std::uint8_t> rate_key_follow0_;
    std::span<const std::uint8_t> rate_key_follow1_;
    std::span<const std::int16_t> key_follow_;
    std::span<const std::uint16_t> rate_curve_;

    /// The depth velocity-sensitivity slope is a signed-16 view of the first 0x80 entries of the
    /// pitch-envelope export.
    std::array<std::int16_t, 0x80> depth_slope_{};
};

} // namespace ts
