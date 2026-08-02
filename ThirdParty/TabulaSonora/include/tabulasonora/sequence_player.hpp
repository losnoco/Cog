#pragma once

#include "tabulasonora/sequence_renderer.hpp"
#include "tabulasonora/tone_generator.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

namespace ts {

/// Drives a running engine from a MIDI event list.
///
/// The engine itself has no notion of a file: this is what turns one into the stream of events it
/// consumes, dispatching each at the render block it falls in.
class SequencePlayer {
public:
    /// Creates a player over an engine and an event list ordered by position.
    ///
    /// Both must outlive the player.
    SequencePlayer(ToneGenerator& generator, std::vector<MidiEvent> events);

    /// Reads a Standard MIDI File and creates a player for it.
    [[nodiscard]] static SequencePlayer from_file(ToneGenerator& generator,
                                                  const std::filesystem::path& path);

    /// The engine being driven.
    [[nodiscard]] ToneGenerator& generator() noexcept { return *generator_; }

    /// Position of the final event of any kind.
    ///
    /// Not the same as the last note: a file commonly closes with controller or meta traffic after
    /// the music stops, and stopping at the last note clips the tail.
    [[nodiscard]] std::int64_t last_event_position() const noexcept { return last_event_position_; }

    /// Current position in samples.
    [[nodiscard]] std::int64_t position() const noexcept { return position_; }

    /// Whether every event has been dispatched and nothing is still sounding.
    [[nodiscard]] bool at_end() const noexcept
    {
        return cursor_ >= events_.size() && generator_->active_voices() == 0;
    }

    /// Renders audio, dispatching every event that falls inside it.
    ///
    /// Throws `std::invalid_argument` if the two channels differ in length.
    void render(std::span<float> left, std::span<float> right);

    /// Streams the whole file into memory, from wherever the player currently is.
    ///
    /// The length is computed the same way the offline renderer computes it, so the two line up
    /// sample for sample.
    [[nodiscard]] RenderResult render_to_end(double tail_seconds = 2.2,
                                             std::optional<double> end_seconds = std::nullopt);

    /// Jumps to a position, leaving the engine in the state the file would have put it in.
    ///
    /// Every event up to that point is replayed except the notes themselves, so program changes,
    /// bank selects, controllers and the GS effect selections all arrive — a seek into the middle
    /// of a song sounds the way playing up to that point would, without the notes in between.
    void seek(std::int64_t sample);

private:
    ToneGenerator* generator_;
    std::vector<MidiEvent> events_;
    std::int64_t last_event_position_ = 0;
    std::int64_t position_ = 0;
    std::size_t cursor_ = 0;
};

} // namespace ts
