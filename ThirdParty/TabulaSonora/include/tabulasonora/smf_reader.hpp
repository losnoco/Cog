#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace ts {

/// What kind of message an event carries.
enum class MidiEventKind {
    /// A channel voice message.
    channel,
    /// A system-exclusive message, including the leading `F0`.
    sysex,
};

/// One scheduled MIDI message.
struct MidiEvent {
    /// Position in samples, quantised to the render-block grid.
    std::int64_t position = 0;
    /// Which kind of message.
    MidiEventKind kind = MidiEventKind::channel;
    /// Status byte, for a channel message.
    int status = 0;
    /// First data byte.
    int data1 = 0;
    /// Second data byte, or zero for two-byte messages.
    int data2 = 0;
    /// Raw bytes, for a system-exclusive message.
    std::vector<std::uint8_t> sysex;

    /// The channel a channel message addresses.
    [[nodiscard]] int channel() const noexcept { return status & 0x0F; }

    /// The message type nibble.
    [[nodiscard]] int message_type() const noexcept { return status & 0xF0; }
};

/// A dependency-free Standard MIDI File reader.
///
/// Handles formats 0, 1 and 2, running status, system-exclusive messages, and the tempo map. Track
/// ticks are converted to seconds through that map and then to samples, and finally quantised to
/// the render-block grid — the granularity at which the engine actually applies events. Quantising
/// here is what lets an offline render line up sample-for-sample with the real engine's own output.
///
/// Meta events other than tempo are dropped; nothing downstream consumes them.
namespace smf {

/// Samples per render block — the grid events are applied on.
inline constexpr int block_grid = 32;

/// Default tempo when a file sets none: 120 bpm, in microseconds per quarter note.
inline constexpr int default_tempo = 500'000;

/// Reads a Standard MIDI File, ordered by position.
[[nodiscard]] std::vector<MidiEvent> read(const std::filesystem::path& path,
                                          int sample_rate = 32000);

/// Parses a Standard MIDI File held in memory, ordered by position.
///
/// Throws `std::runtime_error` if the file is malformed or uses SMPTE timing.
[[nodiscard]] std::vector<MidiEvent> parse(std::span<const std::uint8_t> data,
                                           int sample_rate = 32000);

/// Rounds a sample position onto the render-block grid.
///
/// The rounding is to nearest with ties to **even**, matching .NET's `Math.Round`, because that is
/// what the reference build uses. C's `round` and `llround` go half-away-from-zero instead.
///
/// Measured: for *this* expression the choice does not matter. Over 400,000 half-sample positions
/// the two modes disagree at every one of the 100,000 ties and the resulting block is the same
/// every time — the two candidates differ by one, and the floor onto a 32-sample grid absorbs that
/// unless they straddle a multiple of 32, which a tie can never do. `nearbyint` is kept anyway so
/// the expression matches the original exactly rather than by an argument that could stop holding.
///
/// The same is not true of `Math.Round` where no grid follows it — the delay's tap lengths are the
/// case to watch.
[[nodiscard]] std::int64_t quantise(double samples) noexcept;

} // namespace smf
} // namespace ts
