#pragma once

#include "tabulasonora/note_renderer.hpp"
#include "tabulasonora/sequence.hpp"

#include <atomic>
#include <filesystem>
#include <optional>
#include <vector>

namespace ts {

/// Per-channel mute and solo.
///
/// Read live so a mixer can change it while sound is running; a render takes one snapshot for its
/// whole duration, or some notes of a part would sound and others not.
///
/// The flags are atomic because this is the one place in the library where two threads meet: a UI
/// thread sets them while the render thread reads them on every note-on. Relaxed ordering is enough
/// -- each flag stands alone, nothing is published through it, and the only requirement is that a
/// change is eventually seen and never seen half-written. That is what the reference build's
/// `Volatile` bought too.
class ChannelMask {
public:
    static constexpr int channel_count = 16;

    /// Whether a channel should sound.
    ///
    /// Solo wins: once anything is soloed, only soloed channels are audible regardless of mutes.
    [[nodiscard]] bool is_audible(int channel) const noexcept;

    [[nodiscard]] bool is_muted(int channel) const noexcept;
    [[nodiscard]] bool is_soloed(int channel) const noexcept;
    [[nodiscard]] bool any_soloed() const noexcept;

    void set_muted(int channel, bool muted) noexcept;
    void set_soloed(int channel, bool soloed) noexcept;
    void reset() noexcept;

    /// Whether nothing is muted or soloed, so every channel sounds.
    [[nodiscard]] bool is_default() const noexcept;

private:
    std::array<std::atomic<bool>, channel_count> muted_{};
    std::array<std::atomic<bool>, channel_count> soloed_{};
};

/// How a sequence should be rendered.
struct RenderOptions {
    /// Which vintage's tone map program changes resolve against.
    ToneMap map = ToneMap::sc8820;

    /// Time to render past the last note.
    double tail_seconds = 2.2;

    /// Linear gain applied to the finished mix.
    double output_gain = 1.0;

    /// MIDI channel routed to the drum path.
    int drum_channel = 9;

    /// Drum map row, or nothing to take the row the vintage selects.
    ///
    /// Nothing in a MIDI file can reach this: the module picks it from the part's internal bank
    /// code, and that translation is not reversed. Both renderers take it, and they must, or one
    /// file would render as two different arrangements.
    std::optional<int> drum_map_row;

    /// The row this actually resolves to.
    [[nodiscard]] int effective_drum_map_row() const noexcept
    {
        if (drum_map_row) {
            return *drum_map_row;
        }
        return DrumKitTable::row_for_map(map).value_or(0);
    }

    bool reverb = true;
    bool chorus = true;
    bool delay = true;

    /// Force an effect type instead of taking it from the stream.
    std::optional<int> reverb_type;
    std::optional<int> chorus_type;
    std::optional<int> delay_type;

    /// Stop rendering at this many seconds, if given.
    std::optional<double> end_seconds;

    /// Per-channel mute and solo, or nothing for everything audible.
    const ChannelMask* channels = nullptr;

    /// How long a drum hit rings before its release is spliced in.
    double drum_ring_seconds = 1.8;

    /// Extra time past a drum's ring.
    double drum_tail_seconds = 0.4;
};

/// A finished render.
struct RenderResult {
    std::vector<float> left;
    std::vector<float> right;
    int sample_rate = NoteRenderer::sample_rate;
    /// How many notes actually rendered.
    int note_count = 0;
    /// Peak absolute sample across both channels, after the output gain.
    float peak = 0.0F;
};

/// Renders a whole sequence note by note and mixes the send effects over it.
///
/// Every note is rendered whole and summed into one buffer, so polyphony is unbounded here — the
/// hardware's 64-voice limit belongs to the block loop, which has a notion of *now* that this path
/// does not.
class SequenceRenderer {
public:
    /// Creates a renderer over a note renderer, which must outlive it.
    explicit SequenceRenderer(NoteRenderer& notes) : notes_(&notes) {}

    /// Renders a Standard MIDI File.
    [[nodiscard]] RenderResult render_file(const std::filesystem::path& path,
                                           const RenderOptions& options = {});

    /// Renders a parsed sequence.
    [[nodiscard]] RenderResult render(const Sequence& sequence, const RenderOptions& options = {});

private:
    NoteRenderer* notes_;
};

} // namespace ts
