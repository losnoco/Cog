#pragma once

#include "tabulasonora/envelope_machine.hpp"

#include <array>
#include <cstdint>
#include <span>

namespace ts {

/// A four-segment envelope with a release, evaluated at any sample position.
///
/// The TVA and TVF envelopes have the same shape — four segments that run from note-on, a hold at
/// the last target, then a release that starts wherever note-off caught it. What differs is only
/// how the targets and durations are decoded, which is the job of `TvaChain` and `TvfChain`.
///
/// The trajectory is a pure function of the sample index and the note-off position, so the offline
/// renderer and the real-time voice loop read the same envelope by construction rather than by
/// agreement between two transcriptions. Nothing here is advanced by rendering: a block renderer
/// evaluates it per sample, and an offline one fills an array.
class SegmentEnvelope {
public:
    /// Segments before the release.
    static constexpr int segment_count = 4;

    /// Builds an envelope from decoded segment parameters.
    ///
    /// `control_tick_samples` is the grid note-off is acted on. A tick of one releases at the note
    /// after note-off, which is as close to releasing immediately as this gets and is *not* what
    /// the engine does; see `note_off`.
    ///
    /// Throws `std::invalid_argument` if a parameter array is not four long.
    SegmentEnvelope(const EnvelopeMachine& machine,
                    std::span<const double> targets,
                    std::span<const double> segment_samples,
                    std::span<const bool> linear,
                    double release_target,
                    double release_samples,
                    bool release_linear,
                    double after_release,
                    std::int64_t control_tick_samples);

    /// Where the release starts, or -1 while the note is still held.
    [[nodiscard]] std::int64_t note_off_sample() const noexcept { return note_off_; }

    /// The release duration in samples, at least one.
    [[nodiscard]] std::int64_t release_samples() const noexcept { return release_samples_; }

    /// Starts the release at a sample position. Later calls are ignored.
    ///
    /// The release departs from the value the envelope had reached, not from a segment target,
    /// which is what makes a note released mid-attack decay from where it actually was.
    ///
    /// Note-off does not take effect at the sample it lands on: the engine sees it at its next
    /// control tick, so the envelope holds for the rest of the current tick first. Measured by
    /// sweeping the hold time past a tick boundary — note-off anywhere in 1000–1008 ms produced the
    /// same release, which then stepped a whole tick later at 1010 ms. Releasing immediately
    /// instead runs the tail 0–10 ms early; that is inaudible on a pad but a large fraction of a
    /// short release, and the Accordion reaches -20 dB in 9 ms against the engine's 23 ms.
    ///
    /// `damper` is the CC64 value at release, 1–0x3f for a half-pressed pedal on a half-damper
    /// tone, else zero. The engine writes it into the release ramp's rate-scale byte, which
    /// multiplies the rate word by roughly `1 - v/128`, so a half-pressed pedal lengthens the
    /// release by the reciprocal. Only the 57 piano tones carry the capability; every other tone's
    /// pedal value is quantised to 0 or 0x7f before it can get here.
    void note_off(std::int64_t sample, int damper = 0);

    /// The control tick an event landing at a sample is acted on.
    ///
    /// The *following* tick, even when the event lands exactly on a boundary: that tick's update
    /// has already run by the time the event is latched. Measured on the DLL — a note-off at 1010
    /// ms, exactly a tick, released at 1020 ms, while one at 1008 ms released at 1010 ms. So the
    /// deferral spans one full tick and is never zero.
    [[nodiscard]] static std::int64_t
    defer_to_control_tick(std::int64_t sample, std::int64_t control_tick_samples) noexcept;

    /// The envelope's value at a sample position relative to note-on.
    [[nodiscard]] double value_at(std::int64_t sample) const noexcept;

    /// Whether the release has run out at a sample position.
    [[nodiscard]] bool is_finished(std::int64_t sample) const noexcept
    {
        return note_off_ >= 0 && sample - note_off_ >= release_samples_;
    }

private:
    [[nodiscard]] double held(std::int64_t sample) const noexcept;

    const EnvelopeMachine* machine_;
    std::array<double, segment_count> targets_{};
    std::array<bool, segment_count> linear_{};
    std::array<std::int64_t, segment_count> from_{};
    std::array<std::int64_t, segment_count> to_{};
    std::array<double, segment_count> span_{};

    double release_target_ = 0.0;
    bool release_linear_ = false;
    double release_span_ = 0.0;
    std::int64_t release_samples_ = 1;
    double after_release_ = 0.0;
    std::int64_t control_tick_ = 1;

    std::int64_t note_off_ = -1;
    double at_note_off_ = 0.0;
};

} // namespace ts
