#pragma once

#include "tabulasonora/render_options.hpp"
#include "tabulasonora/smf_reader.hpp"
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

    /// Creates a player over an engine and a parsed song, keeping its loop points.
    ///
    /// The loop points only matter once `set_loop_count` asks for looping; the default remains a
    /// single straight play-through.
    SequencePlayer(ToneGenerator& generator, smf::Song song);

    /// Reads a music file and creates a player for it.
    ///
    /// Every format `smf::load` handles, loop points included.
    [[nodiscard]] static SequencePlayer from_file(ToneGenerator& generator,
                                                  const std::filesystem::path& path);

    /// Which parts the file actually addresses, as `port * 16 + channel`.
    ///
    /// A mixer wants the channels a file uses, not sixteen rows of which four are silent -- and a
    /// four-port file has sixty-four to choose from, so showing them all would be worse than
    /// useless. Computed once from the event list, so it is what the file contains rather than what
    /// has been reached so far.
    [[nodiscard]] std::vector<int> addressed_parts() const;

    /// The engine being driven.
    [[nodiscard]] ToneGenerator& generator() noexcept { return *generator_; }

    /// Position of the final event of any kind.
    ///
    /// Not the same as the last note: a file commonly closes with controller or meta traffic after
    /// the music stops, and stopping at the last note clips the tail.
    [[nodiscard]] std::int64_t last_event_position() const noexcept { return last_event_position_; }

    /// Current position in samples.
    [[nodiscard]] std::int64_t position() const noexcept { return position_; }

    /// The file's loop points, when it declared any.
    [[nodiscard]] const std::optional<smf::SongLoop>& loop() const noexcept { return loop_; }

    /// How many times the looped section should play, counting the first pass.
    ///
    /// The vocabulary is the reference sequencer's: `0` or `1` plays the song once straight
    /// through, which is the default; `-1` loops forever; `N >= 2` targets `N` play-throughs of
    /// the loop body, after which the music keeps looping under a fade to silence rather than
    /// trailing off mid-phrase. A file with no loop points loops whole under `-1` and plays
    /// straight through under any finite count. Asking for a count at or below the play-through
    /// already reached starts the fade immediately; switching to `-1` cancels a pending fade.
    void set_loop_count(int count);

    [[nodiscard]] int loop_count() const noexcept { return loop_count_; }

    /// Completed passes over the loop body, counting up from zero on the initial pass.
    [[nodiscard]] int loops_played() const noexcept { return loops_played_; }

    /// The post-loop fade length. Zero cuts at the loop end.
    void set_fade_seconds(double seconds);

    /// Whether every event has been dispatched and nothing is still sounding, or the post-loop
    /// fade has run to silence.
    [[nodiscard]] bool at_end() const noexcept
    {
        return finished_ || (cursor_ >= events_.size() && generator_->active_voices() == 0);
    }

    /// Renders audio, dispatching every event that falls inside it.
    ///
    /// When looping is engaged this is also where the jumps happen: a *soft* loop -- one whose
    /// end the file marked explicitly -- rewinds with an all-notes-off and nothing else, because
    /// whatever must change at the jump is written inside the loop body and re-fires on its own;
    /// a *hard* loop's end was inferred, so the jump replays state the way a seek does.
    ///
    /// Throws `std::invalid_argument` if the two channels differ in length.
    void render(std::span<float> left, std::span<float> right);

    /// Streams the whole file into memory, from wherever the player currently is.
    ///
    /// The length is computed the same way the offline renderer computes it, so the two line up
    /// sample for sample. A finite loop count extends it to cover the passes and the fade; an
    /// infinite one is ignored here, because a buffer cannot hold forever.
    [[nodiscard]] RenderResult render_to_end(double tail_seconds = 2.2,
                                             std::optional<double> end_seconds = std::nullopt);

    /// Jumps to a position, leaving the engine in the state the file would have put it in.
    ///
    /// Every event up to that point is replayed except the notes themselves, so program changes,
    /// bank selects, controllers and the GS effect selections all arrive — a seek into the middle
    /// of a song sounds the way playing up to that point would, without the notes in between.
    ///
    /// A manual seek also cancels any post-loop fade and restarts the loop counter.
    void seek(std::int64_t sample);

private:
    /// The seek body: replays state up to `sample` without touching the loop bookkeeping.
    void replay_to(std::int64_t sample);

    /// Silences every part the engine has, for the soft-loop rewind.
    void all_notes_off();

    /// Dispatches every pending event at the current position.
    void dispatch();

    /// Dispatches everything inside the next `samples`, stamped by where each event falls.
    void dispatch_within(std::int64_t samples);

    /// Handles a loop-end arrival: counts the pass, starts the fade when the target is reached,
    /// and jumps. Returns whether a jump happened.
    bool handle_loop_point();

    ToneGenerator* generator_;
    std::vector<MidiEvent> events_;
    std::int64_t last_event_position_ = 0;
    std::int64_t position_ = 0;
    std::size_t cursor_ = 0;

    std::optional<smf::SongLoop> loop_;
    int loop_count_ = 1;
    int loops_played_ = 0;
    double fade_seconds_ = 7.0;
    /// Total samples ever rendered. Monotonic where `position_` jumps backward at a loop, which
    /// is what makes it the clock the fade can run on.
    std::int64_t rendered_ = 0;
    /// The `rendered_` time the fade began, or negative for no fade.
    std::int64_t fade_start_ = -1;
    bool finished_ = false;
    /// Notes the engine counted before a replay reset it, so a looped render reports all of them.
    int notes_before_reset_ = 0;
};

} // namespace ts
