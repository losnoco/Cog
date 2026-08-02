#pragma once

#include "tabulasonora/drum_kit_table.hpp"
#include "tabulasonora/engine_noise.hpp"
#include "tabulasonora/smf_reader.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace ts {

/// A controller's value over time, as breakpoints.
///
/// Zero-order hold: a value applies from its position until the next breakpoint. Note parameters
/// are latched at note-on, so what matters is the value in force at that instant.
class ControllerTimeline {
public:
    /// Records a change.
    void add(std::int64_t position, int value) { points_.push_back(Point{position, value}); }

    /// The value in force at a position, or `fallback` before the first breakpoint.
    [[nodiscard]] int value_at(std::int64_t position, int fallback) const noexcept;

    /// Number of recorded changes.
    [[nodiscard]] std::size_t size() const noexcept { return points_.size(); }

    /// Fills a span with this controller's value at every sample from a starting position.
    ///
    /// Returns whether the value changed at any point within the span. Walks the breakpoints once
    /// rather than searching per sample. Sampling at control-tick resolution instead would be ten
    /// times too coarse: events land on the 32-sample render grid, which is the granularity the
    /// engine applies them at.
    bool fill(std::int64_t start, std::span<int> destination, int fallback) const noexcept;

private:
    struct Point {
        std::int64_t position = 0;
        int value = 0;
    };

    std::vector<Point> points_;
};

/// Per-part coarse-pitch and panpot overrides, set by NRPN.
///
/// Drum keys only: a melodic part never reaches these.
class DrumKeyOverrides {
public:
    /// Data-entry value meaning no change.
    static constexpr int centre = 0x40;

    /// The panpot entry that means "random", which only the GS SysEx panpot can write.
    static constexpr int random_pan = 0;

    DrumKeyOverrides() { reset(); }

    /// Whether any override has been recorded.
    [[nodiscard]] bool is_empty() const noexcept { return !any_; }

    /// Clears every override.
    void reset() noexcept;

    /// Records a coarse-pitch override for one key; `centre` is no change.
    void set_pitch(int note, int entry) noexcept;

    /// Records a panpot override for one key, used directly as the key's pan.
    void set_pan(int note, int entry) noexcept;

    /// Records an absolute play-key override for one key, from drum-setup SysEx `41 m1 kk`.
    ///
    /// Unlike the pitch NRPN this is not relative: the value replaces the kit's coarse-pitch
    /// plane entry outright.
    void set_play_key(int note, int entry) noexcept;

    /// Records a level override for one key, replacing the kit's level plane entry.
    void set_level(int note, int entry) noexcept;

    /// Records an assign-group override for one key.
    void set_group(int note, int entry) noexcept;

    /// Records per-key send depths, from NRPN `1D`/`1E`/`1F` or drum-setup SysEx.
    ///
    /// Latched but not yet consumed: the mixer runs one send bus per part, so scaling a single
    /// key's contribution needs per-voice send gains it does not have yet.
    void set_reverb(int note, int entry) noexcept;
    void set_chorus(int note, int entry) noexcept;
    void set_delay(int note, int entry) noexcept;

    /// The coarse-pitch offset held for a key, in kit-plane steps.
    ///
    /// A step's worth in pitch is the tone's own key-follow: a semitone on a 100%-follow tone,
    /// half of one at 50%. The famous "SC-55 mode doubles the NRPN's range" is exactly this --
    /// the SC-55 kits use 100%-follow tones where the later standard kits use 50%.
    [[nodiscard]] int pitch_offset(int note) const noexcept;

    /// The panpot held for a key, or nothing when the key follows its kit record.
    [[nodiscard]] std::optional<int> pan(int note) const noexcept;

    /// The absolute play-key override for a key, or nothing when it follows the kit.
    [[nodiscard]] std::optional<int> play_key(int note) const noexcept;

    /// The level override for a key, or nothing when it follows the kit.
    [[nodiscard]] std::optional<int> level(int note) const noexcept;

    /// The assign-group override for a key, or nothing when it follows the kit.
    [[nodiscard]] std::optional<int> group(int note) const noexcept;

    /// Resolves the panpot for one strike, spreading a randomly-panned key.
    ///
    /// The spread is the engine's own: a draw from the shared generator, reduced to a pan position
    /// by its top seven bits. What still cannot match is the *sequence* under polyphony, since that
    /// depends on how many voices consume draws and in what order.
    ///
    /// Consumes a draw for a randomly-panned key, so call it once per strike.
    [[nodiscard]] std::optional<int> pan_for_hit(int note, EngineNoise& noise) const;

    /// Layers a latched pair of overrides over a key read from the kit record.
    ///
    /// The offset lands on the plane unscaled and the sum clamps to 0-0x7F, which is the engine's
    /// own plane write (`nrpn_apply` case 0x18, confirmed by live plane reads: entry +12 moves the
    /// stored plane 60 to 72 on every map). The "absolute rate floor" measured but never located
    /// in earlier work is this clamp's bottom: keys with different kit planes floor at different
    /// offsets, which is what made it look like a rate limit rather than a pitch one.
    [[nodiscard]] static DrumKey
    apply(DrumKey key, int pitch_offset, std::optional<int> pan) noexcept;

private:
    std::array<int, DrumKitTable::key_count> pitch_{};
    std::array<int, DrumKitTable::key_count> pan_{};
    // The drum-setup planes, -1 where the key follows its kit record.
    std::array<int, DrumKitTable::key_count> play_key_{};
    std::array<int, DrumKitTable::key_count> level_{};
    std::array<int, DrumKitTable::key_count> group_{};
    std::array<int, DrumKitTable::key_count> reverb_{};
    std::array<int, DrumKitTable::key_count> chorus_{};
    std::array<int, DrumKitTable::key_count> delay_{};
    bool any_ = false;
};

/// One note, with every parameter latched as it was at note-on.
struct NoteRecord {
    int channel = 0;
    int note = 0;
    int velocity = 0;
    /// Note-on position in samples.
    std::int64_t on = 0;
    /// Note-off position in samples, after any damper extension.
    std::int64_t off = 0;
    int program = 0;
    /// Bank select MSB.
    int bank = 0;
    /// CC#10.
    int pan = 0;
    /// CC#7.
    int volume = 0;
    /// CC#11.
    int expression = 0;
    /// CC#91.
    int reverb_send = 0;
    /// CC#93.
    int chorus_send = 0;
    /// Part delay send, which has no Control Change and arrives only over SysEx.
    int delay_send = 0;
    /// Coarse-pitch override for this drum key from NRPN 0x18, in steps of one semitone. Zero when
    /// the key follows its kit record, which is every key on a melodic part.
    int drum_pitch = 0;
    /// Panpot override for this drum key from NRPN 0x1C, or nothing when it follows the kit.
    std::optional<int> drum_pan;
};

/// Per-channel controller timelines.
struct PartTimelines {
    ControllerTimeline volume;
    ControllerTimeline expression;
    ControllerTimeline pan;
    /// Pitch bend, as a 14-bit value.
    ControllerTimeline bend;
    /// Bend range in semitones, from RPN 00/00.
    ControllerTimeline bend_range;
    /// CC#64 damper.
    ControllerTimeline damper;
    /// CC#1 modulation.
    ControllerTimeline modulation;
    ControllerTimeline reverb_send;
    ControllerTimeline chorus_send;
    /// Delay send, set over SysEx.
    ControllerTimeline delay_send;
    ControllerTimeline program;
    /// Bank select MSB.
    ControllerTimeline bank;
};

/// A parsed sequence: controller timelines, notes, and the global effect selections.
struct Sequence {
    /// MIDI channels.
    static constexpr int channel_count = 16;

    /// Per-channel timelines, indexed by MIDI channel.
    std::array<PartTimelines, channel_count> parts;
    /// Every note, with its parameters latched at note-on.
    std::vector<NoteRecord> notes;
    ControllerTimeline master_volume;
    /// GS reverb macro selections.
    ControllerTimeline reverb_type;
    /// GS chorus macro selections.
    ControllerTimeline chorus_type;
    /// GS delay macro selections.
    ControllerTimeline delay_type;
    /// Position of the final event of any kind.
    ///
    /// This sets where a render ends, and it is not the same as the last note: a file commonly
    /// closes with controller or meta traffic after the music stops.
    std::int64_t last_event_position = 0;
};

/// Turns a flat event list into per-part controller timelines and a note list.
///
/// The engine has no MIDI output, so nothing can be read back from it: every piece of channel state
/// has to be tracked here.
namespace sequence_builder {

/// Power-on volume after a GM or GS reset.
inline constexpr int default_volume = 100;
/// Power-on expression.
inline constexpr int default_expression = 127;
/// Power-on pan.
inline constexpr int default_pan = 64;
/// Power-on master volume.
inline constexpr int default_master = 127;
/// GM default reverb send.
inline constexpr int default_reverb_send = 40;
/// GM default chorus send.
inline constexpr int default_chorus_send = 0;

/// Builds a sequence from a flat event list, ordered by position.
[[nodiscard]] Sequence build(std::span<const MidiEvent> events);

/// Converts a GS block number to a MIDI channel.
///
/// The block number is not the channel: block 0 is channel 9 (the drum part), blocks 1–9 are
/// channels 0–8, and blocks 10–15 map straight through.
[[nodiscard]] constexpr int channel_from_block(int block) noexcept
{
    return block == 0 ? 9 : block <= 9 ? block - 1 : block;
}

} // namespace sequence_builder
} // namespace ts
